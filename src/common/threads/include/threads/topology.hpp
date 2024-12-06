#pragma once

#include "threads/scheduler.hpp"

namespace sm::threads {
    class ITopology {
    public:
        virtual ~ITopology() = default;

        virtual IScheduler *newScheduler() = 0;
        virtual void save(db::Connection& connection) = 0;

        static ITopology *create();
        static ITopology *load(db::Connection& connection);
    };

    void saveThreadInfo(db::Connection& connection);
}
