//
//  HFActionEvent.cpp
//  interface/src
//
//  Created by Stephen Birarda on 2014-10-27.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "HFActionEvent.h"

HFActionEvent::HFActionEvent(const QPointF& localPosition) :
	QEvent(HFActionEvent::type()),
    _localPosition(localPosition)
{
    
}

QEvent::Type HFActionEvent::type() {
    static QEvent::Type hfActionType = QEvent::None;
    if (hfActionType == QEvent::None) {
        hfActionType = static_cast<QEvent::Type>(QEvent::registerEventType());
    }
    return hfActionType;
}