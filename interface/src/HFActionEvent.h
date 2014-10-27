//
//  HFActionEvent.h
//  interface/src
//
//  Created by Stephen Birarda on 2014-10-27.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_HFActionEvent_h
#define hifi_HFActionEvent_h

#include <qevent.h>

class HFActionEvent : public QEvent {
public:
    HFActionEvent();
    
    static QEvent::Type type();
};

#endif // hifi_HFActionEvent_h