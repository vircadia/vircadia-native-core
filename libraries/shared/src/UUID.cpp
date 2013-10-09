//
//  UUID.cpp
//  hifi
//
//  Created by Stephen Birarda on 10/7/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include "UUID.h"

QString uuidStringWithoutCurlyBraces(const QUuid& uuid) {
    QString uuidStringNoBraces = uuid.toString().mid(1, uuid.toString().length() - 2);
    return uuidStringNoBraces;
}