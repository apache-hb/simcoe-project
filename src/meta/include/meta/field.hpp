#pragma once

namespace sm::reflect {
    class Class;

    class Field {
    public:
        Class& getDeclaringClass() const noexcept;
        std::string_view getName() const noexcept;
    };
}