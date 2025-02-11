#include "stdafx.hpp"

#include "drivers/common.hpp"

#include "db/bind.hpp"

using namespace sm;
using namespace sm::db;

///
/// named bindpoint
///

DbError BindPoint::tryBindInt(int64_t value) noexcept {
    return mImpl->bindIntByName(mName, value);
}

DbError BindPoint::tryBindUInt(uint64_t value) noexcept {
    return mImpl->bindIntByName(mName, value);
}

DbError BindPoint::tryBindBool(bool value) noexcept {
    return mImpl->bindBooleanByName(mName, value);
}

DbError BindPoint::tryBindString(std::string_view value) noexcept {
    return mImpl->bindStringByName(mName, value);
}

DbError BindPoint::tryBindDouble(double value) noexcept {
    return mImpl->bindDoubleByName(mName, value);
}

DbError BindPoint::tryBindBlob(Blob value) noexcept {
    return mImpl->bindBlobByName(mName, std::move(value));
}

DbError BindPoint::tryBindDateTime(DateTime value) noexcept {
    return mImpl->bindDateTimeByName(mName, value);
}

DbError BindPoint::tryBindNull() noexcept {
    return mImpl->bindNullByName(mName);
}
