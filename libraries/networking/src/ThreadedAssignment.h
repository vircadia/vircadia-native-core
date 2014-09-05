//
//  ThreadedAssignment.h
//  libraries/shared/src
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

#include "Assignment.h"

class ThreadedAssignment : public Assignment {
    Q_OBJECT
public:
    ThreadedAssignment(const QByteArray& packet);
    
    void setFinished(bool isFinished);
    virtual void aboutToFinish() { };
    void addPacketStatsAndSendStatsPacket(QJsonObject& statsObject);

public slots:
    /// threaded run of assignment
    virtual void run() = 0;
    virtual void readPendingDatagrams() = 0;
    virtual void sendStatsPacket();
signals:
    void finished();
    
protected:
    bool readAvailableDatagram(QByteArray& destinationByteArray, HifiSockAddr& senderSockAddr);
    void commonInit(const QString& targetName, NodeType_t nodeType, bool shouldSendStats = true);
    bool _isFinished;
    QThread* _datagramProcessingThread;
    
private slots:
    void checkInWithDomainServerOrExit();

};

typedef QSharedPointer<ThreadedAssignment> SharedAssignmentPointer;

#endif // hifi_ThreadedAssignment_h
