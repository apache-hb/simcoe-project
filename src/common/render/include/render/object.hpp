#pragma once

#include "core/unique.hpp"
#include "core/text.hpp"

#include "render/result.hpp"

#include <d3d12.h>

namespace sm::render {

template <typename T>
concept ComObject = std::is_base_of_v<IUnknown, T>;

template <typename T>
concept D3DObject = std::is_base_of_v<ID3D12Object, T>;

constexpr auto kComRelease = [](IUnknown *object) {
    object->Release();
};

template <ComObject T>
class Object : public sm::UniquePtr<T, decltype(kComRelease)> {
    using Super = sm::UniquePtr<T, decltype(kComRelease)>;
public:
    using Super::Super;

    template <ComObject O>
    Result query(O **out) const {
        return Super::get()->QueryInterface(IID_PPV_ARGS(out));
    }

    void rename(std::string_view name) requires(D3DObject<T>) {
        Super::get()->SetName(sm::widen(name).c_str());
    }
};
} // namespace sm::render