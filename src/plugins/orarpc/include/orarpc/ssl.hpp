#pragma once

#include "core/error.hpp"
#include "base/fs.hpp"

#include "net/net.hpp"

#include <openssl/ssl.h>
#include <openssl/err.h>

namespace sm::ssl {
    class SslError : public errors::Error<SslError> {
        using Super = errors::Error<SslError>;

        bool mSuccess;

        SslError(bool success, std::string message);

    public:
        using Exception = class SslException;
        using Super::Super;

        bool isSuccess() const noexcept { return mSuccess; }

        static SslError ok();
        static SslError error();
        static SslError errorOf(std::string_view message);

        template<typename... A>
        static SslError error(fmt::format_string<A...> fmt, A&&... args) {
            return errorOf(fmt::vformat(fmt, fmt::make_format_args(args...)));
        }
    };

    class SslException : public errors::Exception<SslError> {
        using Super = errors::Exception<SslError>;
    public:
        using Super::Super;
    };

    class PrivateKey {
        using KeyHandle = std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)>;
        static KeyHandle generateKey(unsigned bits);

        KeyHandle mKey;

        PrivateKey(KeyHandle key);

    public:
        PrivateKey(unsigned bits);

        static PrivateKey open(const fs::path& path);

        void save(std::ostream& os);

        EVP_PKEY *get() { return mKey.get(); }
    };

    class X509Certificate {
        using CertHandle = std::unique_ptr<X509, decltype(&X509_free)>;
        static CertHandle createCertFromPrivateKey(EVP_PKEY *key);

        CertHandle mCert;

        X509Certificate(CertHandle cert);

    public:
        X509Certificate(PrivateKey& key);

        static X509Certificate open(const fs::path& path);

        void save(std::ostream& os);

        static void save(std::ostream& os, X509 *cert);

        X509 *get() { return mCert.get(); }
    };

    class SslContext {
        using ContextHandle = std::unique_ptr<SSL_CTX, decltype(&SSL_CTX_free)>;
        static ContextHandle create();

        ContextHandle mContext;

    public:
        SslContext(ssl::PrivateKey key, ssl::X509Certificate cert);

        SSL_CTX *get() { return mContext.get(); }
    };

    class SslSession {
        using SessionHandle = std::unique_ptr<SSL, decltype(&SSL_free)>;
        static SessionHandle createSession(SSL_CTX *context);

        // socket comes first as it needs to be destroyed after the session
        net::Socket mSocket;
        SessionHandle mSession;

    public:
        SslSession(SslContext& context, net::Socket socket);
        ~SslSession() noexcept;

        SM_NOCOPY(SslSession);
        SM_NOMOVE(SslSession);

        SslError accept();

        int writeBytes(const void *src, int size);

        int readBytes(void *dst, int size);

        X509 *getPeerCertificate();

        SSL *get() { return mSession.get(); }
    };
}
