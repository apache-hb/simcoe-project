#pragma once

#include "core/error/error.hpp"
#include "core/win32.hpp"
#include "core/error.hpp"

namespace sm::render {
    std::string errorString(HRESULT hr);

    class RenderError final : public errors::Error<RenderError> {
        using Super = errors::Error<RenderError>;

        HRESULT mValue;

    public:
        using Exception = class RenderException;

        RenderError(HRESULT value);

        RenderError(HRESULT value, std::string_view expr);

        template<typename... A>
        RenderError(HRESULT value, fmt::format_string<A...> fmt, A&&... args)
            : RenderError{value, fmt::format(fmt, std::forward<A>(args)...)}
        { }

        HRESULT value() const { return mValue; }
        bool isSuccess() const { return SUCCEEDED(mValue); }
    };

    class RenderException final : public errors::Exception<RenderError> {
        using Super = errors::Exception<RenderError>;
    public:
        using Super::Super;
    };

    template<typename V>
    using RenderResult = RenderError::Result<V>;
}

#define SM_THROW_HR(expr) \
    do { \
        if (HRESULT hr = (expr); FAILED(hr)) { \
            throw sm::render::RenderException{hr, #expr}; \
        } \
    } while (false)
