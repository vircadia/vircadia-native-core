//
//  HFBackEvent.cpp
//  script-engine/src
//
//  Created by Stephen Birarda on 2014-10-27.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "HFBackEvent.h"

HFBackEvent::HFBackEvent(QEvent::Type type) :
    HFMetaEvent(type)
{
    
}

QEvent::Type HFBackEvent::startType() {
    static QEvent::Type startType = HFMetaEvent::newEventType();
    return startType;
}

QEvent::Type HFBackEvent::endType() {
    static QEvent::Type endType = HFMetaEvent::newEventType();
    return endType;
}
