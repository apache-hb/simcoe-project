#pragma once

namespace sm::render::next {
    class RenderException;

    class RenderError {
        HRESULT mValue;

    public:
        RenderError(HRESULT value)
            : mValue(value)
        { }

        [[noreturn]]
        void raise() const throws(RenderException);

        void throwIfFailed() const throws(RenderException);
    };

    class RenderException : public std::exception {
        RenderError mError;

    public:
        RenderException(RenderError error)
            : mError(error)
        { }

        RenderError error() const noexcept { return mError; }
    };
}
