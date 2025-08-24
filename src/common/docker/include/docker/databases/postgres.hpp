#pragma once

#include "docker/docker.hpp"

#include "base.hpp"

namespace sm::docker {
    struct PostgresContainerCreateInfo {
        std::string image = "postgres";
        std::string tag = "17.5-alpine";
        std::string dbName = "postgres";
        std::string username = "postgres";
        std::string password = "postgres";
    };

    class PostgresContainer final : public IDbContainer {
        DockerClient mDocker;
        Container mContainer;

        std::string mDatabaseName;
        std::string mUsername;
        std::string mPassword;

    public:
        PostgresContainer(const PostgresContainerCreateInfo& createInfo);
        ~PostgresContainer() override;

        std::string getHost() const override;
        uint16_t getPort() const override;
        std::string getDatabaseName() const override;
        std::string getUsername() const override;
        std::string getPassword() const override;
    };
}
