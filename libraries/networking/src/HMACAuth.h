//
//  HMACAuth.h
//  libraries/networking/src
//
//  Created by Simon Walton on 3/19/2018.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_HMACAuth_h
#define hifi_HMACAuth_h

#include <vector>
#include <memory>
#include <QtCore/QMutex>

class QUuid;

class HMACAuth {
public:
    enum AuthMethod { MD5, SHA1, SHA224, SHA256, RIPEMD160 };
    using HMACHash = std::vector<unsigned char>;
    
    explicit HMACAuth(AuthMethod authMethod = MD5);
    ~HMACAuth();

    bool setKey(const char* keyValue, int keyLen);
    bool setKey(const QUuid& uidKey);
    // Calculate complete hash in one.
    bool calculateHash(HMACHash& hashResult, const char* data, int dataLen);

    // Append to data to be hashed.
    bool addData(const char* data, int dataLen);
    // Get the resulting hash from calls to addData().
    // Note that only one hash may be calculated at a time for each
    // HMACAuth instance if this interface is used.
    HMACHash result();

private:
    QRecursiveMutex _lock;
    struct hmac_ctx_st* _hmacContext;
    AuthMethod _authMethod;
};

#endif  // hifi_HMACAuth_h
