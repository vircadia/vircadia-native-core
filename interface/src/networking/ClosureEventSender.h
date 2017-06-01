//
//  ClosureEventSender.h
//  interface/src/networking
//
//  Created by Stephen Birarda on 5/31/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ClosureEventSender_h
#define hifi_ClosureEventSender_h

#include <QtCore/QString>
#include <QtCore/QUuid>

#include <DependencyManager.h>

class ClosureEventSender : public Dependency {
    SINGLETON_DEPENDENCY

public:
    void setSessionID(QUuid sessionID) { _sessionID = sessionID; }

    void sendQuitStart();
    void sendQuitFinish();
    void sendCrashEvent();

private:
    QUuid _sessionID;
    QString _accessToken;
};

#endif // hifi_ClosureEventSender_h
