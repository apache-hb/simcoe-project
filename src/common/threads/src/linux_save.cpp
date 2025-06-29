#include "threads/threads.hpp"

#include "threads/topology.hpp"

#include "db/connection.hpp"

#include "topology.dao.hpp"

using namespace sm::threads;

namespace topology = sm::dao::topology;

void sm::threads::saveThreadInfo(db::Connection& connection) {
    connection.createTable(topology::CpuSetInfo::table());
    connection.createTable(topology::LogicalProcessorInfo::table());
}
