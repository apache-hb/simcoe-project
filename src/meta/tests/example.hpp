#pragma once

#include "meta/meta.hpp"

#include "example.reflect.h"

/// @brief Example class
///
/// Lorem ipsum dolor sit amet, consectetur adipiscing elit,
/// sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.
/// Ut enim ad minim veniam, quis nostrud exercitation ullamco
/// laboris nisi ut aliquip ex ea commodo consequat.
/// Duis aute irure dolor in reprehenderit in voluptate velit
/// esse cillum dolore eu fugiat nulla pariatur.
/// Excepteur sint occaecat cupidatat non proident,
/// sunt in culpa qui officia deserunt mollit anim id est laborum.
REFLECT("other")
class Example {
    PROPERTY(range = {0, 10})
    int mMember = 5;

    METHOD(name = "bar", threadsafe)
    void foo();

public:
    Example();
};

ENUM("options", bitflags)
enum class Options : unsigned {
    CASE(name = "option 1")
    eOption1 = (1 << 0),

    CASE(name = "option 2")
    eOption2 = (1 << 1),
};
