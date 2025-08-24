#pragma once

#include "docker/docker.hpp"

#include "base.hpp"

namespace sm::docker {
    enum class OracleDbImage {
        /// @brief Full Oracle 23c free image with all features.
        eOracle23Full,

        /// @brief Oracle 23c Lite image with limited features
        /// @details Improved startup time and container size, designed for CI/CD pipelines.
        eOracle23Lite,
    };

    struct OracleDbContainerCreateInfo {
        OracleDbImage image = OracleDbImage::eOracle23Lite;
        std::string password;
    };

    class OracleDbContainer : public IDbContainer {
        DockerClient mDocker;
        Container mContainer;
    public:
        OracleDbContainer(const OracleDbContainerCreateInfo& createInfo);
        ~OracleDbContainer() override;

        std::string getHost() const override;
        uint16_t getPort() const override;
        std::string getDatabaseName() const override;
        std::string getUsername() const override;
        std::string getPassword() const override;
    };
}
