#include "system/io.hpp"

#include "common.hpp"

#include <cstdint>
#include <cstdio>

#include "core/reflect.hpp"

#include "system.reflect.h"

using namespace sm::system;

static_assert(sizeof(FileVersion) == sizeof(uint32_t));
static_assert(sizeof(FileHeader) == sizeof(uint32_t) * 4);

static constexpr uint32_t kFileMagic = 'CUM\0';
static constexpr FileVersion kCurrentVersion = FileVersion::eVersion0;

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
    auto init_file = [this] {
        FileHeader data = {
            .magic = kFileMagic,
            .version = kCurrentVersion,
            .size = uint32_t(m_size),
            .checksum = 0, // we calculate checksum at closing
        };

        write_header(data);
    };

    auto validate_file = [this]() -> bool {
        FileHeader *header = get_private_header();

        uint32_t checksum = calc_checksum();

        if (header->checksum != checksum) {
            std::printf("file checksum mismatch: (got = %08x, expected = %08x)\n", header->checksum, checksum);
            return false;
        }

        constexpr auto refl = ctu::reflect<FileVersion>();

        if (header->version != kCurrentVersion) {
            auto expected = refl.to_string(kCurrentVersion);
            auto actual = refl.to_string(header->version);
            std::printf("file version mismatch: (got = %s, expected = %s)\n", actual.data(), expected.data());
            return false;
        }

        uint32_t read_size = header->size;
        uint32_t expected_size = uint32_t(m_size);

        if (read_size != expected_size) {
            std::printf("file size mismatch: (got = %u, expected = %u)\n", read_size, expected_size);
            return false;
        }

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
        init_file();
        break;

    case kFileMagic: { // magic matches, validate file before use
        bool ok = validate_file();
        if (!ok) destroy();
        break;
    }

    default:
        // magic doesnt match, dont use it to avoid stomping someone
        // elses file
        std::printf("file magic mismatch: (got = %08x, expected = %08x)\n", header->magic, kFileMagic);
        destroy();
        break;
    }
}

void FileMapping::destroy() {
    // make sure we were already created
    CTASSERT(m_memory != nullptr);
    CTASSERT(m_mapping != nullptr);
    CTASSERT(m_file != nullptr);

    update_header();

    // unmap the file from memory
    SM_ASSERT_WIN32(UnmapViewOfFile(m_memory));
    m_memory = nullptr;

    // close the file mapping
    SM_ASSERT_WIN32(CloseHandle(m_mapping));
    m_mapping = nullptr;

    // close the file
    SM_ASSERT_WIN32(CloseHandle(m_file));
    m_file = nullptr;

    // reset the size
    m_size = 0;
}

FileHeader *FileMapping::get_private_header() const {
    CTASSERT(is_data_mapped());

    return reinterpret_cast<FileHeader*>(m_memory);
}

void FileMapping::write_header(const FileHeader& data) {
    CTASSERT(is_data_mapped());

    FileHeader *header = get_private_header();
    std::memcpy(header, &data, sizeof(data));
}

uint8_t *FileMapping::get_public_region() const {
    CTASSERT(is_data_mapped());

    return reinterpret_cast<uint8_t*>(m_memory) + sizeof(FileHeader);
}

void FileMapping::update_header() {
    FileHeader data = {
        .magic = kFileMagic,
        .version = kCurrentVersion,
        .size = uint32_t(m_size),
        .checksum = calc_checksum(),
    };

    write_header(data);
}

void FileMapping::destroy_safe() {
    if (is_data_mapped()) destroy();
}

void FileMapping::swap(FileMapping& other) {
    std::swap(m_path, other.m_path);
    std::swap(m_file, other.m_file);
    std::swap(m_mapping, other.m_mapping);
    std::swap(m_memory, other.m_memory);
    std::swap(m_size, other.m_size);
}

FileMapping::FileMapping(FileMapping&& other) {
    destroy_safe();

    swap(other);
}

FileMapping& FileMapping::operator=(FileMapping&& other) {
    destroy_safe();

    swap(other);

    return *this;
}

FileMapping::FileMapping(const char *path, size_t size)
    : m_path(path)
    , m_size(size)
{
    CTASSERT(m_path != nullptr);
    CTASSERT(m_size > 0);

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

uint32_t FileMapping::calc_checksum() const {
    CTASSERT(is_data_mapped());

    return checksum(get_public_region(), get_public_size());
}
