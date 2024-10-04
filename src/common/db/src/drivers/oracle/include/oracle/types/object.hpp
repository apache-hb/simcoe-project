#pragma once

#include "db/error.hpp"

#include <oci.h>

namespace sm::db::oracle {
    std::string oraErrorText(void *handle, sword status, ub4 type) noexcept;
    DbError oraGetHandleError(void *handle, sword status, ub4 type) noexcept;
    bool isSuccess(sword status) noexcept;

    /// @brief a handle to an OCI object
    /// @tparam T the opaque type of the OCI object
    /// @tparam H the OCI handle type
    /// @tparam D whether the held type is a descriptor or a handle
    template<typename T, ub4 H, bool D>
    class OraObject {
    public:
        T *mHandle = nullptr;

        OraObject& operator=(T *handle) noexcept {
            mHandle = handle;
            return *this;
        }

        static constexpr inline ub4 kType = H;

        constexpr bool isDescriptorType() const noexcept { return D; }

        sword setAttribute(OCIError *error, ub4 attr, std::string_view value) const noexcept {
            return OCIAttrSet(mHandle, kType, (void*)value.data(), value.size(), attr, error);
        }

        sword setAttribute(OCIError *error, ub4 attr, const void *value) const noexcept {
            return OCIAttrSet(mHandle, kType, (void*)value, 0, attr, error);
        }

        sword getAttribute(OCIError *error, ub4 attr, void *value, ub4 *size = nullptr) const noexcept {
            return OCIAttrGet(mHandle, kType, value, size, attr, error);
        }

        template<typename A>
        DbResult<A> getAttribute(OCIError *error, ub4 attr) const noexcept {
            A value;
            if (sword status = getAttribute(error, attr, &value))
                return std::unexpected(oraGetHandleError(mHandle, status, OCI_HTYPE_ERROR));

            return value;
        }

        operator T*() const noexcept {
            return mHandle;
        }

        T **operator&() noexcept {
            return &mHandle;
        }

        void **voidpp() noexcept {
            return reinterpret_cast<void**>(&mHandle);
        }

        DbError close(OCIError *error) noexcept {
            sword status = freeObject();
            return oraGetHandleError(error, status, OCI_HTYPE_ERROR);
        }

    private:
        sword freeObject() noexcept {
            if constexpr (D) {
                return OCIDescriptorFree(mHandle, kType);
            } else {
                return OCIHandleFree(mHandle, kType);
            }
        }
    };

    template<typename T, ub4 H>
    using OraHandle = OraObject<T, H, false>;

    template<typename T, ub4 H>
    using OraDescriptor = OraObject<T, H, true>;

    template<typename T>
    class OraDelete : public OraHandle<OCIError, OCI_HTYPE_ERROR> {
    public:
        using Super = OraHandle<OCIError, OCI_HTYPE_ERROR>;

        constexpr OraDelete() noexcept = default;
        constexpr OraDelete(OCIError *error) noexcept
            : Super(error)
        { }

        void operator()(T handle) noexcept {
            DbError error = handle.close(*this);
            if (error)
                fmt::println(stderr, "Error closing handle: {}", error.message());
        }
    };

    using OraError = OraHandle<OCIError, OCI_HTYPE_ERROR>;
    using OraEnv = OraHandle<OCIEnv, OCI_HTYPE_ENV>;
    using OraServer = OraHandle<OCIServer, OCI_HTYPE_SERVER>;
    using OraService = OraHandle<OCISvcCtx, OCI_HTYPE_SVCCTX>;
    using OraSession = OraHandle<OCISession, OCI_HTYPE_SESSION>;
    using OraTsx = OraHandle<OCITrans, OCI_HTYPE_TRANS>;
    using OraStmt = OraHandle<OCIStmt, OCI_HTYPE_STMT>;
    using OraBind = OraHandle<OCIBind, OCI_HTYPE_BIND>;
    using OraDefine = OraHandle<OCIDefine, OCI_HTYPE_DEFINE>;

    using OraParam = OraDescriptor<OCIParam, OCI_DTYPE_PARAM>;
    using OraDateTime = OraDescriptor<OCIDateTime, OCI_DTYPE_TIMESTAMP>;
}
