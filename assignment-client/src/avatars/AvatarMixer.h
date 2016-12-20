//
//  AvatarMixer.h
//  assignment-client/src/avatars
//
//  Created by Stephen Birarda on 9/5/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  The avatar mixer receives head, hand and positional data from all connected
//  nodes, and broadcasts that data back to them, every BROADCAST_INTERVAL ms.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AvatarMixer_h
#define hifi_AvatarMixer_h

#include <PortableHighResolutionClock.h>

#include <ThreadedAssignment.h>
#include "AvatarMixerClientData.h"

/// Handles assignments of type AvatarMixer - distribution of avatar data to various clients
class AvatarMixer : public ThreadedAssignment {
    Q_OBJECT
public:
    AvatarMixer(ReceivedMessage& message);
    ~AvatarMixer();
public slots:
    /// runs the avatar mixer
    void run() override;

    void nodeKilled(SharedNodePointer killedNode);

    void sendStatsPacket() override;

private slots:
    void handleAvatarDataPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode);
    void handleAvatarIdentityPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode);
    void handleKillAvatarPacket(QSharedPointer<ReceivedMessage> message);
    void handleNodeIgnoreRequestPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode);
    void handleRadiusIgnoreRequestPacket(QSharedPointer<ReceivedMessage> packet, SharedNodePointer sendingNode);
    void domainSettingsRequestComplete();
    void handlePacketVersionMismatch(PacketType type, const HifiSockAddr& senderSockAddr, const QUuid& senderUUID);


private:
    void broadcastAvatarData();
    void parseDomainServerSettings(const QJsonObject& domainSettings);
    void sendIdentityPacket(AvatarMixerClientData* nodeData, const SharedNodePointer& destinationNode);

    QThread _broadcastThread;

    p_high_resolution_clock::time_point _lastFrameTimestamp;

    float _trailingSleepRatio { 1.0f };
    float _performanceThrottlingRatio { 0.0f };

    int _sumListeners { 0 };
    int _numStatFrames { 0 };
    int _sumIdentityPackets { 0 };

    float _maxKbpsPerNode = 0.0f;

    float _domainMinimumScale { MIN_AVATAR_SCALE };
    float _domainMaximumScale { MAX_AVATAR_SCALE };

    QTimer* _broadcastTimer = nullptr;

    QHash<QString, QPair<int, int>> _sessionDisplayNames;
};

#endif // hifi_AvatarMixer_h
