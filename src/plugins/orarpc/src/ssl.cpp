#define _CRT_SECURE_NO_WARNINGS

#include "orarpc/ssl.hpp"
#include "core/defer.hpp"

#include <fmt/ranges.h>


using namespace sm;
using namespace sm::ssl;

///
/// error
///

using MessageList = std::vector<std::string>;

static MessageList getErrorMessages() {
    MessageList buffer;
    auto cb = [](const char *message, size_t length, void *ctx) -> int {
        MessageList *buffer = reinterpret_cast<MessageList*>(ctx);
        buffer->push_back(std::string(message, length));
        return (int)length;
    };

    ERR_print_errors_cb(cb, reinterpret_cast<void*>(&buffer));

    return buffer;
}

SslError::SslError(bool success, std::string message)
    : Super(std::move(message), success)
    , mSuccess(success)
{ }

SslError SslError::ok() {
    return SslError { true, "SUCCESS" };
}

SslError SslError::error() {
    MessageList messages = getErrorMessages();
    return SslError { false, fmt::format("{}", fmt::join(messages, "\n")) };
}

SslError SslError::errorOf(std::string_view message) {
    MessageList messages = getErrorMessages();
    return SslError { false, fmt::format("{}\n{}", message, fmt::join(messages, "\n")) };
}

///
/// keys and certificates
///

PrivateKey::KeyHandle PrivateKey::generateKey(unsigned bits) {
    if (EVP_PKEY *key = EVP_RSA_gen(bits)) {
        return PrivateKey::KeyHandle(key, &EVP_PKEY_free);
    }

    throw SslException{SslError::error("EVP_RSA_gen({})", bits)};
}

PrivateKey::PrivateKey(KeyHandle key)
    : mKey(std::move(key))
{ }

PrivateKey::PrivateKey(unsigned bits)
    : PrivateKey(generateKey(bits))
{ }

PrivateKey PrivateKey::open(const fs::path& path) {
    FILE *file = fopen(path.string().c_str(), "r");
    if (file == nullptr) {
        throw SslException{SslError::errorOf("fopen")};
    }

    defer { (void)fclose(file); };

    if (EVP_PKEY *key = PEM_read_PrivateKey(file, nullptr, nullptr, nullptr)) {
        return PrivateKey(KeyHandle(key, &EVP_PKEY_free));
    }

    throw SslException{SslError::errorOf("PEM_read_PrivateKey")};
}

void PrivateKey::save(std::ostream& os) {
    BIO *bio = BIO_new(BIO_s_mem());
    if (bio == nullptr) {
        throw SslException{SslError::errorOf("BIO_new_mem_buf")};
    }

    defer { BIO_free(bio); };

    if (PEM_write_bio_PrivateKey(bio, mKey.get(), nullptr, nullptr, 0, nullptr, nullptr) <= 0) {
        throw SslException{SslError::errorOf("PEM_write_bio_PrivateKey")};
    }

    char *data;
    long size = BIO_get_mem_data(bio, &data);
    os.write(data, size);
}

X509Certificate::CertHandle X509Certificate::createCertFromPrivateKey(EVP_PKEY *key) {
    X509 *x509 = X509_new();
    if (x509 == nullptr) {
        throw SslException{SslError::errorOf("X509_new")};
    }

    ASN1_INTEGER_set(X509_get_serialNumber(x509), 1);

    X509_gmtime_adj(X509_get_notBefore(x509), 0);
    X509_gmtime_adj(X509_get_notAfter(x509), (60ul * 60ul * 24ul * 365ul * 100ul)); // oracle rotates keys every 100 years

    X509_set_pubkey(x509, key);

    X509_NAME *name = X509_get_subject_name(x509);

    X509_NAME_add_entry_by_txt(name, "C", MBSTRING_ASC, (unsigned char*)"US", -1, -1, 0);
    X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, (unsigned char*)"ons_ssl", -1, -1, 0);

    X509_set_issuer_name(x509, name);

    X509_sign(x509, key, EVP_sha256());

    return CertHandle(x509, &X509_free);
}

