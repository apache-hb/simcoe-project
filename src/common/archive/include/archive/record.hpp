#pragma once

#include "core/allocators/bitmap_allocator.hpp"

#include "os/os.h"

#include "record.reflect.h"

namespace sm::archive {
    // memory mapped file
    class RecordStore final {
        // file path
        const char *mFilePath = nullptr;

        // mapping data
        os_file_t mFileHandle;
        os_mapping_t mMapHandle;

        void *mMemory = nullptr;
        sm::Memory mSize = 0;
        bool mValid;

        // number of records
        uint_fast16_t mCapacity = 0;

        // number of used records
        uint_fast16_t mUsed = 0;

        // bitmap of allocated space
        // is divided into 8 byte blocks starting from the user region
        // doesnt include the file or record headers
        sm::BitMapIndexAllocator mIndexAllocator;


        void init_alloc_info();
        void writeNewHeader();

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

        uint32_t getRecordCount() const;

        // get the checksum from the file header
        uint32_t getChecksum() const;

        // calculate the checksum of the public region
        uint32_t computeNewChecksum() const;

        // lifetime management
        void destroy_safe();

        // get a record from the file
        // returns false if the record is not found
        // initializes the data buffer with the record data
        // memsets the data buffer to 0 if the record is not found
        RecordLookup getRecordImpl(uint32_t id, void **data, uint16_t size);

    public:
        RecordStore(const RecordStoreConfig& info);
        ~RecordStore();

        // erase all data in the file and reset the header
        // with the given record count
        void reset();

        bool isDataMapped() const { return mMemory != nullptr; }

        // getters
        const char *getPath() const { return mFilePath; }
        bool isValid() const { return mValid && isDataMapped(); }

        template<ctu::Reflected T>
        RecordLookup getRecord(T **record) {
            CTASSERTF(isDataMapped(), "file mapping not initialized");

            constexpr auto refl = ctu::reflect<T>();

            return getRecordImpl(refl.get_id(), (void**)record, sizeof(T));
        }
    };
}
