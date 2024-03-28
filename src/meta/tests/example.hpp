#pragma once

#include "meta/meta.hpp"

#include "example.reflect.h"

class Stub {};

REFLECT("other")
class Example : Stub {
    REFLECT_BODY(Example);

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
