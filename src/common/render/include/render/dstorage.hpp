#pragma once

#include "core/library.hpp"

#include "render/object.hpp"

#include "render.reflect.h"

namespace sm::render {
    class CopyStorage {
        using GetFactoryFn = decltype(&DStorageGetFactory);

        Library mCoreLibrary;
        Library mLibrary;
        GetFactoryFn mGetFactory;

        Object<IDStorageFactory> mFactory;
        Object<IDStorageQueue2> mQueue;

    public:
        CopyStorage();

        void create(DebugFlags flags);
        void destroy();

        void create_queue(ID3D12Device1 *device);
        void destroy_queue();
    };
}
