#include "draw/next/vic20.hpp"

#include <directx/d3dx12_barriers.h>

using namespace sm;
using namespace sm::render::next;
using namespace sm::draw::next;

Vic20DrawContext::Vic20DrawContext(render::next::ContextConfig config, system::Window& window, math::uint2 resolution)
    : Super(config, window)
    , mVic20(addResource<Vic20Display>(math::uint2 { VIC20_SCREEN_WIDTH, VIC20_SCREEN_HEIGHT }))
    , mBlit(addResource<BlitPass>(resolution, DXGI_FORMAT_R8G8B8A8_UNORM))
{
    mVic20->create();
    mBlit->create();
}

void Vic20DrawContext::begin() {
    CommandBufferSet& commands = *mDirectCommandSet;

    ID3D12DescriptorHeap *heaps[] = { mSrvHeap->get() };

    mDirectCommandSet->reset(mCurrentBackBuffer);

    commands->SetDescriptorHeaps(_countof(heaps), heaps);

    mImGui->begin();
}

void Vic20DrawContext::end() {
    mCompute->begin(mCurrentBackBuffer);
    mVic20->record(mCompute->getCommandList(), mCurrentBackBuffer);
    mCompute->end();

    // block the direct queue until all compute work is done
    mCompute->blockQueueUntil(mDirectQueue.get());

    ID3D12Resource *vic20 = mVic20->getTarget();
    ID3D12Resource *blit = mBlit->getTarget();
    ID3D12Resource *surface = surfaceAt(mCurrentBackBuffer);

    const D3D12_RESOURCE_BARRIER cIntoBlit[] = {
        CD3DX12_RESOURCE_BARRIER::Transition(
            vic20,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
        ),
        CD3DX12_RESOURCE_BARRIER::Transition(
            blit,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            D3D12_RESOURCE_STATE_RENDER_TARGET
        )
    };

    const D3D12_RESOURCE_BARRIER cIntoImGui[] = {
        CD3DX12_RESOURCE_BARRIER::Transition(
            vic20,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS
        ),
        CD3DX12_RESOURCE_BARRIER::Transition(
            blit,
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
        ),
        CD3DX12_RESOURCE_BARRIER::Transition(
            surface,
            D3D12_RESOURCE_STATE_PRESENT,
            D3D12_RESOURCE_STATE_RENDER_TARGET
        )
    };

    ID3D12GraphicsCommandList *list = mDirectCommandSet->get();

    list->ResourceBarrier(_countof(cIntoBlit), cIntoBlit);

    if (render::Object<ID3D12DebugCommandList> debug; SUCCEEDED(mDirectCommandSet->get()->QueryInterface(&debug))) {
        debug->AssertResourceState(mBlit->getTarget(), 0, D3D12_RESOURCE_STATE_RENDER_TARGET);
    }

    mBlit->record(list, mCurrentBackBuffer, mVic20->getTargetSrv());

    list->ResourceBarrier(_countof(cIntoImGui), cIntoImGui);

    if (render::Object<ID3D12DebugCommandList> debug; SUCCEEDED(mDirectCommandSet->get()->QueryInterface(&debug))) {
        debug->AssertResourceState(mBlit->getTarget(), 0, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }

    const D3D12_CPU_DESCRIPTOR_HANDLE cRtvHandle = { rtvHandleAt(mCurrentBackBuffer) };

    list->OMSetRenderTargets(1, &cRtvHandle, FALSE, nullptr);

    list->ClearRenderTargetView(cRtvHandle, mSwapChainInfo.clear.data(), 0, nullptr);

    mImGui->end(list);

    endCommands();
    executeCommands();

    mCompute->waitOnQueue(mDirectQueue.get());

    mImGui->updatePlatformViewports(list);

    presentSurface();
}
