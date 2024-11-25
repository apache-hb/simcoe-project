#include <gtest/gtest.h>

#include "core/macros.hpp"

namespace com::apache::simcoe {
    class Thing {

    };
}

struct SomeType;

TEST(SymbolNameTest, Simple) {
    ASSERT_EQ(sm::getTypeName<SomeType>(), "SomeType");
}

TEST(SymbolNameTest, HasNamespaces) {
    ASSERT_EQ(sm::getTypeName<com::apache::simcoe::Thing>(), "com.apache.simcoe.Thing");
}
