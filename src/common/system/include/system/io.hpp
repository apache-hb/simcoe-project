#pragma once

#include <sm_system_api.hpp>

#include "core/win32.h" // IWYU pragma: export

#include "system.reflect.h"

namespace sm::system {
    // memory mapped file memory
    class SM_SYSTEM_API FileMapping {
        // file path
        const char *m_path = nullptr;

        // mapping data
        HANDLE m_file = nullptr;
        HANDLE m_mapping = nullptr;
        void *m_memory = nullptr;
        size_t m_size = 0;

        void create();
        void destroy();

        // write new header contents
        void update_header();

        // file header
        FileHeader *get_private_header() const;

        void write_header(const FileHeader& data);

        // user data region
        uint8_t *get_public_region() const;
        size_t get_public_size() const { return m_size - sizeof(FileHeader); }

        // get the checksum from the file header
        uint32_t read_checksum() const;

        // calculate the checksum of the public region
        uint32_t calc_checksum() const;

        // lifetime management
        void destroy_safe();
        void swap(FileMapping& other);

    public:
        SM_NOCOPY(FileMapping)

        FileMapping(FileMapping&& other);
        FileMapping& operator=(FileMapping&& other);

        FileMapping(const char *path, size_t size);
        ~FileMapping();

        bool has_checksum() const { return read_checksum() != 0; }
        bool is_data_mapped() const { return m_memory != nullptr; }
    };
}
