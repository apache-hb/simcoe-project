#pragma once

#include "core/adt/range.hpp"
#include "core/adt/small_string.hpp"

namespace sm {
    template<std::integral T>
    constexpr size_t zstrlen(const T *str) {
        size_t length = 0;
        while (*str++)
            ++length;
        return length;
    }

    template<std::integral T>
    class ZStringViewBase final : public detail::PointerRange<const T> {
        using Super = detail::PointerRange<const T>;

        static constexpr inline T kEmptyString[] = { T(0) };

        constexpr void verifyZString() const noexcept {
            CTASSERTF(this->getFront() <= this->getBack(), "mFront is greater than mBack");
            CTASSERTF(*this->getBack() == T(0), "mBack is not null terminated");
        }

    public:
        using String = std::basic_string<T>;
        using View = std::basic_string_view<T>;

        constexpr ZStringViewBase(const T *front, const T *back) noexcept
            : Super(front, back)
        {
            verifyZString();
        }

        constexpr ZStringViewBase() noexcept
            : ZStringViewBase(kEmptyString)
        { }

        constexpr ZStringViewBase(const T *front, size_t length) noexcept
            : ZStringViewBase(front, front + length)
        { }

        template<size_t N>
        constexpr ZStringViewBase(T (&str)[N]) noexcept
            : ZStringViewBase(str, N)
        { }

        constexpr ZStringViewBase(const T *front) noexcept
            : ZStringViewBase(front, front + zstrlen(front))
        { }

        constexpr ZStringViewBase(const std::basic_string<T> &str) noexcept
            : ZStringViewBase(str.c_str(), str.c_str() + str.size())
        { }

        template<size_t N>
        constexpr ZStringViewBase(const SmallString<N>& str) noexcept
            : ZStringViewBase(str.data(), str.data() + str.size())
        { }

        constexpr auto *c_str(this auto& self) noexcept { return self.getFront(); }
        constexpr size_t length() const noexcept { return Super::size(); }

        constexpr std::strong_ordering operator<=>(const ZStringViewBase& other) const noexcept {
            return std::lexicographical_compare_three_way(this->cbegin(), this->cend(), other.cbegin(), other.cend());
        }

        constexpr bool operator==(const ZStringViewBase& other) const noexcept {
            return std::equal(this->cbegin(), this->cend(), other.cbegin(), other.cend());
        }

        constexpr bool startsWith(const ZStringViewBase& other) const noexcept {
            return Super::size() >= other.size() && std::equal(other.cbegin(), other.cend(), this->cbegin());
        }

        constexpr bool endsWith(const ZStringViewBase& other) const noexcept {
            return Super::size() >= other.size() && std::equal(other.cbegin(), other.cend(), this->cend() - other.size());
        }

        constexpr View toStringView() const noexcept {
            return View(this->data(), this->size());
        }

        constexpr String toString() const noexcept {
            return String(this->data(), this->size());
        }
    };

    constexpr bool isNullTerminated(std::string_view view) {
        return view.data()[view.size()] == '\0';
    }

    extern template class ZStringViewBase<char>;
    extern template class ZStringViewBase<char8_t>;
    extern template class ZStringViewBase<char16_t>;
    extern template class ZStringViewBase<char32_t>;
    extern template class ZStringViewBase<wchar_t>;

    using ZStringView = ZStringViewBase<char>;
    using ZStringViewWide = ZStringViewBase<wchar_t>;

    namespace literals {
        constexpr ZStringView operator""_zsv(const char *str, size_t length) noexcept {
            return ZStringView(str, length);
        }

        constexpr ZStringViewWide operator""_zsv(const wchar_t *str, size_t length) noexcept {
            return ZStringViewWide(str, length);
        }
    }
}
