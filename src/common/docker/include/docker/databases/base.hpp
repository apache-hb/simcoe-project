#pragma once

#include <stdint.h>

#include <string>

namespace sm::docker {
    struct DbContainerCreateInfo {
        std::string dbName;
        std::string username;
        std::string password;
    };

    class IDbContainer {
    public:
        virtual ~IDbContainer() = default;

        virtual std::string getHost() const = 0;
        virtual uint16_t getPort() const = 0;
        virtual std::string getDatabaseName() const = 0;
        virtual std::string getUsername() const = 0;
        virtual std::string getPassword() const = 0;
    };
}
