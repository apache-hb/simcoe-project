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

        void create(DebugFlags flags);
        void destroy();

        void create_queues(ID3D12Device1 *device);
        void destroy_queues();
    };
}
