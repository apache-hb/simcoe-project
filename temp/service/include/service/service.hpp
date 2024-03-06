#pragma once

namespace sm::service {
    class IService {
    public:
        virtual ~IService() = default;
    };

    class IEngine {

    public:
        virtual ~IEngine() = default;
    };
}
