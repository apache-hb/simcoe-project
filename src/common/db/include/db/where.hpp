#pragma once

#include "dao/dao.hpp"

namespace sm::db {
    class Where {

    };

    template<dao::DaoInterface T>
    Where where(auto (T::*member)()) noexcept {

    }
}
