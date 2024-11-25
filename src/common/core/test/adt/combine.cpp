#include <gtest/gtest.h>

#include <vector>

#include "core/adt/combine.hpp"

namespace adt = sm::adt;

class BaseClass {
public:
    virtual ~BaseClass() = default;

    virtual int compute(int number) const { return number * 5; }
    virtual bool evaluate(const char *text) const = 0;

    virtual void mutate(int number) = 0;
    virtual int getInner() = 0;
};

class DerivedOne : public BaseClass {
    int mInner = 0;
public:
    int compute(int number) const override { return number * 10; }
    bool evaluate(const char *text) const override { return text != nullptr; }

    void mutate(int number) override { mInner = number; }
    int getInner() override { return mInner; }
};

class DerivedTwo : public BaseClass {
    std::vector<int> mValues;
    int mIndex = 0;
public:
    int compute(int number) const override { return number * 20; }
    bool evaluate(const char *text) const override { return text != nullptr && text[0] != '\0'; }

    void mutate(int number) override { mValues.push_back(number); }
    int getInner() override { return mValues[mIndex]; }
};

TEST(CombineTest, Inheritance) {
    adt::Combine<BaseClass, DerivedOne, DerivedTwo> combined{DerivedTwo{}};

    ASSERT_EQ(combined->compute(5), 100);
    ASSERT_TRUE(combined->evaluate("hello"));
    ASSERT_FALSE(combined->evaluate(nullptr));

    combined->mutate(10);
    ASSERT_EQ(combined->getInner(), 10);
}
