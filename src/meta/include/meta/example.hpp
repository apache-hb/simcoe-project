#pragma once

#include "meta.hpp"

reflect(name = "other")
class Example {
    property(range = (0, 10))
    int mMember = 5;

public:
    Example();
};

class Example2 {
    property(range = (0, 10))
    int mMember = 5;
};
