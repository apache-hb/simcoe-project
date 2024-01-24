#pragma once

namespace simcoe::service {
    class IService {
    public:
        virtual ~IService() = default;
    };

    class IEngine {

    public:
        virtual ~IEngine() = default;
    };
}
