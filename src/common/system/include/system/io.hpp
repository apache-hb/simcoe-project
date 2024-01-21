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

        // get the checksum from the file header
        uint32_t read_checksum() const;

        // calculate the checksum of the public region
        uint32_t calc_checksum() const;

        // lifetime management
        void destroy_safe();
        void swap(FileMapping& other);

        // get a record from the file
        // returns false if the record is not found
        // initializes the data buffer with the record data
        // memsets the data buffer to 0 if the record is not found
        bool get_record(uint32_t id, void *data, size_t size);

    public:
        SM_NOCOPY(FileMapping)

        FileMapping(FileMapping&& other);
        FileMapping& operator=(FileMapping&& other);

        FileMapping(const char *path, size_t size);
        ~FileMapping();

        bool has_checksum() const { return read_checksum() != 0; }
        bool is_data_mapped() const { return m_memory != nullptr; }

        // user data region
        // these apis are pretty raw access, you probably want to access
        // via the reflection api instead
        uint8_t *get_public_region() const;
        size_t get_public_size() const { return m_size - sizeof(FileHeader); }

        // file path
        const char *get_path() const { return m_path; }

        template<typename T>
        bool get_record(T& record) const {
            CTASSERTF(is_data_mapped(), "file mapping not initialized");

            constexpr auto refl = ctu::reflect<T>();
            constexpr auto id = refl.id.get_id();

            return get_record(id, &record, sizeof(record));
        }
    };
}
