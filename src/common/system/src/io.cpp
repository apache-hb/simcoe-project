#include "system/io.hpp"

#include "common.hpp"

#include <cstdint>
#include <cstdio>
#include <cstring>

#include "system.reflect.h"

using namespace sm::sys;

static_assert(sizeof(FileHeader) == 20);
static_assert(sizeof(RecordHeader) == 8);

static constexpr uint32_t kFileMagic = 'CUM\0';
static constexpr FileVersion kCurrentVersion = FileVersion::eVersion0;

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

void FileMapping::create() {
    auto fail = [this](sys::MappingError error) {
        m_error = error;
        destroy();
    };

    // do first time initialization of file header
    auto setup_file = [&] {
        FileHeader data = {
            .magic = kFileMagic,
            .version = kCurrentVersion,
            .size = uint32_t(m_size),
            .checksum = 0, // we calculate checksum at closing
            .count = uint16_t(m_capacity),
            .used = uint16_t(m_used),
        };

        FileHeader *header = get_private_header();
        std::memcpy(header, &data, sizeof(data));
    };

    // we have an existing file, validate it
    auto validate_file = [this] {
        constexpr auto refl = ctu::reflect<FileVersion>();
        FileHeader *header = get_private_header();

        CTASSERTF(header->version == kCurrentVersion, "file version mismatch, %s/%s", refl.to_string(header->version).data(), refl.to_string(kCurrentVersion).data());

        uint32_t read_size = header->size;
        uint32_t expected_size = uint32_t(m_size);
        CTASSERTF(read_size == expected_size, "file size mismatch, %u/%u", read_size, expected_size);

        uint32_t checksum = calc_checksum();
        CTASSERTF(header->checksum == checksum, "file checksum mismatch, %08x/%08x", header->checksum, checksum);

        CTASSERTF(header->count == m_capacity, "file record count mismatch, %u/%u", header->count, m_capacity);
        CTASSERTF(header->used <= header->count, "file used count over limit, %u/%u", header->used, header->count);
    };

    m_file = CreateFileA(
        /* lpFileName = */ m_path,
        /* dwDesiredAccess = */ GENERIC_READ | GENERIC_WRITE,
        /* dwShareMode = */ FILE_SHARE_READ,
        /* lpSecurityAttributes = */ nullptr,
        /* dwCreationDisposition = */ OPEN_ALWAYS,
        /* dwFlagsAndAttributes = */ FILE_ATTRIBUTE_NORMAL,
        /* hTemplateFile = */ nullptr);

    SM_ASSERT_WIN32(m_file != INVALID_HANDLE_VALUE);

    // resize file to the desired size

    CTASSERTF(m_size <= UINT32_MAX, "file size too large: %zu", m_size);
    LARGE_INTEGER file_size = { .QuadPart = LONGLONG(m_size) };

    SM_ASSERT_WIN32(SetFilePointerEx(m_file, file_size, nullptr, FILE_BEGIN));
    SM_ASSERT_WIN32(SetEndOfFile(m_file));

    // map the file into memory

    m_mapping = CreateFileMappingA(
        /* hFile = */ m_file,
        /* lpFileMappingAttributes = */ nullptr,
        /* flProtect = */ PAGE_READWRITE,
        /* dwMaximumSizeHigh = */ 0,
        /* dwMaximumSizeLow = */ DWORD(m_size),
        /* lpName = */ nullptr);

    SM_ASSERT_WIN32(m_mapping != nullptr);

    m_memory = MapViewOfFile(
        /* hFileMappingObject = */ m_mapping,
        /* dwDesiredAccess = */ FILE_MAP_READ | FILE_MAP_WRITE,
        /* dwFileOffsetHigh = */ 0,
        /* dwFileOffsetLow = */ 0,
        /* dwNumberOfBytesToMap = */ m_size);

    SM_ASSERT_WIN32(m_memory != nullptr);

    // read in the file header

    FileHeader *header = get_private_header();
    switch (header->magic) {
    case 0: // no magic, assume new file
        setup_file();
        break;

    case kFileMagic: // magic matches, validate file before use
        validate_file();
        break;

    default: // magic doesnt match, dont use it to avoid stomping
        fail(MappingError::eInvalidMagic);
        return;
    }

    size_t public_size = get_public_size();
    CTASSERTF(public_size % 8 == 0, "public_size size must be a multiple of 8, %zu", public_size);

    // intialize allocation data
    m_used = header->used;
    m_capacity = header->count;

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

void FileMapping::destroy() {
    // unmap the file from memory
    if (m_memory != nullptr) {
        update_header();
        SM_ASSERT_WIN32(UnmapViewOfFile(m_memory));
        m_memory = nullptr;
    }

    // close the file mapping
    if (m_mapping != nullptr) {
        SM_ASSERT_WIN32(CloseHandle(m_mapping));
        m_mapping = nullptr;
    }

    // close the file
    if (m_file != nullptr) {
        SM_ASSERT_WIN32(CloseHandle(m_file));
        m_file = nullptr;
    }

    // reset the size
    m_size = 0;

    m_space.reset();
    m_error = MappingError::eOk;
}

bool FileMapping::get_record(uint32_t id, void **data, uint16_t size) {
    for (uint32_t i = 0; i <= m_used; i++) {
        RecordHeader *record = get_record_header(i);
        if (record->id != id) continue;

        CTASSERTF(record->size == size, "record size mismatch, somebody didnt update the version number, %hu/%hu", record->size, size);

        *data = get_record_data(record->offset);
        return true;
    }

    // no existing record found, create a new one
    CTASSERTF(m_capacity >= m_used, "record slots at capacity, cant create new record, %u/%u", m_used, m_capacity);

    // find free record header
    volatile RecordHeader *found = nullptr;
    for (uint16_t i = 0; i < m_capacity; i++) {
        RecordHeader *record = get_record_header(i);
        if (record->id == 0) {
            found = record;
            break;
        }
    }

    CTASSERTF(found != nullptr, "unable to find free record header, %u/%u", m_used, m_capacity);

    // find free space

    // m_space operates on 8 byte chunks, convert the size and scan for free space
    BitMap::Index index = m_space.scan_set_range((size + 7) / 8);

    CTASSERTF(index != BitMap::Index::eInvalid, "unable to find free space, %hu required, %zu free total. defragment or make the file bigger", size, m_space.freecount());

    // write record header
    found->id = id;
    found->offset = uint32_t(index.as_integral() * 8);
    found->size = size;

    m_used += 1;

    // clear the data
    void *ptr = get_record_data(found->offset);
    std::memset(ptr, 0, size);
    *data = ptr;

    return false;
}

void FileMapping::update_header() {
    FileHeader *header = get_private_header();
    FileHeader it = {
        .magic = kFileMagic,
        .version = kCurrentVersion,
        .size = uint32_t(m_size),
        .checksum = calc_checksum(),
        .count = header->count,
        .used = header->used,
    };

    std::memcpy(header, &it, sizeof(it));
}

void FileMapping::destroy_safe() {
    if (is_data_mapped()) destroy();
}

FileMapping::FileMapping(const char *path, size_t size, uint16_t count)
    : m_path(path)
    , m_size(size + (header_size(count)))
    , m_capacity(count)
{
    CTASSERT(m_path != nullptr);
    CTASSERT(m_size > 0);
    CTASSERT(m_capacity > 0);

    create();
}

FileMapping::~FileMapping() {
    destroy_safe();
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
    return m_size - header_size(m_capacity);
}

void *FileMapping::get_record_data(uint32_t offset) const {
    return get_public_region() + offset;
}

uint32_t FileMapping::calc_checksum() const {
    return checksum(get_public_region(), get_public_size());
}
