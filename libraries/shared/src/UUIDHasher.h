//
//  UUIDHasher.h
//  libraries/networking/src
//
//  Created by Stephen Birarda on 2014-11-05.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_UUIDHasher_h
#define hifi_UUIDHasher_h

#include <QUuid>

// uses the same hashing performed by Qt
// https://qt.gitorious.org/qt/qtbase/source/73ef64fb5fabb60101a3cac6e43f0c5bb2298000:src/corelib/plugin/quuid.cpp

class UUIDHasher {
public:
    size_t operator()(const QUuid& uuid) const {
        return qHash(uuid);
    }
};

namespace std {
    template <> struct hash<QUuid> {
        size_t operator()(const QUuid& uuid) const {
            return qHash(uuid);
        }
    };
}

#endif // hifi_UUIDHasher_h
