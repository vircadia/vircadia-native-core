//
//  AssignmentClient.h
//  assignment-client/src
//
//  Created by Stephen Birarda on 11/25/2013.
//  Copyright 2013 High Fidelity, Inc.
//  Copyright 2021 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AssignmentClient_h
#define hifi_AssignmentClient_h

#include <QtCore/QCoreApplication>
#include <QtCore/QPointer>
#include <QtCore/QSharedPointer>

#include <shared/WebRTC.h>

#include "ThreadedAssignment.h"

class QSharedMemory;

class AssignmentClient : public QObject {
    Q_OBJECT
public:
    AssignmentClient(Assignment::Type requestAssignmentType, QString assignmentPool,
                     quint16 listenPort, QUuid walletUUID, QString assignmentServerHostname,
                     quint16 assignmentServerPort, quint16 assignmentMonitorPort,
                     bool disableDomainPortAutoDiscovery);
    ~AssignmentClient();

public slots:
    void aboutToQuit();

private slots:
    void sendAssignmentRequest();
    void assignmentCompleted();
    void handleAuthenticationRequest();
    void sendStatusPacketToACM();
    void stopAssignmentClient();
    void handleCreateAssignmentPacket(QSharedPointer<ReceivedMessage> message);
    void handleStopNodePacket(QSharedPointer<ReceivedMessage> message);
#if defined(WEBRTC_DATA_CHANNELS)
    void handleWebRTCSignalingPacket(QSharedPointer<ReceivedMessage> message);
    void sendSignalingMessageToUserClient(const QJsonObject& json);
#endif

signals:
#if defined(WEBRTC_DATA_CHANNELS)
    void webrtcSignalingMessageFromUserClient(const QJsonObject& json);
#endif

private:
    void setUpStatusToMonitor();

    Assignment _requestAssignment;
    QPointer<ThreadedAssignment> _currentAssignment;
    bool _isAssigned { false };
    QString _assignmentServerHostname;
    SockAddr _assignmentServerSocket;
    QTimer _requestTimer; // timer for requesting and assignment
    QTimer _statsTimerACM; // timer for sending stats to assignment client monitor
    QUuid _childAssignmentUUID = QUuid::createUuid();
    bool _disableDomainPortAutoDiscovery { false };

 protected:
    SockAddr _assignmentClientMonitorSocket;
};

#endif // hifi_AssignmentClient_h
