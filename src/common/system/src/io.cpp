#include "system/io.hpp"

#include "core/format.hpp" // IWYU pragma: keep

using namespace sm;
using namespace sm::sys;

static_assert(sizeof(FileHeader) == 20);
static_assert(sizeof(RecordHeader) == 8);

static constexpr uint32_t kFileMagic = '\0MUC';
static constexpr FileVersion kCurrentVersion = FileVersion::eVersion0;

#define SM_CHECKF(sink, expr, ...)               \
    [&]() -> bool {                              \
        if (auto result = (expr); !result) {     \
            (sink).error(#expr " = {}", result); \
            (sink).error(__VA_ARGS__);           \
            return false;                        \
        }                                        \
        return true;                             \
    }()

static constexpr size_t header_size(size_t record_count) {
    return sizeof(FileHeader) + (record_count * sizeof(RecordHeader));
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

bool RecordLookup::has_valid_data() const {
    return m_value == eCreated || m_value == eOpened;
}

bool FileMapping::create() {
    // do first time initialization of file header
    auto setup_file = [&] {
        FileHeader data = {
            .magic = kFileMagic,
            .version = kCurrentVersion,
            .size = int_cast<uint32>(mSize.as_bytes()),
            .checksum = 0, // we calculate checksum at closing
            .count = int_cast<uint16>(mCapacity),
            .used = int_cast<uint16>(mUsed),
        };

        FileHeader *header = get_private_header();
        std::memcpy(header, &data, sizeof(data));
    };

    // we have an existing file, validate it
    auto validate_file = [this]() -> bool {
        FileHeader *header = get_private_header();

        if (!SM_CHECKF(mSink, header->version == kCurrentVersion, "file version mismatch, {}/{}",
                       header->version, kCurrentVersion))
            return false;

        sm::Memory actual_size = header->size;
        if (!SM_CHECKF(mSink, mSize == actual_size, "file size mismatch, {}/{}", actual_size,
                       mSize))
            return false;

        uint32_t checksum = calc_checksum();
        if (!SM_CHECKF(mSink, header->checksum == checksum,
                       "file checksum mismatch, {:#08x}/{:#08x}", header->checksum, checksum))
            return false;

        if (!SM_CHECKF(mSink, header->count == mCapacity, "file record count mismatch, {}/{}",
                       header->count, mCapacity))
            return false;

        if (!SM_CHECKF(mSink, header->used <= header->count, "file used count over limit, {}/{}",
                       header->used, header->count))
            return false;

        return true;
    };

    static constexpr os_access_t kAccess = os_access_t(eAccessRead | eAccessWrite);

    if (OsError err = os_file_open(mFilePath, kAccess, &mFileHandle)) {
        mSink.error("unable to open file, {}", err);
        return false;
    }

    // resize file to the desired size

    CTASSERTF(mSize <= UINT32_MAX, "file size too large: %s", mSize.to_string().c_str());

    size_t size_as_bytes = mSize.as_bytes();

    if (OsError err = os_file_expand(&mFileHandle, size_as_bytes)) {
        mSink.error("unable to expand file, {}", err);
        return false;
    }

    // map the file into memory

    static constexpr os_protect_t kProtect = os_protect_t(eProtectRead | eProtectWrite);

    if (OsError err = os_file_map(&mFileHandle, kProtect, size_as_bytes, &mMappedRegion)) {
        mSink.error("unable to map file, {}", err);
        return false;
    }

    CTASSERTF(os_mapping_active(&mMappedRegion), "mapping for %s not active", mFilePath);

    mData = os_mapping_data(&mMappedRegion);

    // read in the file header

    FileHeader *header = get_private_header();
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
    if (public_size.as_bytes() % 8 != 0) {
        mSink.error("public_size size must be a multiple of 8, {}", public_size);
        return false;
    }

    size_t as_bytes = public_size.as_bytes();

    // intialize allocation data
    mUsed = header->used;
    mCapacity = header->count;

    // mark header region as used
    mAllocator.resize(as_bytes / 8);
    mAllocator.set_range(BitMap::Index(0), BitMap::Index(as_bytes / 8));

    // mark already used record data as used
    for (uint32_t i = 0; i < mUsed; i++) {
        const RecordHeader *record = get_record_header(i);
        if (record->id == 0) continue;

        mAllocator.set_range(BitMap::Index(record->offset),
                          BitMap::Index(record->offset + record->size));
    }

    return true;
}

void FileMapping::destroy() {
    mSink.info("destroying file mapping for {}", mFilePath);
    // unmap the file from memory
    if (mData != nullptr) {
        // only update the header if the file is valid
        // we dont want to trample over a file that we didnt create
        if (is_valid()) update_header();
        mData = nullptr;
    }

    if (os_mapping_active(&mMappedRegion)) {
        os_file_unmap(&mMappedRegion);
    }

    os_file_close(&mFileHandle);

    // reset the size
    mSize = 0;

    mAllocator.clear();
}

RecordLookup FileMapping::get_record(uint32_t id, void **data, uint16_t size) {
    for (uint32_t i = 0; i <= mUsed; i++) {
        RecordHeader *record = get_record_header(i);
        if (record->id != id) continue;

        if (!SM_CHECKF(mSink, record->size == size,
                       "record size mismatch, somebody didnt update the version number, {}/{}",
                       record->size, size))
            return RecordLookup::eRecordInvalid;

        *data = get_record_data(record->offset);
        return RecordLookup::eOpened;
    }

    // no existing record found, create a new one
    if (!SM_CHECKF(mSink, mCapacity >= mUsed,
                   "record slots at capacity, cant create new record, {}/{}", mUsed, mCapacity))
        return RecordLookup::eRecordTableExhausted;

    // find free record header
    RecordHeader *found = nullptr;
    for (uint_fast16_t i = 0; i < mCapacity; i++) {
        RecordHeader *record = get_record_header(i);
        if (record->id == 0) {
            found = record;
            break;
        }
    }

    if (!SM_CHECKF(mSink, found != nullptr,
                   "unable to find free space, {:#08x} required, {:#08x} free total. defragment or "
                   "make the file bigger",
                   mUsed, mCapacity))
        return RecordLookup::eRecordTableExhausted;

    // find free space

    // mAllocator operates on 8 byte chunks, convert the size and scan for free space
    uint16_t chunks = (size + 7) / 8;
    BitMap::Index index = mAllocator.scan_set_range(BitMap::Index{chunks});

    if (!SM_CHECKF(mSink, index != BitMap::Index::eInvalid,
                   "unable to find free record header, {}/{}", size, mAllocator.freecount()))
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

    mSink.info("record created\n| id: {}\n| size: {}\n| offset: {}", id, size, found->offset);

    return RecordLookup::eCreated;
}

void FileMapping::init_alloc_info() {
    size_t public_size = get_public_size();

    // mark header region as used
    mAllocator.resize(public_size / 8);
    mAllocator.set_range(BitMap::Index(0), BitMap::Index(public_size / 8));

    // mark already used record data as used
    for (uint32_t i = 0; i < mUsed; i++) {
        const RecordHeader *record = get_record_header(i);
        if (record->id == 0) continue;
        mAllocator.set_range(BitMap::Index(record->offset),
                          BitMap::Index(record->offset + record->size));
    }
}

void FileMapping::update_header() {
    FileHeader *header = get_private_header();
    FileHeader it = {
        .magic = kFileMagic,
        .version = kCurrentVersion,
        .size = uint32_t(mSize.as_bytes()),
        .checksum = calc_checksum(),
        .count = header->count,
        .used = header->used,
    };

    std::memcpy(header, &it, sizeof(it));
}

void FileMapping::destroy_safe() {
    if (is_data_mapped()) destroy();
}

FileMapping::FileMapping(const MappingConfig &config)
    : mFilePath(config.path)
    , mSink(config.logger)
    , mSize(config.size.b() + (header_size(config.record_count)))
    , mCapacity(config.record_count) {
    CTASSERT(mFilePath != nullptr);
    CTASSERT(mSize > 0);
    CTASSERT(mCapacity > 0);

    mValid = create();
    if (mValid) {
        mSink.info("file mapping created\n| path: {}\n| size: {}\n| records: {}", mFilePath, mSize,
                   mCapacity);
    }
}

FileMapping::~FileMapping() {
    destroy_safe();
}

void FileMapping::reset() {
    // this doesnt unmap the file, only resets the contents

    // reset the header
    FileHeader *header = get_private_header();
    header->magic = kFileMagic;
    header->version = kCurrentVersion;
    header->size = uint32_t(mSize.as_bytes());
    header->checksum = 0;
    header->count = uint16_t(mCapacity);
    header->used = 0;

    // reset the space bitmap
    if (mAllocator.is_valid())
        mAllocator.clear();
    else
        mAllocator.resize(get_public_size() / 8);

    init_alloc_info();
}

uint32_t FileMapping::read_checksum() const {
    CTASSERT(is_data_mapped());

    FileHeader *header = get_private_header();
    return header->checksum;
}

uint32_t FileMapping::get_record_count() const {
    return mCapacity;
}

FileHeader *FileMapping::get_private_header() const {
    CTASSERT(is_data_mapped());

    return reinterpret_cast<FileHeader *>(get_private_region());
}

uint8_t *FileMapping::get_public_region() const {
    CTASSERT(is_data_mapped());

    return reinterpret_cast<uint8_t *>(mData) + header_size(get_record_count());
}

RecordHeader *FileMapping::get_record_header(uint32_t index) const {
    return reinterpret_cast<RecordHeader *>(get_private_region() + sizeof(FileHeader) +
                                            (index * sizeof(RecordHeader)));
}

uint8_t *FileMapping::get_private_region() const {
    CTASSERT(is_data_mapped());

    return reinterpret_cast<uint8_t *>(mData);
}

size_t FileMapping::get_public_size() const {
    CTASSERT(is_data_mapped());
    return mSize.as_bytes() - header_size(mCapacity);
}

void *FileMapping::get_record_data(uint32_t offset) const {
    return get_public_region() + offset;
}

uint32_t FileMapping::calc_checksum() const {
    return checksum(get_public_region(), get_public_size());
}
