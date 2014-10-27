//
//  HFMetaEvent.h
//  interface/src/events
//
//  Created by Stephen Birarda on 2014-10-27.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_HFMetaEvent_h
#define hifi_HFMetaEvent_h

#include <qevent.h>

class HFMetaEvent : public QEvent {
public:
    HFMetaEvent(QEvent::Type type) : QEvent(type) {};
    static const QSet<QEvent::Type>& types() { return HFMetaEvent::_types; }
protected:
    static QEvent::Type newEventType();
    
    static QSet<QEvent::Type> _types;
};

#endif // hifi_HFMetaEvent_h