#pragma once

#include "db/db.hpp"
#include "db/error.hpp"

#include "dao/dao.hpp"

namespace sm::db {
    class ParamInfo {
        DataType mDataType;
        int mIndex;

    public:
        ParamInfo(DataType type, int index) noexcept
            : mDataType(type)
            , mIndex(index)
        { }

        DataType type() const noexcept { return mDataType; }
        int index() const noexcept { return mIndex; }
    };

    using ValueHolder = std::variant<int64_t, uint64_t, bool, std::string, double, Blob, DateTime>;

    class ParamValue {
        ParamInfo mParamInfo;
        ValueHolder mValue;

    public:
        template<typename T>
        ParamValue(DataType type, int index, T value) noexcept
            : mParamInfo(type, index)
            , mValue(value)
        { }

        ParamInfo info() const noexcept { return mParamInfo; }
        bool isA(DataType type) const noexcept { return mParamInfo.type() == type; }

        ValueHolder value() const noexcept { return mValue; }

        int64_t toInt() const noexcept { return std::get<int64_t>(mValue); }
        uint64_t toUInt() const noexcept { return std::get<uint64_t>(mValue); }
        bool toBool() const noexcept { return std::get<bool>(mValue); }
        std::string toString() const noexcept { return std::get<std::string>(mValue); }
        double toDouble() const noexcept { return std::get<double>(mValue); }
        Blob toBlob() const noexcept { return std::get<Blob>(mValue); }
        DateTime toDateTime() const noexcept { return std::get<DateTime>(mValue); }
    };
}