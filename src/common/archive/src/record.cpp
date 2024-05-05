#include "stdafx.hpp"

#include "archive/record.hpp"

#include "core/error.hpp"
#include "core/format.hpp" // IWYU pragma: keep

#include "core/units.hpp"
#include "logs/logs.hpp"

using namespace sm;
using namespace sm::archive;

static_assert(sizeof(RecordStoreHeader) == 20);
static_assert(sizeof(RecordEntryHeader) == 8);

static constexpr uint32_t kFileMagic = '\0MUC';
static constexpr RecordStoreVersion kCurrentVersion = RecordStoreVersion::eCurrent;

static constexpr size_t header_size(size_t record_count) {
    return sizeof(RecordStoreHeader) + (record_count * sizeof(RecordEntryHeader));
}

// fletcher32 checksum
static constexpr uint32_t checksum(const uint8_t *data, size_t size) {
    uint32 sum1 = 0;
    uint32 sum2 = 0;

    for (size_t i = 0; i < size; i++) {
        sum1 = (sum1 + data[i]) % UINT16_MAX;
        sum2 = (sum2 + sum1) % UINT16_MAX;
    }

    return (sum2 << 16) | sum1;
}

template<typename... A>
constexpr bool check(bool expr, fmt::format_string<A...> fmt, A&&... args) {
    if (!expr) {
        logs::gAssets.error(fmt, std::forward<A>(args)...);
        return false;
    }
    return true;
}

bool RecordLookup::has_valid_data() const {
    return m_value == eCreated || m_value == eOpened;
}

bool RecordStore::create() {
    // do first time initialization of file header
    auto setup_file = [&] {
        RecordStoreHeader data = {
            .magic = kFileMagic,
            .version = kCurrentVersion,
            .count = int_cast<uint16>(mCapacity),
            .used = int_cast<uint16>(mUsed),
            .size = int_cast<uint32>(mSize.asBytes()),
            .checksum = 0, // we calculate checksum at closing
        };

        RecordStoreHeader *header = get_private_header();
        std::memcpy(header, &data, sizeof(data));
    };

    // we have an existing file, validate it
    auto validate_file = [&]() -> bool {
        RecordStoreHeader *header = get_private_header();

        if (!check(header->version == kCurrentVersion, "file version mismatch, {}/{}",
                       header->version, kCurrentVersion))
            return false;

        sm::Memory actual_size = header->size;
        if (!check(mSize == actual_size, "file size mismatch, {}/{}", actual_size,
                       mSize))
            return false;

        uint32_t checksum = calc_checksum();
        if (!check(header->checksum == checksum,
                       "file checksum mismatch, {:#08x}/{:#08x}", header->checksum, checksum))
            return false;

        if (!check(header->count == mCapacity, "file record count mismatch, {}/{}",
                       header->count, mCapacity))
            return false;

        if (!check(header->used <= header->count, "file used count over limit, {}/{}",
                       header->used, header->count))
            return false;

        return true;
    };

    static constexpr os_access_t kAccess = eOsAccessRead | eOsAccessWrite;

    if (OsError err = os_file_open(mFilePath, kAccess, &mFileHandle)) {
        logs::gAssets.error("unable to open file, {}", err);
        return false;
    }

    // resize file to the desired size

    CTASSERTF(mSize <= UINT32_MAX, "file size too large: %s", mSize.toString().c_str());

    size_t size_as_bytes = mSize.asBytes();

    if (OsError err = os_file_resize(&mFileHandle, size_as_bytes)) {
        logs::gAssets.error("unable to expand file, {}", err);
        return false;
    }

    // map the file into memory

    static constexpr os_protect_t kProtect = eOsProtectRead | eOsProtectWrite;

    if (OsError err = os_file_map(&mFileHandle, kProtect, size_as_bytes, &mMapHandle)) {
        logs::gAssets.error("unable to map file, {}", err);
        return false;
    }

    CTASSERTF(os_mapping_active(&mMapHandle), "failed to map %s, but os_file_map was successful", mFilePath);

    mMemory = os_mapping_data(&mMapHandle);

    // read in the file header

    RecordStoreHeader *header = get_private_header();
    switch (header->magic) {
    case 0: // no magic, assume new file
        setup_file();
        break;

    case kFileMagic: // magic matches, validate file before use
        if (!validate_file()) return false;
        break;

    default: // magic doesnt match, dont use it to avoid stomping
        destroy();
        return false;
    }

    sm::Memory public_size = get_public_size();
    if (public_size.asBytes() % 8 != 0) {
        logs::gAssets.error("public_size size must be a multiple of 8, {}", public_size);
        return false;
    }

    size_t as_bytes = public_size.asBytes();

    // intialize allocation data
    mUsed = header->used;
    mCapacity = header->count;

    // mark header region as used
    mIndexAllocator.resize(as_bytes / 8);
    mIndexAllocator.setRange(0, as_bytes / 8);

    // mark already used record data as used
    for (uint32_t i = 0; i < mUsed; i++) {
        const RecordEntryHeader *record = get_record_header(i);
        if (record->id == 0) continue;

        mIndexAllocator.setRange(record->offset, record->offset + record->size);
    }

    return true;
}

void RecordStore::destroy() {
    logs::gAssets.info("closing record store {}", mFilePath);
    // unmap the file from memory
    if (is_valid())
        update_header();

    mMemory = nullptr;

    if (os_mapping_active(&mMapHandle)) {
        if (OsError err = os_unmap(&mMapHandle)) {
            logs::gAssets.error("unable to unmap file, {}", err);
        }
    }

    if (OsError err = os_file_close(&mFileHandle)) {
        logs::gAssets.error("unable to close file, {}", err);
    }

    // reset the size
    mSize = 0;

    mIndexAllocator.clear();
}

