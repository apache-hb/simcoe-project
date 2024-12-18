#include "test/draw_test_common.hpp"

#include "draw/next/context.hpp"

namespace dn = sm::draw::next;

void runTestPass(std::string name) {
    DrawWindowTestContext test{180, true, std::move(name)};
    bt_update();

    // creating 2 windows in the same process causes the second one to close immediately
    // do this kinda silly hack to make sure we run all iters
    while (test.frames.iters-- > 0) {
        test.doEvent();
        test.update();
    }

    SUCCEED("Ran CoreContext loop");
}

TEST_CASE("Dear ImGui DrawContext") {
    system::create(GetModuleHandle(nullptr));

    runTestPass(sm::system::getProgramName() + "-run1");
    runTestPass(sm::system::getProgramName() + "-run2");
}
