//
//  AvatarMixerClientData.h
//  assignment-client/src/avatars
//
//  Created by Stephen Birarda on 2/4/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AvatarMixerClientData_h
#define hifi_AvatarMixerClientData_h

#include <algorithm>
#include <cfloat>
#include <unordered_map>
#include <vector>
#include <queue>

#include <QtCore/QJsonObject>
#include <QtCore/QUrl>

#include <AvatarData.h>
#include <AssociatedTraitValues.h>
#include <NodeData.h>
#include <NumericalConstants.h>
#include <udt/PacketHeaders.h>
#include <PortableHighResolutionClock.h>
#include <SimpleMovingAverage.h>
#include <UUIDHasher.h>
#include <shared/ConicalViewFrustum.h>

const QString OUTBOUND_AVATAR_DATA_STATS_KEY = "outbound_av_data_kbps";
const QString INBOUND_AVATAR_DATA_STATS_KEY = "inbound_av_data_kbps";

struct SlaveSharedData;

class AvatarMixerClientData : public NodeData {
    Q_OBJECT
public:
    AvatarMixerClientData(const QUuid& nodeID, Node::LocalID nodeLocalID);
    virtual ~AvatarMixerClientData() {}
    using HRCTime = p_high_resolution_clock::time_point;

    int parseData(ReceivedMessage& message) override;
    AvatarData& getAvatar() { return *_avatar; }
    const AvatarData& getAvatar() const { return *_avatar; }
    const AvatarData* getConstAvatarData() const { return _avatar.get(); }
    AvatarSharedPointer getAvatarSharedPointer() const { return _avatar; }

    uint16_t getLastBroadcastSequenceNumber(const QUuid& nodeUUID) const;
    void setLastBroadcastSequenceNumber(const QUuid& nodeUUID, uint16_t sequenceNumber)
        { _lastBroadcastSequenceNumbers[nodeUUID] = sequenceNumber; }
    Q_INVOKABLE void removeLastBroadcastSequenceNumber(const QUuid& nodeUUID) { _lastBroadcastSequenceNumbers.erase(nodeUUID); }

    uint64_t getLastBroadcastTime(const QUuid& nodeUUID) const;
    void setLastBroadcastTime(const QUuid& nodeUUID, uint64_t broadcastTime) { _lastBroadcastTimes[nodeUUID] = broadcastTime; }
    Q_INVOKABLE void removeLastBroadcastTime(const QUuid& nodeUUID) { _lastBroadcastTimes.erase(nodeUUID); }

    Q_INVOKABLE void cleanupKilledNode(const QUuid& nodeUUID, Node::LocalID nodeLocalID);

    uint16_t getLastReceivedSequenceNumber() const { return _lastReceivedSequenceNumber; }

    uint64_t getIdentityChangeTimestamp() const { return _identityChangeTimestamp; }
    void flagIdentityChange() { _identityChangeTimestamp = usecTimestampNow(); }
    bool getAvatarSessionDisplayNameMustChange() const { return _avatarSessionDisplayNameMustChange; }
    void setAvatarSessionDisplayNameMustChange(bool set = true) { _avatarSessionDisplayNameMustChange = set; }

    void resetNumAvatarsSentLastFrame() { _numAvatarsSentLastFrame = 0; }
    void incrementNumAvatarsSentLastFrame() { ++_numAvatarsSentLastFrame; }
    int getNumAvatarsSentLastFrame() const { return _numAvatarsSentLastFrame; }

    void recordNumOtherAvatarStarves(int numAvatarsHeldBack) { _otherAvatarStarves.updateAverage((float) numAvatarsHeldBack); }
    float getAvgNumOtherAvatarStarvesPerSecond() const { return _otherAvatarStarves.getAverageSampleValuePerSecond(); }

    void recordNumOtherAvatarSkips(int numOtherAvatarSkips) { _otherAvatarSkips.updateAverage((float) numOtherAvatarSkips); }
    float getAvgNumOtherAvatarSkipsPerSecond() const { return _otherAvatarSkips.getAverageSampleValuePerSecond(); }

    void incrementNumOutOfOrderSends() { ++_numOutOfOrderSends; }

    int getNumFramesSinceFRDAdjustment() const { return _numFramesSinceAdjustment; }
    void incrementNumFramesSinceFRDAdjustment() { ++_numFramesSinceAdjustment; }
    void resetNumFramesSinceFRDAdjustment() { _numFramesSinceAdjustment = 0; }

    void recordSentAvatarData(int numBytes) { _avgOtherAvatarDataRate.updateAverage((float) numBytes); }

    float getOutboundAvatarDataKbps() const
        { return _avgOtherAvatarDataRate.getAverageSampleValuePerSecond() / (float) BYTES_PER_KILOBIT; }

    void loadJSONStats(QJsonObject& jsonObject) const;

