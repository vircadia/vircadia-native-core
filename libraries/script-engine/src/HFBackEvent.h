//
//  HFBackEvent.h
//  script-engine/src
//
//  Created by Stephen Birarda on 2014-10-27.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_HFBackEvent_h
#define hifi_HFBackEvent_h

#include <qevent.h>
#include <qscriptengine.h>

#include "HFMetaEvent.h"

class HFBackEvent : public HFMetaEvent {
public:
    HFBackEvent() {};
    HFBackEvent(QEvent::Type type);
    
    static QEvent::Type startType();
    static QEvent::Type endType();
};

#endif // hifi_HFBackEvent_h