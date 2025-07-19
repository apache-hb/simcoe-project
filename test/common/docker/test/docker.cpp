#include <gtest/gtest.h>

#include "docker/docker.hpp"
#include "core/macros.h"
#include "logs/logger.hpp"
#include "logs/sinks/channels.hpp"

#include "core/string.hpp"

#include "base/panic.h"
#include "core/macros.h"
#include "os/core.h"

class DockerClientTest : public testing::Test {
public:
    static void SetUpTestSuite() {
        gPanicHandler = [](source_info_t info, const char *msg, va_list args) {
            auto err = sm::vformat(msg, args);

            fmt::println(stderr, "ASSERT : {}", info.function);
            fmt::println(stderr, "WHERE  : {}:{}", info.file, info.line);
            fmt::println(stderr, "--------");
            fmt::println(stderr, "{}", err);

            os_exit(CT_EXIT_INTERNAL);
        };

        sm::docker::init();
        sm::logs::create(sm::logs::LoggingConfig { });
        sm::logs::sinks::addConsoleChannel();
    }

    static void TearDownTestSuite() {
        sm::docker::cleanup();
    }
};

TEST_F(DockerClientTest, ConnectLocal) {
    sm::docker::DockerClient client = sm::docker::DockerClient::local();
    EXPECT_NO_THROW({
        auto containers = client.listContainers();
        auto images = client.listImages();
        for (const auto& image : images) {
            fmt::println("Image: {}", image);
        }
    });
}
