//
// HmacAuth.cpp

#include <openssl/hmac.h>

#include "HmacAuth.h"

#include <QUuid>

HmacAuth::HmacAuth(AuthMethod authMethod)
    : _hmacContext(new(HMAC_CTX))
    , _authMethod(authMethod) {
    HMAC_CTX_init(_hmacContext.get());
}

HmacAuth::~HmacAuth() {
    HMAC_CTX_cleanup(_hmacContext.get());
}

bool HmacAuth::setKey(const char * keyValue, int keyLen) {
    const EVP_MD * sslStruct = nullptr;

    switch (_authMethod)
    {
    case SHA1:
        sslStruct = EVP_sha1();
        break;

    case RIPEMD160:
        sslStruct = EVP_ripemd160();
        break;

    default:
        return false;
    }

    return (bool) HMAC_Init(_hmacContext.get(), keyValue, keyLen, sslStruct);
}

bool HmacAuth::setKey(const QUuid& uidKey) {
    const QByteArray rfcBytes(uidKey.toRfc4122());
    return setKey(rfcBytes.constData(), rfcBytes.length());
}

bool HmacAuth::addData(const char * data, int dataLen) {
    return (bool) HMAC_Update(_hmacContext.get(), reinterpret_cast<const unsigned char*>(data), dataLen);
}

HmacAuth::HmacHash HmacAuth::result() {
    HmacHash hashValue(EVP_MAX_MD_SIZE);
    unsigned int  hashLen;
    HMAC_Final(_hmacContext.get(), &hashValue[0], &hashLen);
    hashValue.resize((size_t) hashLen);
    return hashValue;
}
