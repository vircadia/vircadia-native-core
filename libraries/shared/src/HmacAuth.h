//
// HmacAuth.h
// libraries/shared/src
//
//  Created by Simon Walton on 3/19/2018.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_HmacAuth_h
#define hifi_HmacAuth_h

#include <vector>
#include <memory>
#include <QtCore/QMutex>

class QUuid;

class HmacAuth {
public:
    enum AuthMethod { MD5, SHA1, SHA224, SHA256, RIPEMD160 };
    using HmacHash = std::vector<unsigned char>;
    
    explicit HmacAuth(AuthMethod authMethod = MD5);
    ~HmacAuth();

    bool setKey(const char * keyValue, int keyLen);
    bool setKey(const QUuid& uidKey);
    bool addData(const char * data, int dataLen);
    HmacHash result();

private:
    QMutex _lock;
    std::unique_ptr<struct hmac_ctx_st> _hmacContext;
    AuthMethod _authMethod { MD5 };
};

#endif  // hifi_HmacAuth_h
