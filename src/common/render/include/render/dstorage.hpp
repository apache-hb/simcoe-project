#pragma once

#include "render/object.hpp"

#include "render.reflect.h"

namespace sm::render {
    class StorageQueue {
        Object<IDStorageQueue2> mQueue;
        Object<IDStorageStatusArray> mStatusArray;

    public:
        Result init(IDStorageFactory *factory, const DSTORAGE_QUEUE_DESC& desc);
        void reset();

        void enqueue(const DSTORAGE_REQUEST& request);
        void signal(ID3D12Fence *fence, uint64 value);
        void submit();
    };

    class CopyStorage {
        Object<IDStorageFactory> mFactory;
        StorageQueue mFileQueue;
        StorageQueue mMemoryQueue;

    public:
        void create(DebugFlags flags);
        void destroy();

        void create_queues(ID3D12Device1 *device);
        void destroy_queues();

        Result open(const fs::path& path, IDStorageFile **file);

        void submit_file_copy(const DSTORAGE_REQUEST& request);
        void submit_memory_copy(const DSTORAGE_REQUEST& request);

        void signal_file_queue(ID3D12Fence *fence, uint64 value);
        void signal_memory_queue(ID3D12Fence *fence, uint64 value);

        void flush_queues();

        // TODO: check errors
    };

    struct RequestBuilder : DSTORAGE_REQUEST {
        RequestBuilder() {
            static_cast<DSTORAGE_REQUEST&>(*this) = {};
        }

        RequestBuilder& compression(DSTORAGE_COMPRESSION_FORMAT format, uint32 size) {
            Options.CompressionFormat = format;
            UncompressedSize = size;
            return *this;
        }

        RequestBuilder& src(void const *data, uint size) {
            DSTORAGE_SOURCE_MEMORY source = {
                .Source = data,
                .Size = size,
            };

            Options.SourceType = DSTORAGE_REQUEST_SOURCE_MEMORY;
            Source.Memory = source;
            return *this;
        }

        RequestBuilder& src(IDStorageFile *file, uint64 offset, uint size) {
            DSTORAGE_SOURCE_FILE source = {
                .Source = file,
                .Offset = offset,
                .Size = size,
            };

            Options.SourceType = DSTORAGE_REQUEST_SOURCE_FILE;
            Source.File = source;
            return *this;
        }

        RequestBuilder& dst(ID3D12Resource *resource, uint64 offset, uint size) {
            DSTORAGE_DESTINATION_BUFFER destination = {
                .Resource = resource,
                .Offset = offset,
                .Size = size,
            };

            Options.DestinationType = DSTORAGE_REQUEST_DESTINATION_BUFFER;
            Destination.Buffer = destination;
            return *this;
        }

        RequestBuilder& dst(DSTORAGE_DESTINATION_MULTIPLE_SUBRESOURCES it) {
            Options.DestinationType = DSTORAGE_REQUEST_DESTINATION_MULTIPLE_SUBRESOURCES;
            Destination.MultipleSubresources = it;
            return *this;
        }

        RequestBuilder& tag(uint64 tag) {
            CancellationTag = tag;
            return *this;
        }

        RequestBuilder& name(char const *name) {
            Name = name;
            return *this;
        }
    };
}
