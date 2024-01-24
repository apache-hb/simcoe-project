#pragma once

#include "core/bitmap.hpp"

#include "system/system.hpp"

namespace sm::sys {
    // memory mapped file memory
    class FileMapping {
        // file path
        const char *m_path = nullptr;
        SystemSink m_log;

        // mapping data
        HANDLE m_file = nullptr;
        HANDLE m_mapping = nullptr;
        void *m_memory = nullptr;
        Memory m_size = 0;
        bool m_valid;

        // allocation info

        // bitmap of allocated space
        // is divided into 8 byte blocks starting from the user region
        // doesnt include the file or record headers
        sm::BitMap m_space;

        // number of records
        uint_fast16_t m_capacity = 0;

        // number of used records
        uint_fast16_t m_used = 0;

        void init_alloc_info();

        void update_header();

        bool create();
        void destroy();

        // region where records are placed
        uint8_t *get_public_region() const;
        size_t get_public_size() const;

        // file header region
        // includes base file header and record headers
        uint8_t *get_private_region() const;
        FileHeader *get_private_header() const;
        RecordHeader *get_record_header(uint32_t index) const;
        void *get_record_data(uint32_t offset) const;

        uint32_t get_record_count() const;

        // get the checksum from the file header
        uint32_t read_checksum() const;

        // calculate the checksum of the public region
        uint32_t calc_checksum() const;

        // lifetime management
        void destroy_safe();

        // get a record from the file
        // returns false if the record is not found
        // initializes the data buffer with the record data
        // memsets the data buffer to 0 if the record is not found
        RecordLookup get_record(uint32_t id, void **data, uint16_t size);

    public:
        SM_NOCOPY(FileMapping)
        SM_NOMOVE(FileMapping)

        /// @brief create a new file mapping
        ///
        /// @param path path to the file
        /// @param size size of the record data region
        /// @param count maximum number of records
        FileMapping(const MappingConfig& info);
        ~FileMapping();

        // erase all data in the file and reset the header
        // with the given record count
        void reset();

        bool is_data_mapped() const { return m_memory != nullptr; }

        // getters
        const char *get_path() const { return m_path; }
        bool is_valid() const { return m_valid && is_data_mapped(); }

        template<ctu::Reflected T>
        RecordLookup get_record(T **record) {
            CTASSERTF(is_data_mapped(), "file mapping not initialized");

            constexpr auto refl = ctu::reflect<T>();

            return get_record(refl.get_id(), (void**)record, sizeof(T));
        }
    };
}
