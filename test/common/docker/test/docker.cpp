#include <gtest/gtest.h>

#include "docker/docker.hpp"
#include "core/macros.h"
#include "logs/logger.hpp"
#include "logs/appenders/channels.hpp"

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
        sm::logs::appenders::addConsoleChannel();
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

TEST_F(DockerClientTest, PullImage) {
    sm::docker::DockerClient client = sm::docker::DockerClient::local();
    EXPECT_NO_THROW({
        client.pullImage("postgres", "16.9-alpine");
    });
}

TEST_F(DockerClientTest, ExportImage) {
    sm::docker::DockerClient client = sm::docker::DockerClient::local();
    std::fstream output("postgresql.tar", std::ios::out | std::ios::binary);
    EXPECT_NO_THROW({
        client.exportImage("postgres", "16.9-alpine", output);
    });
}

TEST_F(DockerClientTest, CreateContainer) {
    sm::docker::DockerClient client = sm::docker::DockerClient::local();
    auto containers = client.listContainers();
    for (const auto& container : containers) {
        if (container.isNamed("test-container")) {
            if (container.getState() == sm::docker::ContainerStatus::Running) {
                client.stop(container);
            }
            client.destroyContainer(container.getId());
        }
    }

    sm::docker::ContainerCreateInfo createInfo;
    createInfo.name = "test-container";
    createInfo.image = "postgres";
    createInfo.tag = "16.9-alpine";
    createInfo.ports.push_back({5432, 0, "tcp"});
    createInfo.env["POSTGRES_PASSWORD"] = "testpassword";

    sm::docker::ContainerId containerId;

    EXPECT_NO_THROW({
        containerId = client.createContainer(createInfo);
    });

    EXPECT_EQ(containerId.getId().size(), 64) << "Container ID should contain a valid ID";

    auto container = client.getContainer(containerId);
    EXPECT_EQ(container.getId(), containerId);
    EXPECT_EQ(container.getState(), sm::docker::ContainerStatus::Created);

    client.start(containerId);

    container = client.getContainer(containerId);
    EXPECT_EQ(container.getState(), sm::docker::ContainerStatus::Running);

    uint16_t mappedPort = container.getMappedPort(5432);
    EXPECT_NE(mappedPort, 0) << "Mapped port should not be zero";

    client.stop(containerId);
    client.destroyContainer(containerId);

    // auto port = container.getMappedPort(5432);
    // ASSERT_NE(port, 0) << "Mapped port should not be zero";
}
