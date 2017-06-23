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

#include <shared/RateCounter.h>
#include <PortableHighResolutionClock.h>

#include <ThreadedAssignment.h>
#include "AvatarMixerClientData.h"

#include "AvatarMixerSlavePool.h"

/// Handles assignments of type AvatarMixer - distribution of avatar data to various clients
class AvatarMixer : public ThreadedAssignment {
    Q_OBJECT
public:
    AvatarMixer(ReceivedMessage& message);

    static bool shouldReplicateTo(const Node& from, const Node& to) {
        return to.getType() == NodeType::DownstreamAvatarMixer &&
               to.getPublicSocket() != from.getPublicSocket() &&
               to.getLocalSocket() != from.getLocalSocket();
    }

public slots:
    /// runs the avatar mixer
    void run() override;

    void nodeKilled(SharedNodePointer killedNode);

    void sendStatsPacket() override;

private slots:
    void queueIncomingPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer node);
    void handleAdjustAvatarSorting(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode);
    void handleViewFrustumPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode);
    void handleAvatarIdentityPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode);
    void handleKillAvatarPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode);
    void handleNodeIgnoreRequestPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode);
    void handleRadiusIgnoreRequestPacket(QSharedPointer<ReceivedMessage> packet, SharedNodePointer sendingNode);
    void handleRequestsDomainListDataPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode);
    void handleReplicatedPacket(QSharedPointer<ReceivedMessage> message);
    void handleReplicatedBulkAvatarPacket(QSharedPointer<ReceivedMessage> message);
    void domainSettingsRequestComplete();
    void handlePacketVersionMismatch(PacketType type, const HifiSockAddr& senderSockAddr, const QUuid& senderUUID);
    void start();


private:
    AvatarMixerClientData* getOrCreateClientData(SharedNodePointer node);
    std::chrono::microseconds timeFrame(p_high_resolution_clock::time_point& timestamp);
    void throttle(std::chrono::microseconds duration, int frame);

    void parseDomainServerSettings(const QJsonObject& domainSettings);
    void sendIdentityPacket(AvatarMixerClientData* nodeData, const SharedNodePointer& destinationNode);

    void manageIdentityData(const SharedNodePointer& node);
    bool isAvatarInWhitelist(const QUrl& url);

    const QString REPLACEMENT_AVATAR_DEFAULT{ "" };
    QStringList _avatarWhitelist { };
    QString _replacementAvatar { REPLACEMENT_AVATAR_DEFAULT };

    void optionallyReplicatePacket(ReceivedMessage& message, const Node& node);

    p_high_resolution_clock::time_point _lastFrameTimestamp;

    // FIXME - new throttling - use these values somehow
    float _trailingMixRatio { 0.0f };
    float _throttlingRatio { 0.0f };


    int _sumListeners { 0 };
    int _numStatFrames { 0 };
    int _numTightLoopFrames { 0 };
    int _sumIdentityPackets { 0 };

    float _maxKbpsPerNode = 0.0f;

    float _domainMinimumScale { MIN_AVATAR_SCALE };
    float _domainMaximumScale { MAX_AVATAR_SCALE };

    RateCounter<> _broadcastRate;
    p_high_resolution_clock::time_point _lastDebugMessage;
    QHash<QString, QPair<int, int>> _sessionDisplayNames;

    quint64 _displayNameManagementElapsedTime { 0 }; // total time spent in broadcastAvatarData/display name management... since last stats window
    quint64 _ignoreCalculationElapsedTime { 0 };
    quint64 _avatarDataPackingElapsedTime { 0 };
    quint64 _packetSendingElapsedTime { 0 };

    quint64 _broadcastAvatarDataElapsedTime { 0 }; // total time spent in broadcastAvatarData since last stats window
    quint64 _broadcastAvatarDataInner { 0 };
    quint64 _broadcastAvatarDataLockWait { 0 };
    quint64 _broadcastAvatarDataNodeTransform { 0 };
    quint64 _broadcastAvatarDataNodeFunctor { 0 };

    quint64 _handleAdjustAvatarSortingElapsedTime { 0 };
    quint64 _handleViewFrustumPacketElapsedTime { 0 };
    quint64 _handleAvatarIdentityPacketElapsedTime { 0 };
    quint64 _handleKillAvatarPacketElapsedTime { 0 };
    quint64 _handleNodeIgnoreRequestPacketElapsedTime { 0 };
    quint64 _handleRadiusIgnoreRequestPacketElapsedTime { 0 };
    quint64 _handleRequestsDomainListDataPacketElapsedTime { 0 };
    quint64 _processQueuedAvatarDataPacketsElapsedTime { 0 };
    quint64 _processQueuedAvatarDataPacketsLockWaitElapsedTime { 0 };

    quint64 _processEventsElapsedTime { 0 };
    quint64 _sendStatsElapsedTime { 0 };
    quint64 _queueIncomingPacketElapsedTime { 0 };
    quint64 _lastStatsTime { usecTimestampNow() };

    RateCounter<> _loopRate; // this is the rate that the main thread tight loop runs


    AvatarMixerSlavePool _slavePool;

};

#endif // hifi_AvatarMixer_h
