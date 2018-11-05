//
//  HMACAuth.cpp
//  libraries/networking/src
//
//  Created by Simon Walton on 3/19/2018.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "HMACAuth.h"

#include <openssl/opensslv.h>
#include <openssl/hmac.h>

#include <QUuid>
#include "NetworkLogging.h"
#include <cassert>

#if OPENSSL_VERSION_NUMBER >= 0x10100000
HMACAuth::HMACAuth(AuthMethod authMethod)
    : _hmacContext(HMAC_CTX_new())
    , _authMethod(authMethod) { }

HMACAuth::~HMACAuth()
{
    HMAC_CTX_free(_hmacContext);
}

#else

HMACAuth::HMACAuth(AuthMethod authMethod)
    : _hmacContext(new HMAC_CTX())
    , _authMethod(authMethod) {
    HMAC_CTX_init(_hmacContext);
}

HMACAuth::~HMACAuth() {
    HMAC_CTX_cleanup(_hmacContext);
    delete _hmacContext;
}
#endif

bool HMACAuth::setKey(const char* keyValue, int keyLen) {
    const EVP_MD* sslStruct = nullptr;

    switch (_authMethod) {
    case MD5:
        sslStruct = EVP_md5();
        break;

    case SHA1:
        sslStruct = EVP_sha1();
        break;

    case SHA224:
        sslStruct = EVP_sha224();
        break;

    case SHA256:
        sslStruct = EVP_sha256();
        break;

    case RIPEMD160:
        sslStruct = EVP_ripemd160();
        break;

    default:
        return false;
    }

    QMutexLocker lock(&_lock);
    return (bool) HMAC_Init_ex(_hmacContext, keyValue, keyLen, sslStruct, nullptr);
}

bool HMACAuth::setKey(const QUuid& uidKey) {
    const QByteArray rfcBytes(uidKey.toRfc4122());
    return setKey(rfcBytes.constData(), rfcBytes.length());
}

bool HMACAuth::addData(const char* data, int dataLen) {
    QMutexLocker lock(&_lock);
    return (bool) HMAC_Update(_hmacContext, reinterpret_cast<const unsigned char*>(data), dataLen);
}

HMACAuth::HMACHash HMACAuth::result() {
    HMACHash hashValue(EVP_MAX_MD_SIZE);
    unsigned int hashLen;
    QMutexLocker lock(&_lock);
    
    auto hmacResult = HMAC_Final(_hmacContext, &hashValue[0], &hashLen);
    
    if (hmacResult) {
        hashValue.resize((size_t)hashLen);
    } else {
        // the HMAC_FINAL call failed - should not be possible to get into this state
        qCWarning(networking) << "Error occured calling HMAC_Final";
        assert(hmacResult);
    }

    // Clear state for possible reuse.
    HMAC_Init_ex(_hmacContext, nullptr, 0, nullptr, nullptr);
    return hashValue;
}

bool HMACAuth::calculateHash(HMACHash& hashResult, const char* data, int dataLen) {
    QMutexLocker lock(&_lock);
    if (!addData(data, dataLen)) {
        qCWarning(networking) << "Error occured calling HMACAuth::addData()";
        assert(false);
        return false;
    }

    hashResult = result();
    return true;
}
