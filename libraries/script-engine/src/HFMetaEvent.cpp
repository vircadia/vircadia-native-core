//
//  HFMetaEvent.cpp
//  script-engine/src
//
//  Created by Stephen Birarda on 2014-10-27.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "HFMetaEvent.h"

QSet<QEvent::Type> HFMetaEvent::_types = QSet<QEvent::Type>();

QEvent::Type HFMetaEvent::newEventType() {
    QEvent::Type newType = static_cast<QEvent::Type>(QEvent::registerEventType());
    _types.insert(newType);
    return newType;
}