#pragma once

#include <string>

namespace sm::db {
    class User {
        std::string mName;
        bool mIsManaged;

    public:
        User() = default;
        User(std::string name, bool isManaged)
            : mName(std::move(name))
            , mIsManaged(isManaged)
        { }

        const std::string& getName() const { return mName; }
        bool isManaged() const { return mIsManaged; }

        bool operator==(const User& other) const {
            return mName == other.mName && mIsManaged == other.mIsManaged;
        }
    };
}
