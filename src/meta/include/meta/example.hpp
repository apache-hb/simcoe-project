#pragma once

#include "meta.hpp"

#include reflect_header("example.reflect.hpp")

class reflect(name = "other") Example {
    reflect_impl()

    reflect(range = (0, 10))
    int mMember = 5;
public:
    Example();
};
