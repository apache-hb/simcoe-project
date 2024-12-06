#include "threads/topology.hpp"

#include "common.hpp"

#include "config/config.hpp"

using namespace sm;
using namespace sm::threads;

static sm::opt<bool> gUsePinning {
    name = "pinning",
    desc = "Use core pinning",
    init = true
};

threads::ITopology *threads::ITopology::create() {
    return detail::hwlocInit();
}
