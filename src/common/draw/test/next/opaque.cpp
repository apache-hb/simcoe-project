#include "test/render_test_common.hpp"

#include "draw/components/camera.hpp"
#include "draw/next/opaque.hpp"
#include "draw/next/blit.hpp"

namespace dn = sm::draw::next;

TEST_CASE("Opaque draw") {
    system::create(GetModuleHandle(nullptr));
    WindowContextTest test{30};
    auto& context = test.getContext();

    [[maybe_unused]]
    dn::ViewportInfo viewport {
        .colour = DXGI_FORMAT_R8G8B8A8_UNORM,
        .depth = DXGI_FORMAT_D32_FLOAT,
        .size = test.getClientSize(),
    };

    [[maybe_unused]]
    dn::CameraInfo camera {
        .position = { 0.0f, 0.0f, -5.0f },
        .direction = math::quatf::identity(),
        .fov = math::radf(60.0f),
    };

    while (test.next()) {
        context.begin();

        context.end();
    }

    SUCCEED("Ran CoreContext loop");
}
