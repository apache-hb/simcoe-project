#include "system/io.hpp"

#include "core/reflect.hpp" // IWYU pragma: keep

#include <winbase.h>

using namespace sm::sys;

static_assert(sizeof(FileHeader) == 20);
static_assert(sizeof(RecordHeader) == 8);

static constexpr uint32_t kFileMagic = 'CUM\0';
static constexpr FileVersion kCurrentVersion = FileVersion::eVersion0;

#define SM_CHECKF(sink, expr, ...) \
    [&]() -> bool {          \
        if (auto result = (expr); !result) {       \
            (sink).error(#expr " = {}", result); \
            (sink).error(__VA_ARGS__);            \
            return false;    \
        }                    \
        return true;         \
    }()

static constexpr size_t header_size(size_t record_count) {
    return sizeof(FileHeader) + (record_count * sizeof(RecordHeader));
}

// fletcher32 checksum
static constexpr uint32_t checksum(const uint8_t *data, size_t size) {
    uint32_t sum1 = 0;
    uint32_t sum2 = 0;

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
            .size = uint32_t(m_size.as_bytes()),
            .checksum = 0, // we calculate checksum at closing
            .count = uint16_t(m_capacity),
            .used = uint16_t(m_used),
        };

        FileHeader *header = get_private_header();
        std::memcpy(header, &data, sizeof(data));
    };

    // we have an existing file, validate it
    auto validate_file = [this]() -> bool {
        FileHeader *header = get_private_header();

        if (!SM_CHECKF(m_log, header->version == kCurrentVersion, "file version mismatch, {}/{}", header->version, kCurrentVersion))
            return false;

        sm::Memory actual_size = header->size;
        if (!SM_CHECKF(m_log, m_size == actual_size, "file size mismatch, {}/{}", actual_size, m_size))
            return false;

        uint32_t checksum = calc_checksum();
        if (!SM_CHECKF(m_log, header->checksum == checksum, "file checksum mismatch, {:#08x}/{:#08x}", header->checksum, checksum))
            return false;

        if (!SM_CHECKF(m_log, header->count == m_capacity, "file record count mismatch, {}/{}", header->count, m_capacity))
            return false;

        if (!SM_CHECKF(m_log, header->used <= header->count, "file used count over limit, {}/{}", header->used, header->count))
            return false;

        return true;
    };

    m_file = CreateFileA(
        /* lpFileName = */ m_path,
        /* dwDesiredAccess = */ GENERIC_READ | GENERIC_WRITE,
        /* dwShareMode = */ FILE_SHARE_READ,
        /* lpSecurityAttributes = */ nullptr,
        /* dwCreationDisposition = */ OPEN_ALWAYS,
        /* dwFlagsAndAttributes = */ FILE_ATTRIBUTE_NORMAL,
        /* hTemplateFile = */ nullptr);

    if (!SM_CHECK_WIN32(m_file != INVALID_HANDLE_VALUE, m_log)) return false;

    // resize file to the desired size

    CTASSERTF(m_size <= UINT32_MAX, "file size too large: %s", m_size.to_string().c_str());
    LARGE_INTEGER file_size = { .QuadPart = LONGLONG(m_size.as_bytes()) };

    if (!SM_CHECK_WIN32(SetFilePointerEx(m_file, file_size, nullptr, FILE_BEGIN), m_log)) return false;
    if (!SM_CHECK_WIN32(SetEndOfFile(m_file), m_log)) return false;

    // map the file into memory

    m_mapping = CreateFileMappingA(
        /* hFile = */ m_file,
        /* lpFileMappingAttributes = */ nullptr,
        /* flProtect = */ PAGE_READWRITE,
        /* dwMaximumSizeHigh = */ 0,
        /* dwMaximumSizeLow = */ DWORD(m_size.as_bytes()),
        /* lpName = */ nullptr);

    if (!SM_CHECK_WIN32(m_mapping != nullptr, m_log)) return false;

    m_memory = MapViewOfFile(
        /* hFileMappingObject = */ m_mapping,
        /* dwDesiredAccess = */ FILE_MAP_READ | FILE_MAP_WRITE,
        /* dwFileOffsetHigh = */ 0,
        /* dwFileOffsetLow = */ 0,
        /* dwNumberOfBytesToMap = */ m_size.as_bytes());

    if (!SM_CHECK_WIN32(m_memory != nullptr, m_log)) return false;

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
        m_log.error("public_size size must be a multiple of 8, {}", public_size);
        return false;
    }

    size_t as_bytes = public_size.as_bytes();

    // intialize allocation data
    m_used = header->used;
    m_capacity = header->count;

    // mark header region as used
    m_space.resize(as_bytes / 8);
    m_space.set_range(BitMap::Index(0), BitMap::Index(as_bytes / 8));

    // mark already used record data as used
    for (uint32_t i = 0; i < m_used; i++) {
        const RecordHeader *record = get_record_header(i);
        if (record->id == 0) continue;
        m_space.set_range(BitMap::Index(record->offset), BitMap::Index(record->offset + record->size));
    }

    return true;
}

void FileMapping::destroy() {
    // unmap the file from memory
    if (m_memory != nullptr) {
        // only update the header if the file is valid
        // we dont want to trample over a file that we didnt create
        if (is_valid()) update_header();

        SM_CHECK_WIN32(UnmapViewOfFile(m_memory), m_log);
        m_memory = nullptr;
    }

    // close the file mapping
    if (m_mapping != nullptr) {
        SM_CHECK_WIN32(CloseHandle(m_mapping), m_log);
        m_mapping = nullptr;
    }

    // close the file
    if (m_file != nullptr) {
        SM_CHECK_WIN32(CloseHandle(m_file), m_log);
        m_file = nullptr;
    }

    // reset the size
    m_size = 0;

    m_space.reset();
}

