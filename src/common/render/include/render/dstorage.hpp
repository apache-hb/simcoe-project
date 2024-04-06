#pragma once

#include "render/object.hpp"

#include "render.reflect.h"

namespace sm::render {
    class CopyStorage {
        Object<IDStorageFactory> mFactory;
        Object<IDStorageQueue2> mFileQueue;
        Object<IDStorageQueue2> mMemoryQueue;

    public:
        CopyStorage();
        ~CopyStorage();

        void create(DebugFlags flags);
        void destroy();

        void create_queues(ID3D12Device1 *device);
        void destroy_queues();

        Result open(const fs::path& path, IDStorageFile **file);

        void submit_file_copy(const DSTORAGE_REQUEST& request);
        void submit_memory_copy(const DSTORAGE_REQUEST& request);

        void signal_file_queue(ID3D12Fence *fence, uint64 value);
        void signal_memory_queue(ID3D12Fence *fence, uint64 value);
    };

    struct RequestBuilder : DSTORAGE_REQUEST {
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
