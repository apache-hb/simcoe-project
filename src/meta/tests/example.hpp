#pragma once

#include "meta/meta.hpp"

#include "example.reflect.h"

INTERFACE()
class IExample {
    REFLECT_BODY(IExample);

public:
    virtual ~IExample() = default;

    METHOD(name = "bar", threadsafe)
    virtual void foo() = 0;
};

REFLECT()
class Stub {
    REFLECT_BODY(Stub);

    PROPERTY(name = "range", range = {0, 10})
    int mRange = 5;

public:
    Stub();
};

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
