#pragma once

#include <sm_service_api.hpp>

#include <string>

namespace simcoe::service {
    class SM_SERVICE_API IService {
        std::string name;

    public:
        virtual ~IService() = default;
    };

    class SM_SERVICE_API IEngine {

    public:
        virtual ~IEngine() = default;
    };
}
