//
//  AssignmentClient.h
//  assignment-client/src
//
//  Created by Stephen Birarda on 11/25/2013.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AssignmentClient_h
#define hifi_AssignmentClient_h

#include <QtCore/QCoreApplication>

#include "ThreadedAssignment.h"

class QSharedMemory;

class AssignmentClient : public QCoreApplication {
    Q_OBJECT
public:
    AssignmentClient(int &argc, char **argv, QUuid nodeUUID);
    static const SharedAssignmentPointer& getCurrentAssignment() { return _currentAssignment; }

private slots:
    void sendAssignmentRequest();
    void readPendingDatagrams();
    void assignmentCompleted();
    void handleAuthenticationRequest();
    void sendStatsPacketToACM();

private:
    void setUpStatsToMonitor();
    Assignment _requestAssignment;
    static SharedAssignmentPointer _currentAssignment;
    QString _assignmentServerHostname;
    HifiSockAddr _assignmentServerSocket;
    QSharedMemory* _localASPortSharedMem; // memory shared with domain server
    QSharedMemory* _localACMPortSharedMem; // memory shared with assignment client monitor
    QTimer _requestTimer; // timer for requesting and assignment
    QTimer _statsTimerACM; // timer for sending stats to assignment client monitor

 protected:
    HifiSockAddr _assignmentClientMonitorSocket;
};

#endif // hifi_AssignmentClient_h
