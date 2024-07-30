#pragma once

#include <string>
#include <string_view>

namespace sm::db {
    class DbError {
        int mCode = 0;
        int mStatus = eOk;
        std::string mMessage;

    public:
        enum : int {
            eOk = 0,
            eError = 1,
            eUnimplemented = 2,
            eNoMoreData = 3,
        };

        DbError(int code, int status, std::string message) noexcept
            : mCode(code)
            , mStatus(status)
            , mMessage(std::move(message))
        { }

        int code() const noexcept { return mCode; }
        int status() const noexcept { return mStatus; }
        std::string_view message() const noexcept { return mMessage; }

        operator bool() const noexcept {
            return mStatus != eOk;
        }

        bool isSuccess() const noexcept {
            return mStatus == eOk;
        }

        bool isDone() const noexcept {
            return mStatus == eNoMoreData;
        }

        static DbError ok() noexcept;
        static DbError todo() noexcept;
        static DbError error(int code, std::string message) noexcept;
        static DbError noMoreData(int code) noexcept;
        static DbError unsupported(std::string_view subject) noexcept;
    };
}
