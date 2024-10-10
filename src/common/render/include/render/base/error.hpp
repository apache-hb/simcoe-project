#pragma once

#include "core/error/error.hpp"
#include "core/win32.hpp"
#include "core/error.hpp"

namespace sm::render {
    class RenderException;
    class RenderError;

    std::string errorString(HRESULT hr);

    class RenderError final : public errors::Error<RenderError> {
        HRESULT mValue;

        using Super = errors::Error<RenderError>;
    public:
        using Exception = RenderException;

        RenderError(HRESULT value)
            : Super(errorString(value), FAILED(value))
            , mValue(value)
        { }

        HRESULT value() const { return mValue; }
        bool isSuccess() const { return SUCCEEDED(mValue); }
    };

    class RenderException final : public errors::Exception<RenderError> {
        using Super = errors::Exception<RenderError>;
    public:
        using Super::Super;
    };
}
