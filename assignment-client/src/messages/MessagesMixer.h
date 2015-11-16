//
//  MessagesMixer.h
//  assignment-client/src/messages
//
//  Created by Brad hefta-Gaub on 11/16/2015.
//  Copyright 2015 High Fidelity, Inc.
//
//  The avatar mixer receives head, hand and positional data from all connected
//  nodes, and broadcasts that data back to them, every BROADCAST_INTERVAL ms.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_MessagesMixer_h
#define hifi_MessagesMixer_h

#include <ThreadedAssignment.h>

/// Handles assignments of type MessagesMixer - distribution of avatar data to various clients
class MessagesMixer : public ThreadedAssignment {
    Q_OBJECT
public:
    MessagesMixer(NLPacket& packet);
    ~MessagesMixer();
public slots:
    /// runs the avatar mixer
    void run();

    void nodeKilled(SharedNodePointer killedNode);
    
    void sendStatsPacket();

private slots:
    void handleMessagesDataPacket(QSharedPointer<NLPacket> packet, SharedNodePointer senderNode);
    
private:
    void broadcastMessagesData();
    void parseDomainServerSettings(const QJsonObject& domainSettings);
    
    QThread _broadcastThread;
    
    quint64 _lastFrameTimestamp;
    
    float _trailingSleepRatio;
    float _performanceThrottlingRatio;
    
    int _sumListeners;
    int _numStatFrames;
    int _sumBillboardPackets;
    int _sumIdentityPackets;

    float _maxKbpsPerNode = 0.0f;

    QTimer* _broadcastTimer = nullptr;
};

#endif // hifi_MessagesMixer_h
