#pragma once

#include "render/object.hpp"

#include "render.reflect.h"

namespace sm::render {
    class StorageQueue {
        Object<IDStorageQueue2> mQueue;
        Object<IDStorageStatusArray> mStatusArray;

        // TODO: this is perhaps a bit hacky
        // prevents us from submitting fence waits if there are no pending requests
        bool mHasPendingRequests = false;

    public:
        Result init(IDStorageFactory *factory, const DSTORAGE_QUEUE_DESC& desc);
        void reset();

        void enqueue(const DSTORAGE_REQUEST& request);
        void signal(ID3D12Fence *fence, uint64 value);
        void submit();

        bool hasPendingRequests() const { return mHasPendingRequests; }
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

        StorageQueue newQueue(const DSTORAGE_QUEUE_DESC& desc);

        StorageQueue& file_queue() { return mFileQueue; }
        StorageQueue& memory_queue() { return mMemoryQueue; }

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

        RequestBuilder& compression(DSTORAGE_COMPRESSION_FORMAT format, uint32 size);

        RequestBuilder& src(void const *data, uint size);
        RequestBuilder& src(IDStorageFile *file, uint64 offset, uint size);

        RequestBuilder& dst(ID3D12Resource *resource, uint64 offset, uint size);
        RequestBuilder& dst(DSTORAGE_DESTINATION_MULTIPLE_SUBRESOURCES it);

        RequestBuilder& tag(uint64 tag);

        RequestBuilder& name(char const *name);
    };
}
