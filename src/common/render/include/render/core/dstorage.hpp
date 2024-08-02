#pragma once

#include "render/core/object.hpp"

#include "render.reflect.h"

namespace sm::render {
    class StorageQueue {
        Object<IDStorageQueue2> mQueue;

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

        // TODO: check errors
    };

    class CopyStorage {
        Object<IDStorageFactory> mFactory;

    public:
        void create(DebugFlags flags);
        void destroy();

        StorageQueue newQueue(const DSTORAGE_QUEUE_DESC& desc);

        Result open(const fs::path& path, IDStorageFile **file);

        void flush_queues();
    };

    struct RequestBuilder : DSTORAGE_REQUEST {
        RequestBuilder()
            : DSTORAGE_REQUEST()
        { }

        RequestBuilder& compression(DSTORAGE_COMPRESSION_FORMAT format, uint32 size);

        RequestBuilder& src(void const *data, uint size);
        RequestBuilder& src(IDStorageFile *file, uint64 offset, uint size);

        RequestBuilder& dst(ID3D12Resource *resource, uint64 offset, uint size);
        RequestBuilder& dst(DSTORAGE_DESTINATION_MULTIPLE_SUBRESOURCES it);

        RequestBuilder& tag(uint64 tag);

        RequestBuilder& name(char const *name);
    };
}
