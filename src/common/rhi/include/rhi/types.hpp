#pragma once

#include "core/win32.h" // IWYU pragma: export
#include <windef.h>
#include <winuser.h>
#include <winbase.h>

#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>

#include "core/macros.hpp"
#include "core/text.hpp"

#include <type_traits>

namespace sm::rhi {
template <typename T>
concept ComObject = std::is_base_of_v<IUnknown, T>;

template <typename T>
concept D3DObject = std::is_base_of_v<ID3D12Object, T>;

template <ComObject T>
class Object {
    T *m_object;

public:
    SM_NOCOPY(Object)

    constexpr Object(T *object = nullptr)
        : m_object(object) {}
    ~Object() {
        try_release();
    }

    constexpr Object(Object &&other) noexcept
        : m_object(other.m_object) {
        other.m_object = nullptr;
    }

    constexpr Object &operator=(Object &&other) noexcept {
        try_release();
        m_object = other.m_object;
        other.m_object = nullptr;
        return *this;
    }

    template <ComObject O>
    HRESULT query(O **out) const {
        return m_object->QueryInterface(IID_PPV_ARGS(out));
    }

    void release() {
        CTASSERT(is_valid());
        m_object->Release();
        m_object = nullptr;
    }

    T *acquire() {
        CTASSERT(is_valid());
        T *object = m_object;
        m_object = nullptr;
        return object;
    }

    bool try_release() {
        if (is_valid()) {
            release();
            return true;
        }
        return false;
    }

    constexpr T *operator->() const {
        CTASSERT(is_valid());
        return m_object;
    }
    constexpr T **operator&() {
        return &m_object;
    }
    constexpr T *get() const {
        CTASSERT(is_valid());
        return m_object;
    }
    constexpr T **get_address() {
        return &m_object;
    }

    constexpr bool is_valid() const {
        return m_object != nullptr;
    }
    constexpr operator bool() const {
        return is_valid();
    }

    void rename(std::string_view name)
        requires(D3DObject<T>)
    {
        CTASSERT(is_valid());
        m_object->SetName(sm::widen(name).c_str());
    }
};

using AdapterObject = Object<IDXGIAdapter1>;
using FactoryObject = Object<IDXGIFactory4>;
using CommandQueueObject = Object<ID3D12CommandQueue>;
using FenceObject = Object<ID3D12Fence>;
using CommandListObject = Object<ID3D12GraphicsCommandList2>;
using DeviceObject = Object<ID3D12Device1>;
using RootSignatureObject = Object<ID3D12RootSignature>;
using PipelineStateObject = Object<ID3D12PipelineState>;
using ResourceObject = Object<ID3D12Resource>;
using DescriptorHeapObject = Object<ID3D12DescriptorHeap>;
using AllocatorObject = Object<ID3D12CommandAllocator>;
} // namespace sm::rhi
