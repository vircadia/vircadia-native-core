//
//  UUIDHashKey.h
//  libraries/physcis/src
//
//  Created by Andrew Meadows 2014.11.05
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_UUIDHashKey_h
#define hifi_UUIDHashKey_h

#include <QUuid>

class UUIDHashKey {
public:
    UUIDHashKey(const QUuid& id) : _hash(0), _id(id) { _hash = (int)(qHash(id)); }

    bool equals(const UUIDHashKey& other) const {
        return _hash == other._hash && _id == other._id;
    }

    unsigned int getHash() const { return (unsigned int)_hash; }
protected:
    int _hash;
    QUuid _id;
};

#endif // hifi_UUIDHashKey_h
