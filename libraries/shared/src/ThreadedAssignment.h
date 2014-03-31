//
//  ThreadedAssignment.h
//  hifi
//
//  Created by Stephen Birarda on 12/3/2013.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__ThreadedAssignment__
#define __hifi__ThreadedAssignment__

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

protected:
    bool readAvailableDatagram(QByteArray& destinationByteArray, HifiSockAddr& senderSockAddr);
    void commonInit(const QString& targetName, NodeType_t nodeType, bool shouldSendStats = true);
    bool _isFinished;
private slots:
    void checkInWithDomainServerOrExit();
signals:
    void finished();
};

typedef QSharedPointer<ThreadedAssignment> SharedAssignmentPointer;


#endif /* defined(__hifi__ThreadedAssignment__) */
