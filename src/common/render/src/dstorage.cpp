#include "render/render.hpp"

#include "stdafx.hpp"

using namespace sm;
using namespace sm::render;

using GetFactoryFn = decltype(&DStorageGetFactory);

static constexpr UINT32 get_dstorage_flags(DebugFlags flags) {
    UINT32 result = DSTORAGE_DEBUG_NONE;
    if (flags.test(DebugFlags::eDirectStorageDebug)) {
        result |= DSTORAGE_DEBUG_SHOW_ERRORS;
    }
    if (flags.test(DebugFlags::eDirectStorageBreak)) {
        result |= DSTORAGE_DEBUG_BREAK_ON_ERROR;
    }
    if (flags.test(DebugFlags::eDirectStorageNames)) {
        result |= DSTORAGE_DEBUG_RECORD_OBJECT_NAMES;
    }
    return result;
}

CopyStorage::CopyStorage()
    : mCoreLibrary(sys::get_redist("dstorage/dstoragecore.dll"))
    , mLibrary(sys::get_redist("dstorage/dstorage.dll"))
{
    auto err = mLibrary.get_error();
    SM_ASSERTF(err.success(), "Failed to load DirectStorage redist: {}", err);

    mGetFactory = mLibrary.fn<GetFactoryFn>("DStorageGetFactory");
    SM_ASSERTF(mGetFactory, "Failed to find DStorageGetFactory in DirectStorage redist");
}

void CopyStorage::create(DebugFlags flags) {
    SM_ASSERT_HR(mGetFactory(IID_PPV_ARGS(&mFactory)));

    mFactory->SetDebugFlags(get_dstorage_flags(flags));
}

void CopyStorage::destroy() {
    mFactory.reset();
}

void CopyStorage::create_queue(ID3D12Device1 *device) {
    const DSTORAGE_QUEUE_DESC desc = {
        .SourceType = DSTORAGE_REQUEST_SOURCE_MEMORY,
        .Capacity = DSTORAGE_MAX_QUEUE_CAPACITY,
        .Priority = DSTORAGE_PRIORITY_NORMAL,
        .Name = "Memory -> Video Queue",
        .Device = device,
    };

    SM_ASSERT_HR(mFactory->CreateQueue(&desc, IID_PPV_ARGS(&mQueue)));
}

void CopyStorage::destroy_queue() {
    mQueue.reset();
}

void Context::create_dstorage() {
    mStorage.create(mDebugFlags);
    mStorage.create_queue(mDevice.get());
}

void Context::destroy_dstorage() {
    mStorage.destroy_queue();
    mStorage.destroy();
}
