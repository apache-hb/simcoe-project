#pragma once

#include "archive/bundle.hpp"

#include "core/bitmap.hpp"

#include "os/os.h"

#include "record.reflect.h"

namespace sm::archive {
    // memory mapped file
    class RecordStore {
        // file path
        const char *mFilePath = nullptr;

        // mapping data
        os_file_t mFileHandle;
        os_mapping_t mMapHandle;

        void *mMemory = nullptr;
        sm::Memory mSize = 0;
        bool mValid;

        // allocation info

        // bitmap of allocated space
        // is divided into 8 byte blocks starting from the user region
        // doesnt include the file or record headers
        sm::BitMap mAllocator;

        // number of records
        uint_fast16_t mCapacity = 0;

        // number of used records
        uint_fast16_t mUsed = 0;

        bool check(bool expr, std::string_view fmt, auto&&... args) const;

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
        RecordStoreHeader *get_private_header() const;
        RecordEntryHeader *get_record_header(uint32_t index) const;
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
        RecordStore(const RecordStoreConfig& info);
        ~RecordStore();

        // erase all data in the file and reset the header
        // with the given record count
        void reset();

        bool is_data_mapped() const { return mMemory != nullptr; }

        // getters
        const char *get_path() const { return mFilePath; }
        bool is_valid() const { return mValid && is_data_mapped(); }

        template<ctu::Reflected T>
        RecordLookup get_record(T **record) {
            CTASSERTF(is_data_mapped(), "file mapping not initialized");

            constexpr auto refl = ctu::reflect<T>();

            return get_record(refl.get_id(), (void**)record, sizeof(T));
        }
    };
}
