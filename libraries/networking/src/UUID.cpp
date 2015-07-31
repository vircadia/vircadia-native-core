//
//  UUID.cpp
//  libraries/shared/src
//
//  Created by Stephen Birarda on 10/7/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "UUID.h"

QString uuidStringWithoutCurlyBraces(const QUuid& uuid) {
    QString uuidStringNoBraces = uuid.toString().mid(1, uuid.toString().length() - 2);
    return uuidStringNoBraces;
}
