#include "meta/info.hpp"

#include <limits>

using namespace sm::reflect;

using SmallSize = unsigned char;
static constexpr SmallSize kMaxSize = std::numeric_limits<SmallSize>::max();

template<typename T>
static consteval SmallSize getSizeOf() {
    constexpr size_t size = sizeof(T);
    static_assert(size <= kMaxSize, "Type is too large");
    return size;
}

template<typename T>
static consteval SmallSize getAlignOf() {
    constexpr size_t align = alignof(T);
    static_assert(align <= kMaxSize, "Type alignment is too large");
    return align;
}

static constexpr SmallSize kIntegerSizeOf[(int)IntegerWidth::eCount] = {
    [(int)IntegerWidth::eChar ] = getSizeOf<char>(),
    [(int)IntegerWidth::eShort] = getSizeOf<short>(),
    [(int)IntegerWidth::eInt  ] = getSizeOf<int>(),
    [(int)IntegerWidth::eLong ] = getSizeOf<long>(),
    [(int)IntegerWidth::eDiff ] = getSizeOf<ptrdiff_t>(),
    [(int)IntegerWidth::ePtr  ] = getSizeOf<intptr_t>(),
    [(int)IntegerWidth::eSize ] = getSizeOf<size_t>(),
};

static constexpr SmallSize kIntegerAlignOf[(int)IntegerWidth::eCount] = {
    [(int)IntegerWidth::eChar ] = getAlignOf<char>(),
    [(int)IntegerWidth::eShort] = getAlignOf<short>(),
    [(int)IntegerWidth::eInt  ] = getAlignOf<int>(),
    [(int)IntegerWidth::eLong ] = getAlignOf<long>(),
    [(int)IntegerWidth::eDiff ] = getAlignOf<ptrdiff_t>(),
    [(int)IntegerWidth::ePtr  ] = getAlignOf<intptr_t>(),
    [(int)IntegerWidth::eSize ] = getAlignOf<size_t>(),
};

size_t IntegerType::getSizeOf() const noexcept {
    return kIntegerSizeOf[(int)mWidth];
}

size_t IntegerType::getAlignOf() const noexcept {
    return kIntegerAlignOf[(int)mWidth];
}

static constexpr SmallSize kRealSizeOf[(int)RealWidth::eCount] = {
    [(int)RealWidth::eFloat ] = getSizeOf<float>(),
    [(int)RealWidth::eDouble] = getSizeOf<double>()
};

static constexpr SmallSize kRealAlignOf[(int)RealWidth::eCount] = {
    [(int)RealWidth::eFloat ] = getAlignOf<float>(),
    [(int)RealWidth::eDouble] = getAlignOf<double>()
};

size_t RealType::getSizeOf() const noexcept {
    return kRealSizeOf[(int)mWidth];
}

size_t RealType::getAlignOf() const noexcept {
    return kRealAlignOf[(int)mWidth];
}
