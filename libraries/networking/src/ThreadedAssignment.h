//
//  ThreadedAssignment.h
//  libraries/networking/src
//
//  Created by Stephen Birarda on 12/3/2013.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ThreadedAssignment_h
#define hifi_ThreadedAssignment_h

#include <QtCore/QSharedPointer>

#include "ReceivedMessage.h"

#include "Assignment.h"

class ThreadedAssignment : public Assignment {
    Q_OBJECT
public:
    ThreadedAssignment(ReceivedMessage& message);
    ~ThreadedAssignment() { stop(); }

    void setFinished(bool isFinished);
    virtual void aboutToFinish() { };
    void addPacketStatsAndSendStatsPacket(QJsonObject statsObject);

public slots:
    /// threaded run of assignment
    virtual void run() = 0;
    Q_INVOKABLE virtual void stop() { setFinished(true); }
    virtual void sendStatsPacket();
    void clearQueuedCheckIns() { _numQueuedCheckIns = 0; }

signals:
    void finished();

protected:
    void commonInit(const QString& targetName, NodeType_t nodeType);
    bool _isFinished;
    QTimer _domainServerTimer;
    QTimer _statsTimer;
    int _numQueuedCheckIns { 0 };
    
protected slots:
    void domainSettingsRequestFailed();
    
private slots:
    void checkInWithDomainServerOrExit();
};

typedef QSharedPointer<ThreadedAssignment> SharedAssignmentPointer;

#endif // hifi_ThreadedAssignment_h
