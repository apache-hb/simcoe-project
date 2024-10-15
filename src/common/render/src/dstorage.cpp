#include "stdafx.hpp"

#include "render/dstorage.hpp"

using namespace sm;
using namespace sm::render;

///
/// storage queue
///

Result StorageQueue::init(IDStorageFactory *factory, const DSTORAGE_QUEUE_DESC& desc) {
    if (Result hr = factory->CreateQueue(&desc, IID_PPV_ARGS(&mQueue)))
        return hr;

    return S_OK;
}

void StorageQueue::reset() {
    mQueue.reset();
}

void StorageQueue::enqueue(const DSTORAGE_REQUEST& request) {
    mQueue->EnqueueRequest(&request);
    mHasPendingRequests = true;
}

void StorageQueue::signal(ID3D12Fence *fence, uint64 value) {
    CTASSERTF(mHasPendingRequests, "Cannot enqueue a signal with no pending requests");
    mQueue->EnqueueSignal(fence, value);
}

void StorageQueue::submit() {
    mQueue->Submit();
    mHasPendingRequests = false;
}

static constexpr UINT32 getDirectStorageDebugFlags(DebugFlags flags) {
    UINT32 result = DSTORAGE_DEBUG_NONE;
    if (bool(flags & DebugFlags::eDirectStorageDebug)) {
        result |= DSTORAGE_DEBUG_SHOW_ERRORS;
    }
    if (bool(flags & DebugFlags::eDirectStorageBreak)) {
        result |= DSTORAGE_DEBUG_BREAK_ON_ERROR;
    }
    if (bool(flags & DebugFlags::eDirectStorageNames)) {
        result |= DSTORAGE_DEBUG_RECORD_OBJECT_NAMES;
    }
    return result;
}

///
/// storage factory
///

void CopyStorage::create(DebugFlags flags) {
    SM_ASSERT_HR(DStorageGetFactory(IID_PPV_ARGS(&mFactory)));

    mFactory->SetDebugFlags(getDirectStorageDebugFlags(flags));
}

void CopyStorage::destroy() noexcept {
    mFactory.reset();
}

StorageQueue CopyStorage::newQueue(const DSTORAGE_QUEUE_DESC& desc) {
    StorageQueue queue;
    SM_ASSERT_HR(queue.init(mFactory.get(), desc));
    return queue;
}

Result CopyStorage::open(const fs::path& path, IDStorageFile **file) {
    return mFactory->OpenFile(path.c_str(), IID_PPV_ARGS(file));
}

///
/// storage request builder
///

RequestBuilder& RequestBuilder::compression(DSTORAGE_COMPRESSION_FORMAT format, uint32 size) {
    Options.CompressionFormat = format;
    UncompressedSize = size;
    return *this;
}

RequestBuilder& RequestBuilder::src(void const *data, uint size) {
    DSTORAGE_SOURCE_MEMORY source = {
        .Source = data,
        .Size = size,
    };

    Options.SourceType = DSTORAGE_REQUEST_SOURCE_MEMORY;
    Source.Memory = source;
    return *this;
}

RequestBuilder& RequestBuilder::src(IDStorageFile *file, uint64 offset, uint size) {
    DSTORAGE_SOURCE_FILE source = {
        .Source = file,
        .Offset = offset,
        .Size = size,
    };

    Options.SourceType = DSTORAGE_REQUEST_SOURCE_FILE;
    Source.File = source;
    return *this;
}

RequestBuilder& RequestBuilder::dst(ID3D12Resource *resource, uint64 offset, uint size) {
    DSTORAGE_DESTINATION_BUFFER destination = {
        .Resource = resource,
        .Offset = offset,
        .Size = size,
    };

    Options.DestinationType = DSTORAGE_REQUEST_DESTINATION_BUFFER;
    Destination.Buffer = destination;
    return *this;
}

RequestBuilder& RequestBuilder::dst(DSTORAGE_DESTINATION_MULTIPLE_SUBRESOURCES it) {
    Options.DestinationType = DSTORAGE_REQUEST_DESTINATION_MULTIPLE_SUBRESOURCES;
    Destination.MultipleSubresources = it;
    return *this;
}

RequestBuilder& RequestBuilder::tag(uint64 tag) {
    CancellationTag = tag;
    return *this;
}

RequestBuilder& RequestBuilder::name(char const *name) {
    Name = name;
    return *this;
}
