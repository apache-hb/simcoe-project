#pragma once

#include "drivers/common.hpp"

#include "oracle.hpp"

namespace sm::db::oracle {
    class OraEnvironment final : public detail::IEnvironment {
        OraEnv mEnv;

        detail::IConnection *connect(const ConnectionConfig& config) noexcept(false) override;

        bool close() noexcept override {
            return true;
        }

    public:

        void *malloc(size_t size) noexcept;
        void *realloc(void *ptr, size_t size) noexcept;
        void free(void *ptr) noexcept;

        static void *wrapMalloc(void *ctx, size_t size);
        static void *wrapRealloc(void *ctx, void *ptr, size_t size);
        static void wrapFree(void *ctx, void *ptr);

        OraEnvironment() = default;

        void attach(OraEnv env) noexcept { mEnv = env; }
        OraEnv& env() noexcept { return mEnv; }
    };
}
