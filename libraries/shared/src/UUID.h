//
//  UUID.h
//  libraries/shared/src
//
//  Created by Stephen Birarda on 10/7/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_UUID_h
#define hifi_UUID_h

#include <QtCore/QUuid>

const int NUM_BYTES_RFC4122_UUID = 16;

QString uuidStringWithoutCurlyBraces(const QUuid& uuid);

#endif // hifi_UUID_h
