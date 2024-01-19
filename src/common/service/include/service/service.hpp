#pragma once

#include <sm_service_api.hpp>

namespace simcoe::service {
    class SM_SERVICE_API IService {
    public:
        virtual ~IService() = default;
    };

    class SM_SERVICE_API IEngine {

    public:
        virtual ~IEngine() = default;
    };
}
