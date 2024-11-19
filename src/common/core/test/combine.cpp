#include "test/common.hpp"

#include "core/combine.hpp"

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

TEST_CASE("Combining inheritance") {
    sm::Combine<BaseClass, DerivedOne, DerivedTwo> combined{DerivedTwo{}};

    THEN("The combined object uses the correct behavior") {
        REQUIRE(combined->compute(5) == 100);
        REQUIRE(combined->evaluate("hello"));
        REQUIRE_FALSE(combined->evaluate(nullptr));

        combined->mutate(10);
        REQUIRE(combined->getInner() == 10);
    }
}
