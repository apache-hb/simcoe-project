#pragma once

// std::begin, std::end
#include <span>

namespace sm {
    template<typename T, size_t N = std::dynamic_extent>
    using Span = std::span<T, N>;

#if 0
    template<typename T>
    class Span {
        T *m_front;
        T *m_back;

    public:
        constexpr Span(T *front = nullptr, T *back = nullptr)
            : m_front(front), m_back(back) {
        }

        constexpr Span(T *front, size_t count)
            : m_front(front), m_back(front + count) {
        }

        template<typename C>
        constexpr Span(C &container)
            : m_front(std::begin(container))
            , m_back(std::end(container))
        { }

        constexpr T *begin() const {
            return m_front;
        }

        constexpr T *end() const {
            return m_back;
        }

        constexpr T *data() const {
            return m_front;
        }

        constexpr size_t length() const {
            return m_back - m_front;
        }

        constexpr size_t size_bytes() const {
            return length() * sizeof(T);
        }

        constexpr size_t size() const {
            return length();
        }

        constexpr T &operator[](size_t index) const {
            return m_front[index];
        }

        constexpr T &front() const {
            return *m_front;
        }

        constexpr T &back() const {
            return *(m_back - 1);
        }
    };
#endif
}
