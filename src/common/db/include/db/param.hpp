#pragma once

#include "db/db.hpp"

#include <map>

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

    class StatementParamInfo {
        std::multimap<std::string, int> mNamedParams;
        std::vector<ParamInfo> mParamInfo;
    };
}
