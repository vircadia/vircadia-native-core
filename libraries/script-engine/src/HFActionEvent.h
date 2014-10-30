//
//  HFActionEvent.h
//  script-engine/src
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

#include <qscriptengine.h>

class HFActionEvent : public HFMetaEvent {
public:
    HFActionEvent() {};
    HFActionEvent(QEvent::Type type, const QPointF& localPosition);
    
    static QEvent::Type startType();
    static QEvent::Type endType();
    
    static QScriptValue toScriptValue(QScriptEngine* engine, const HFActionEvent& event);
    static void fromScriptValue(const QScriptValue& object, HFActionEvent& event);
    
    QPointF localPosition;
};

Q_DECLARE_METATYPE(HFActionEvent)

#endif // hifi_HFActionEvent_h