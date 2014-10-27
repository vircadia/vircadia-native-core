//
//  HFActionEvent.cpp
//  interface/src/events
//
//  Created by Stephen Birarda on 2014-10-27.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "HFActionEvent.h"

HFActionEvent::HFActionEvent(QEvent::Type type, const QPointF& localPosition) :
	HFMetaEvent(type),
    _localPosition(localPosition)
{
    
}

QEvent::Type HFActionEvent::startType() {
    static QEvent::Type startType = HFMetaEvent::newEventType();
    return startType;
}

QEvent::Type HFActionEvent::endType() {
    static QEvent::Type endType = HFMetaEvent::newEventType();
    return endType;
}

