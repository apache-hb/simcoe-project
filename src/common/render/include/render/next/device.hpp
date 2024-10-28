#pragma once

#include "render/next/render.hpp"

#include "render/base/instance.hpp"

#include <D3D12MemAlloc.h>

#include <directx/d3dx12_root_signature.h>

namespace sm::render::next {
    class CoreDebugState {
        Object<ID3D12Debug1> mDebug;

        bool mDebugLayer = false;
        bool mGpuValidation = false;
        bool mAutoNamedObjects = false;
        bool mDeviceRemovedInfo = false;

        bool setupDebugLayer(bool enabled);

    public:
        CoreDebugState(DebugFlags flags) throws(RenderException);
    };

    class CoreDevice {
        FeatureLevel mFeatureLevel;
        Adapter *mAdapter;
        Object<ID3D12Device1> mDevice;

        Object<ID3D12InfoQueue1> mInfoQueue;
        DWORD mCookie = ULONG_MAX;

        D3D_ROOT_SIGNATURE_VERSION mRootSignatureVersion;

        void setupCoreDevice(Adapter& adapter, FeatureLevel level);
        void setupInfoQueue(bool enabled);

    public:
        CoreDevice(Adapter& adapter, FeatureLevel level, DebugFlags flags) throws(RenderException);

        Object<D3D12MA::Allocator> newAllocator(D3D12MA::ALLOCATOR_FLAGS flags) throws(RenderException);

        Object<ID3D12CommandQueue> newCommandQueue(D3D12_COMMAND_QUEUE_DESC desc) throws(RenderException);

        void reset() noexcept;

        bool setDeviceRemoved() noexcept;
        bool isDeviceRemoved() const noexcept;

        D3D_ROOT_SIGNATURE_VERSION rootSignatureVersion() const { return mRootSignatureVersion; }
        FeatureLevel level() const { return mFeatureLevel; }
        AdapterLUID luid() const { return mAdapter->luid(); }
        ID3D12Device1 *get() const { return mDevice.get(); }

        ID3D12Device1 *operator->() const { return mDevice.get(); }
    };

    HRESULT serializeRootSignature(const CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC *desc, const CoreDevice& device, ID3DBlob **blob, ID3DBlob **error);
    Blob rootSignatureToBlob(const CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC& desc, const CoreDevice& device);
    render::Object<ID3D12RootSignature> createRootSignature(const CoreDevice& device, const CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC& desc);
}