RecordLookup RecordStore::get_record(uint32_t id, void **data, uint16_t size) {
    logs::gAssets.info("looking for record {}", id);
    RecordEntryHeader *found = nullptr;
    for (uint32_t i = 0; i <= mUsed; i++) {
        RecordEntryHeader *record = get_record_header(i);
        if (!found && record->id == 0) {
            found = record;
            continue;
        }

        if (record->id != id) continue;

        if (!check(record->size == size,
                       "record size mismatch, somebody didnt update the version number, {}/{}",
                       record->size, size))
            return RecordLookup::eRecordInvalid;

        *data = get_record_data(record->offset);
        logs::gAssets.info("record found\n| id: {}\n| size: {}\n| offset: {}", id, size, record->offset);
        return RecordLookup::eOpened;
    }

    if (!check(found != nullptr, "record slots at capacity, cant create new record, {}/{}",
                  mUsed, mCapacity))
        return RecordLookup::eRecordTableExhausted;

    // find free space

    // mAllocator operates on 8 byte chunks, convert the size and scan for free space
    uint16_t chunks = (size + 7) / 8;
    size_t index = mIndexAllocator.allocateRange(chunks);

    if (!check(index != BitMapIndexAllocator::kInvalidIndex,
                   "unable to find free record header, {}/{}", size, mIndexAllocator.freecount()))
        return RecordLookup::eDataRegionExhausted;

    // write record header
    found->id = id;
    found->offset = uint32_t(index * 8);
    found->size = size;

    mUsed += 1;

    // clear the data
    void *ptr = get_record_data(found->offset);
    std::memset(ptr, 0, size);
    *data = ptr;

    logs::gAssets.info("record created\n| id: {}\n| size: {}\n| offset: {}", id, size, found->offset);

    return RecordLookup::eCreated;
}

void RecordStore::init_alloc_info() {
    size_t public_size = get_public_size();

    // mark header region as used
    mIndexAllocator.resize(public_size / 8);
    mIndexAllocator.setRange(0, public_size / 8);

    // mark already used record data as used
    for (uint32_t i = 0; i < mUsed; i++) {
        const RecordEntryHeader *record = get_record_header(i);
        if (record->id == 0) continue;
        mIndexAllocator.setRange(record->offset, record->offset + record->size);
    }
}

void RecordStore::update_header() {
    RecordStoreHeader *header = get_private_header();
    RecordStoreHeader it = {
        .magic = kFileMagic,
        .version = kCurrentVersion,
        .count = header->count,
        .used = header->used,
        .size = uint32_t(mSize.asBytes()),
        .checksum = calc_checksum(),
    };

    std::memcpy(header, &it, sizeof(it));
}

void RecordStore::destroy_safe() {
    if (is_data_mapped()) destroy();
}

RecordStore::RecordStore(const RecordStoreConfig &config)
    : mFilePath(config.path)
    , mSize(config.size.asBytes() + (header_size(config.record_count)))
    , mIndexAllocator(64)
    , mCapacity(config.record_count)
{
    CTASSERT(mFilePath != nullptr);
    CTASSERT(mSize > 0);
    CTASSERT(mCapacity > 0);

    mValid = create();
    if (mValid) {
        logs::gAssets.info("record store created\n| path: {}\n| size: {}\n| records: {}", mFilePath, mSize,
                   mCapacity);
    }
}

RecordStore::~RecordStore() {
    destroy_safe();
}

void RecordStore::reset() {
    // this doesnt unmap the file, only resets the contents

    // reset the header
    RecordStoreHeader *header = get_private_header();
    header->magic = kFileMagic;
    header->version = kCurrentVersion;
    header->size = uint32_t(mSize.asBytes());
    header->checksum = 0;
    header->count = uint16_t(mCapacity);
    header->used = 0;

    // reset the space bitmap
    mIndexAllocator.resizeAndClear(get_public_size() / 8);

    init_alloc_info();
}

uint32_t RecordStore::read_checksum() const {
    CTASSERT(is_data_mapped());

    RecordStoreHeader *header = get_private_header();
    return header->checksum;
}

uint32_t RecordStore::get_record_count() const {
    return mCapacity;
}

RecordStoreHeader *RecordStore::get_private_header() const {
    CTASSERT(is_data_mapped());

    return reinterpret_cast<RecordStoreHeader *>(get_private_region());
}

uint8_t *RecordStore::get_public_region() const {
    CTASSERT(is_data_mapped());

    return reinterpret_cast<uint8_t *>(mMemory) + header_size(get_record_count());
}

RecordEntryHeader *RecordStore::get_record_header(uint32_t index) const {
    return reinterpret_cast<RecordEntryHeader *>(get_private_region() + header_size(index));
}

uint8_t *RecordStore::get_private_region() const {
    CTASSERT(is_data_mapped());

    return reinterpret_cast<uint8_t *>(mMemory);
}

size_t RecordStore::get_public_size() const {
    CTASSERT(is_data_mapped());
    return mSize.asBytes() - header_size(mCapacity);
}

void *RecordStore::get_record_data(uint32_t offset) const {
    return get_public_region() + offset;
}

uint32_t RecordStore::calc_checksum() const {
    return checksum(get_public_region(), get_public_size());
}