    glm::vec3 getPosition() const { return _avatar ? _avatar->getWorldPosition() : glm::vec3(0); }
    bool isRadiusIgnoring(const QUuid& other) const;
    void addToRadiusIgnoringSet(const QUuid& other);
    void removeFromRadiusIgnoringSet(const QUuid& other);
    void ignoreOther(SharedNodePointer self, SharedNodePointer other);
    void ignoreOther(const Node* self, const Node* other);

    void readViewFrustumPacket(const QByteArray& message);

    bool otherAvatarInView(const AABox& otherAvatarBox);

    void resetInViewStats() { _recentOtherAvatarsInView = _recentOtherAvatarsOutOfView = 0; }
    void incrementAvatarInView() { _recentOtherAvatarsInView++; }
    void incrementAvatarOutOfView() { _recentOtherAvatarsOutOfView++; }
    const QString& getBaseDisplayName() { return _baseDisplayName; }
    void setBaseDisplayName(const QString& baseDisplayName) { _baseDisplayName = baseDisplayName; }
    bool getRequestsDomainListData() { return _requestsDomainListData; }
    void setRequestsDomainListData(bool requesting) { _requestsDomainListData = requesting; }

    const ConicalViewFrustums& getViewFrustums() const { return _currentViewFrustums; }

    uint64_t getLastOtherAvatarEncodeTime(QUuid otherAvatar) const;
    void setLastOtherAvatarEncodeTime(const QUuid& otherAvatar, uint64_t time);

    QVector<JointData>& getLastOtherAvatarSentJoints(QUuid otherAvatar) { return _lastOtherAvatarSentJoints[otherAvatar]; }

    void queuePacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer node);
    int processPackets(const SlaveSharedData& slaveSharedData); // returns number of packets processed

    void processSetTraitsMessage(ReceivedMessage& message, const SlaveSharedData& slaveSharedData, Node& sendingNode);
    void checkSkeletonURLAgainstWhitelist(const SlaveSharedData& slaveSharedData, Node& sendingNode,
                                          AvatarTraits::TraitVersion traitVersion);

    using TraitsCheckTimestamp = std::chrono::steady_clock::time_point;

    TraitsCheckTimestamp getLastReceivedTraitsChange() const { return _lastReceivedTraitsChange; }

    AvatarTraits::TraitVersions& getLastReceivedTraitVersions() { return _lastReceivedTraitVersions; }
    const AvatarTraits::TraitVersions& getLastReceivedTraitVersions() const { return _lastReceivedTraitVersions; }

    TraitsCheckTimestamp getLastOtherAvatarTraitsSendPoint(Node::LocalID otherAvatar) const;
    void setLastOtherAvatarTraitsSendPoint(Node::LocalID otherAvatar, TraitsCheckTimestamp sendPoint)
        { _lastSentTraitsTimestamps[otherAvatar] = sendPoint; }

    AvatarTraits::TraitVersions& getLastSentTraitVersions(Node::LocalID otherAvatar) { return _sentTraitVersions[otherAvatar]; }

    void resetSentTraitData(Node::LocalID nodeID);

private:
    struct PacketQueue : public std::queue<QSharedPointer<ReceivedMessage>> {
        QWeakPointer<Node> node;
    };
    PacketQueue _packetQueue;

    AvatarSharedPointer _avatar { new AvatarData() };

    uint16_t _lastReceivedSequenceNumber { 0 };
    std::unordered_map<QUuid, uint16_t> _lastBroadcastSequenceNumbers;
    std::unordered_map<QUuid, uint64_t> _lastBroadcastTimes;

    // this is a map of the last time we encoded an "other" avatar for
    // sending to "this" node
    std::unordered_map<QUuid, uint64_t> _lastOtherAvatarEncodeTime;
    std::unordered_map<QUuid, QVector<JointData>> _lastOtherAvatarSentJoints;

    uint64_t _identityChangeTimestamp;
    bool _avatarSessionDisplayNameMustChange{ true };
    bool _avatarSkeletonModelUrlMustChange{ false };

    int _numAvatarsSentLastFrame = 0;
    int _numFramesSinceAdjustment = 0;

    SimpleMovingAverage _otherAvatarStarves;
    SimpleMovingAverage _otherAvatarSkips;
    int _numOutOfOrderSends = 0;

    SimpleMovingAverage _avgOtherAvatarDataRate;
    std::vector<QUuid> _radiusIgnoredOthers;
    ConicalViewFrustums _currentViewFrustums;

    int _recentOtherAvatarsInView { 0 };
    int _recentOtherAvatarsOutOfView { 0 };
    QString _baseDisplayName{}; // The santized key used in determinging unique sessionDisplayName, so that we can remove from dictionary.
    bool _requestsDomainListData { false };

    AvatarTraits::TraitVersions _lastReceivedTraitVersions;
    TraitsCheckTimestamp _lastReceivedTraitsChange;

    std::unordered_map<Node::LocalID, TraitsCheckTimestamp> _lastSentTraitsTimestamps;
    std::unordered_map<Node::LocalID, AvatarTraits::TraitVersions> _sentTraitVersions;
};

#endif // hifi_AvatarMixerClientData_h
