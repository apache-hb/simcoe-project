#include "stdafx.hpp"

#include "render/descriptor_heap.hpp"

namespace render = sm::render;
namespace next = sm::render::next;
using DescriptorPoolBase = next::DescriptorPoolBase;

static ID3D12DescriptorHeap *newDescriptorHeap(ID3D12Device1 *device, D3D12_DESCRIPTOR_HEAP_DESC desc) {
    ID3D12DescriptorHeap *heap;
    SM_ASSERT_HR(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap)));
    return heap;
}

DescriptorPoolBase::DescriptorPoolBase(ID3D12Device1 *device, D3D12_DESCRIPTOR_HEAP_DESC desc)
    : DescriptorPoolBase(newDescriptorHeap(device, desc), device->GetDescriptorHandleIncrementSize(desc.Type), desc.Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
{ }
