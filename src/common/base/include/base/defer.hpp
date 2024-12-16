#pragma once

enum class DeferTag { eTag };
enum class ErrorDeferTag { eTag };

template<typename F>
constexpr auto operator+(DeferTag, F fn) {
    struct Inner {
        F fn;
        ~Inner() noexcept { fn(); }
    };

    return Inner{fn};
}

template<typename F>
constexpr auto operator+(ErrorDeferTag, F fn) {
    struct Inner {
        F fn;
        ~Inner() noexcept { if (std::uncaught_exceptions() > 0) { fn(); } }
    };

    return Inner{fn};
}

#define CONCAT2(a, b) a##b
#define CONCAT(a, b) CONCAT2(a, b)
#define EXPAND(x) x

#define defer const auto CONCAT(defer, __COUNTER__) = DeferTag::eTag + [&] ()

#define errdefer const auto CONCAT(errdefer, __COUNTER__) = ErrorDeferTag::eTag + [&] ()