RecordLookup FileMapping::get_record(uint32_t id, void **data, uint16_t size) {
    for (uint32_t i = 0; i <= m_used; i++) {
        RecordHeader *record = get_record_header(i);
        if (record->id != id) continue;

        if (!SM_CHECKF(m_log, record->size == size, "record size mismatch, somebody didnt update the version number, {}/{}", record->size, size))
            return RecordLookup::eRecordInvalid;

        *data = get_record_data(record->offset);
        return RecordLookup::eOpened;
    }

    // no existing record found, create a new one
    if (!SM_CHECKF(m_log, m_capacity >= m_used, "record slots at capacity, cant create new record, {}/{}", m_used, m_capacity))
        return RecordLookup::eRecordTableExhausted;

    // find free record header
    volatile RecordHeader *found = nullptr;
    for (uint_fast16_t i = 0; i < m_capacity; i++) {
        RecordHeader *record = get_record_header(i);
        if (record->id == 0) {
            found = record;
            break;
        }
    }

    if (!SM_CHECKF(m_log, found != nullptr, "unable to find free record header, {}/{}", m_used, m_capacity))
        return RecordLookup::eRecordTableExhausted;

    // find free space

    // m_space operates on 8 byte chunks, convert the size and scan for free space
    uint16_t chunks = (size + 7) / 8;
    BitMap::Index index = m_space.scan_set_range(BitMap::Index{chunks});

    if (!SM_CHECKF(m_log, index != BitMap::Index::eInvalid, "unable to find free space, {:#08x} required, {:#08x} free total. defragment or make the file bigger", size, m_space.freecount()))
        return RecordLookup::eDataRegionExhausted;

    // write record header
    found->id = id;
    found->offset = uint32_t(index * 8);
    found->size = size;

    m_used += 1;

    // clear the data
    void *ptr = get_record_data(found->offset);
    std::memset(ptr, 0, size);
    *data = ptr;

    return RecordLookup::eCreated;
}

void FileMapping::init_alloc_info() {
    size_t public_size = get_public_size();

    // mark header region as used
    m_space.resize(public_size / 8);
    m_space.set_range(BitMap::Index(0), BitMap::Index(public_size / 8));

    // mark already used record data as used
    for (uint32_t i = 0; i < m_used; i++) {
        const RecordHeader *record = get_record_header(i);
        if (record->id == 0) continue;
        m_space.set_range(BitMap::Index(record->offset), BitMap::Index(record->offset + record->size));
    }
}

void FileMapping::update_header() {
    FileHeader *header = get_private_header();
    FileHeader it = {
        .magic = kFileMagic,
        .version = kCurrentVersion,
        .size = uint32_t(m_size.as_bytes()),
        .checksum = calc_checksum(),
        .count = header->count,
        .used = header->used,
    };

    std::memcpy(header, &it, sizeof(it));
}

void FileMapping::destroy_safe() {
    if (is_data_mapped()) destroy();
}

FileMapping::FileMapping(const MappingConfig& config)
    : m_path(config.path)
    , m_log(config.logger)
    , m_size(config.size.b() + (header_size(config.record_count)))
    , m_capacity(config.record_count)
{
    CTASSERT(m_path != nullptr);
    CTASSERT(m_size > 0);
    CTASSERT(m_capacity > 0);

    m_valid = create();
    if (m_valid) {
        m_log.info("file mapping created");
        m_log.info("| path: {}", m_path);
        m_log.info("| size: {}", m_size);
        m_log.info("| records: {}", m_capacity);
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
    header->size = uint32_t(m_size.as_bytes());
    header->checksum = 0;
    header->count = uint16_t(m_capacity);
    header->used = 0;

    // reset the space bitmap
    if (m_space.is_valid())
        m_space.reset();
    else
        m_space.resize(get_public_size() / 8);

    init_alloc_info();
}

uint32_t FileMapping::read_checksum() const {
    CTASSERT(is_data_mapped());

    FileHeader *header = get_private_header();
    return header->checksum;
}

uint32_t FileMapping::get_record_count() const {
    return m_capacity;
}

FileHeader *FileMapping::get_private_header() const {
    CTASSERT(is_data_mapped());

    return reinterpret_cast<FileHeader*>(get_private_region());
}

uint8_t *FileMapping::get_public_region() const {
    CTASSERT(is_data_mapped());

    return reinterpret_cast<uint8_t*>(m_memory) + header_size(get_record_count());
}

RecordHeader *FileMapping::get_record_header(uint32_t index) const {
    return reinterpret_cast<RecordHeader*>(get_private_region() + sizeof(FileHeader) + (index * sizeof(RecordHeader)));
}

uint8_t *FileMapping::get_private_region() const {
    CTASSERT(is_data_mapped());

    return reinterpret_cast<uint8_t*>(m_memory);
}

size_t FileMapping::get_public_size() const {
    CTASSERT(is_data_mapped());
    return m_size.as_bytes() - header_size(m_capacity);
}

void *FileMapping::get_record_data(uint32_t offset) const {
    return get_public_region() + offset;
}

uint32_t FileMapping::calc_checksum() const {
    return checksum(get_public_region(), get_public_size());
}
