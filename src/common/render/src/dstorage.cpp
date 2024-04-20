#include "stdafx.hpp"

#include "render/render.hpp"

using namespace sm;
using namespace sm::render;

///
/// storage queue
///

Result StorageQueue::init(IDStorageFactory *factory, const DSTORAGE_QUEUE_DESC& desc) {
    if (Result hr = factory->CreateQueue(&desc, IID_PPV_ARGS(&mQueue)))
        return hr;

    if (Result hr = factory->CreateStatusArray(4, desc.Name, IID_PPV_ARGS(&mStatusArray)))
        return hr;

    return S_OK;
}

void StorageQueue::reset() {
    mQueue.reset();
    mStatusArray.reset();
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

///
/// storage factory
///

void CopyStorage::create(DebugFlags flags) {
    SM_ASSERT_HR(DStorageGetFactory(IID_PPV_ARGS(&mFactory)));

    mFactory->SetDebugFlags(getDirectStorageDebugFlags(flags));
}

void CopyStorage::destroy() {
    mFactory.reset();
}

StorageQueue CopyStorage::newQueue(const DSTORAGE_QUEUE_DESC& desc) {
    StorageQueue queue;
    SM_ASSERT_HR(queue.init(mFactory.get(), desc));
    return queue;
}

void CopyStorage::create_queues(ID3D12Device1 *device) {
    mMemoryQueue = newQueue({
        .SourceType = DSTORAGE_REQUEST_SOURCE_MEMORY,
        .Capacity = DSTORAGE_MAX_QUEUE_CAPACITY,
        .Priority = DSTORAGE_PRIORITY_NORMAL,
        .Name = "host -> device",
        .Device = device,
    });

    mFileQueue = newQueue({
        .SourceType = DSTORAGE_REQUEST_SOURCE_FILE,
        .Capacity = DSTORAGE_MAX_QUEUE_CAPACITY,
        .Priority = DSTORAGE_PRIORITY_NORMAL,
        .Name = "disk -> device",
        .Device = device,
    });
}

void CopyStorage::destroy_queues() {
    mMemoryQueue.reset();
    mFileQueue.reset();
}

Result CopyStorage::open(const fs::path& path, IDStorageFile **file) {
    return mFactory->OpenFile(path.c_str(), IID_PPV_ARGS(file));
}

void CopyStorage::submit_file_copy(const DSTORAGE_REQUEST& request) {
    mFileQueue.enqueue(request);
}

void CopyStorage::submit_memory_copy(const DSTORAGE_REQUEST& request) {
    mMemoryQueue.enqueue(request);
}

void CopyStorage::signal_file_queue(ID3D12Fence *fence, uint64 value) {
    mFileQueue.signal(fence, value);
}

void CopyStorage::signal_memory_queue(ID3D12Fence *fence, uint64 value) {
    mMemoryQueue.signal(fence, value);
}

void CopyStorage::flush_queues() {
    mFileQueue.submit();
    mMemoryQueue.submit();
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

///
/// storage context stuff
/// TODO: move this out of here
///

void Context::create_dstorage() {
    mStorage.create(mDebugFlags);
    mStorage.create_queues(mDevice.get());
}

void Context::destroy_dstorage() {
    mStorage.destroy_queues();
    mStorage.destroy();
}

IDStorageFile *Context::get_storage_file(world::IndexOf<world::File> index) {
    if (auto it = mStorageFiles.find(index); it != mStorageFiles.end()) {
        return it->second.get();
    }

    const auto& file = mWorld.get(index);

    Object<IDStorageFile> dsfile;
    SM_ASSERT_HR(mStorage.open(file.path, &dsfile));

    auto [it, _] = mStorageFiles.emplace(index, std::move(dsfile));
    auto& [_, fd] = *it;

    return fd.get();
}

const uint8 *Context::get_storage_buffer(world::IndexOf<world::Buffer> index) {
    auto& buffer = mWorld.get(index);

    return buffer.data.data();
}
