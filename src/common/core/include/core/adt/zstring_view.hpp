#pragma once

#include "core/adt/range.hpp"
#include "core/adt/small_string.hpp"

namespace sm {
    template<typename T>
    concept IsChar
        = std::same_as<T, char>
        || std::same_as<T, char8_t>
        || std::same_as<T, char16_t>
        || std::same_as<T, char32_t>
        || std::same_as<T, wchar_t>;

    template<IsChar T>
    constexpr size_t zstrlen(const T *str) {
        size_t length = 0;
        while (*str++) {
            length += 1;
        }
        return length;
    }

    template<IsChar T>
    class BasicZStringView final : public core::detail::Collection<const T> {
        using Super = core::detail::Collection<const T>;
        using Self = BasicZStringView;

        static constexpr T kEmptyString[] = { T(0) };

        constexpr void verifyZString() const noexcept {
            CTASSERTF(this->getFront() <= this->getBack(), "mFront is greater than mBack");
            CTASSERTF(*this->getBack() == T(0), "mBack is not null terminated");
        }

    public:
        using String = std::basic_string<T>;
        using View = std::basic_string_view<T>;

        constexpr BasicZStringView(const T *front, const T *back) noexcept
            : Super(front, back)
        {
            verifyZString();
        }

        constexpr BasicZStringView() noexcept
            : Self(kEmptyString)
        { }

        constexpr BasicZStringView(const T *front, size_t length) noexcept
            : Self(front, front + length)
        { }

        template<size_t N>
        constexpr BasicZStringView(T (&str)[N]) noexcept
            : Self(str, N)
        { }

        constexpr BasicZStringView(const T *front) noexcept
            : Self(front, front + zstrlen(front))
        { }

        constexpr BasicZStringView(const std::basic_string<T> &str) noexcept
            : Self(str.c_str(), str.c_str() + str.size())
        { }

        template<size_t N>
        constexpr BasicZStringView(const SmallString<N>& str) noexcept
            : Self(str.data(), str.data() + str.size())
        { }

        [[nodiscard]] constexpr const T *c_str() const noexcept { return this->mFront; }

        [[nodiscard]] constexpr size_t length() const noexcept { return Super::size(); }

        constexpr std::strong_ordering operator<=>(const Self& other) const noexcept {
            return std::lexicographical_compare_three_way(this->cbegin(), this->cend(), other.cbegin(), other.cend());
        }

        constexpr bool operator==(const Self& other) const noexcept {
            return std::equal(this->cbegin(), this->cend(), other.cbegin(), other.cend());
        }

        [[nodiscard]] constexpr bool startsWith(const Self& other) const noexcept {
            return Super::size() >= other.size() && std::equal(other.cbegin(), other.cend(), this->cbegin());
        }

        [[nodiscard]] constexpr bool endsWith(const Self& other) const noexcept {
            return Super::size() >= other.size() && std::equal(other.cbegin(), other.cend(), this->cend() - other.size());
        }

        [[nodiscard]] constexpr View toStringView() const noexcept {
            return View(this->data(), this->size());
        }

        [[nodiscard]] constexpr String toString() const noexcept {
            return String(this->data(), this->size());
        }
    };

    constexpr bool isNullTerminated(std::string_view view) {
        return view[view.size()] == '\0';
    }

    extern template class BasicZStringView<char>;
    extern template class BasicZStringView<char8_t>;
    extern template class BasicZStringView<char16_t>;
    extern template class BasicZStringView<char32_t>;
    extern template class BasicZStringView<wchar_t>;

    using ZStringView = BasicZStringView<char>;
    using ZStringViewWide = BasicZStringView<wchar_t>;

    namespace literals {
        constexpr ZStringView operator""_zsv(const char *str, size_t length) noexcept {
            return {str, length};
        }

        constexpr ZStringViewWide operator""_zsv(const wchar_t *str, size_t length) noexcept {
            return {str, length};
        }
    }
}
