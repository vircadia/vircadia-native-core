//
//  CloseEventSender.h
//  interface/src/networking
//
//  Created by Stephen Birarda on 5/31/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_CloseEventSender_h
#define hifi_CloseEventSender_h

#include <atomic>

#include <QtCore/QString>
#include <QtCore/QUuid>

#include <DependencyManager.h>

class CloseEventSender : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY

public:
    void startThread();
    bool hasTimedOutQuitEvent();
    bool hasFinishedQuitEvent() { return _hasFinishedQuitEvent; }

public slots:
    void sendQuitEventAsync();

private slots:
    void handleQuitEventFinished();

private:
    std::atomic<bool> _hasFinishedQuitEvent { false };
    std::atomic<int64_t> _quitEventStartTimestamp;
};

#endif // hifi_CloseEventSender_h
