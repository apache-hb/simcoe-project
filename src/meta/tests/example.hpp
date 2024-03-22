#pragma once

#include "meta/meta.hpp"

class reflect(name = "example") Example {
    reflect(range = (0, 10))
    int x = 5;
public:
    Example();
};
