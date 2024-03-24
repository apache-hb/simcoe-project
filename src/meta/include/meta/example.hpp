#pragma once

#include "meta.hpp"

#include "example.reflect.h"

struct AAA {
    METHOD()
    void foo();
};

REFLECTED(name = "other")
class Example {
    PROPERTY(range = (0, 10))
    int mMember = 5;

    METHOD()
    void foo();

public:
    Example();
};
