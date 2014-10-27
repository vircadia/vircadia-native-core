//
//  HFActionEvent.h
//  interface/src/events
//
//  Created by Stephen Birarda on 2014-10-27.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_HFActionEvent_h
#define hifi_HFActionEvent_h

#include "HFMetaEvent.h"

class HFActionEvent : public HFMetaEvent {
public:
    HFActionEvent(QEvent::Type type, const QPointF& localPosition);
    
    static QEvent::Type startType();
    static QEvent::Type endType();
private:
    QPointF _localPosition;
};

#endif // hifi_HFActionEvent_h