X509Certificate::X509Certificate(CertHandle cert)
    : mCert(std::move(cert))
{ }

X509Certificate::X509Certificate(PrivateKey& key)
    : X509Certificate(createCertFromPrivateKey(key.get()))
{ }

X509Certificate X509Certificate::open(const fs::path& path) {
    FILE *file = fopen(path.string().c_str(), "r");
    if (file == nullptr) {
        throw SslException{SslError::errorOf("fopen")};
    }

    defer { (void)fclose(file); };

    if (X509 *x509 = PEM_read_X509(file, nullptr, nullptr, nullptr)) {
        return X509Certificate(CertHandle(x509, &X509_free));
    }

    throw SslException{SslError::errorOf("PEM_read_X509")};
}

void X509Certificate::save(std::ostream& os) {
    X509Certificate::save(os, mCert.get());
}

void X509Certificate::save(std::ostream& os, X509 *cert) {
    CTASSERTF(cert != nullptr, "X509 certificate is null");

    BIO *bio = BIO_new(BIO_s_mem());
    if (bio == nullptr) {
        throw SslException{SslError::errorOf("BIO_new")};
    }

    defer { BIO_free(bio); };

    if (int rc = PEM_write_bio_X509(bio, cert); rc != 1) {
        throw SslException{SslError::error("PEM_write_bio_X509 {:#x}", ERR_get_error())};
    }

    BUF_MEM *mem = nullptr;
    BIO_get_mem_ptr(bio, &mem);

    if (mem == nullptr || mem->data == nullptr || mem->length == 0) {
        throw SslException{SslError::error("BIO_get_mem_ptr {:#x}", ERR_get_error())};
    }

    os.write(mem->data, mem->length);
}

///
/// ssl context
///

SslContext::ContextHandle SslContext::create() {
    const SSL_METHOD *method = TLS_server_method();
    if (SSL_CTX *context = SSL_CTX_new(method)) {
        return ContextHandle(context, &SSL_CTX_free);
    }

    throw SslException{SslError::errorOf("SSL_CTX_new + TLS_server_method")};
}

static void setupEncryptionConfig(SSL_CTX *context, ssl::PrivateKey key, ssl::X509Certificate cert) {
    if (SSL_CTX_use_certificate(context, cert.get()) <= 0) {
        throw SslException{SslError::errorOf("SSL_CTX_use_certificate")};
    }

    if (SSL_CTX_use_PrivateKey(context, key.get()) <= 0) {
        throw SslException{SslError::errorOf("SSL_CTX_use_PrivateKey")};
    }
}

SslContext::SslContext(ssl::PrivateKey key, ssl::X509Certificate cert)
    : mContext(create())
{
    setupEncryptionConfig(mContext.get(), std::move(key), std::move(cert));
    SSL_CTX_set_verify(mContext.get(), SSL_VERIFY_PEER, nullptr);
}

///
/// ssl session
///

SslSession::SessionHandle SslSession::createSession(SSL_CTX *context) {
    if (SSL *ssl = SSL_new(context)) {
        return SessionHandle(ssl, &SSL_free);
    }

    throw SslException{SslError::errorOf("SSL_new")};
}

SslSession::SslSession(SslContext& context, net::Socket socket)
    : mSocket(std::move(socket))
    , mSession(createSession(context.get()))
{
    SSL_set_fd(mSession.get(), mSocket.get());
}

SslSession::~SslSession() noexcept {
    if (SSL_get_fd(mSession.get()) != -1) {
        SSL_shutdown(mSession.get());
    }
}

SslError SslSession::accept() {
    if (SSL_accept(mSession.get()) <= 0) {
        return SslError::errorOf("SSL_accept");
    } else {
        return SslError::ok();
    }
}

int SslSession::writeBytes(const void *src, int size) {
    return SSL_write(mSession.get(), src, size);
}

int SslSession::readBytes(void *dst, int size) {
    return SSL_read(mSession.get(), dst, size);
}

X509 *SslSession::getPeerCertificate() {
    return SSL_get_peer_certificate(mSession.get());
}
