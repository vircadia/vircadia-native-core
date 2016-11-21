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
#include <unordered_set>

#include <QtCore/QJsonObject>
#include <QtCore/QUrl>

#include <AvatarData.h>
#include <NodeData.h>
#include <NumericalConstants.h>
#include <udt/PacketHeaders.h>
#include <PortableHighResolutionClock.h>
#include <SimpleMovingAverage.h>
#include <UUIDHasher.h>

const QString OUTBOUND_AVATAR_DATA_STATS_KEY = "outbound_av_data_kbps";
const QString INBOUND_AVATAR_DATA_STATS_KEY = "inbound_av_data_kbps";

class AvatarMixerClientData : public NodeData {
    Q_OBJECT
public:
    using HRCTime = p_high_resolution_clock::time_point;

    int parseData(ReceivedMessage& message) override;
    AvatarData& getAvatar() { return *_avatar; }

    bool checkAndSetHasReceivedFirstPacketsFrom(const QUuid& uuid);

    uint16_t getLastBroadcastSequenceNumber(const QUuid& nodeUUID) const;
    void setLastBroadcastSequenceNumber(const QUuid& nodeUUID, uint16_t sequenceNumber)
        { _lastBroadcastSequenceNumbers[nodeUUID] = sequenceNumber; }
    Q_INVOKABLE void removeLastBroadcastSequenceNumber(const QUuid& nodeUUID) { _lastBroadcastSequenceNumbers.erase(nodeUUID); }

    uint16_t getLastReceivedSequenceNumber() const { return _lastReceivedSequenceNumber; }

    HRCTime getIdentityChangeTimestamp() const { return _identityChangeTimestamp; }
    void flagIdentityChange() { _identityChangeTimestamp = p_high_resolution_clock::now(); }

    void setFullRateDistance(float fullRateDistance) { _fullRateDistance = fullRateDistance; }
    float getFullRateDistance() const { return _fullRateDistance; }

    void setMaxAvatarDistance(float maxAvatarDistance) { _maxAvatarDistance = maxAvatarDistance; }
    float getMaxAvatarDistance() const { return _maxAvatarDistance; }

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

    glm::vec3 getPosition() { return _avatar ? _avatar->getPosition() : glm::vec3(0); }
    bool isRadiusIgnoring(const QUuid& other) { return _radiusIgnoredOthers.find(other) != _radiusIgnoredOthers.end(); }
    void addToRadiusIgnoringSet(const QUuid& other) { _radiusIgnoredOthers.insert(other); }
    void removeFromRadiusIgnoringSet(const QUuid& other) { _radiusIgnoredOthers.erase(other); }
    void ignoreOther(SharedNodePointer self, SharedNodePointer other);

private:
    AvatarSharedPointer _avatar { new AvatarData() };

    uint16_t _lastReceivedSequenceNumber { 0 };
    std::unordered_map<QUuid, uint16_t> _lastBroadcastSequenceNumbers;
    std::unordered_set<QUuid> _hasReceivedFirstPacketsFrom;

    HRCTime _identityChangeTimestamp;

    float _fullRateDistance = FLT_MAX;
    float _maxAvatarDistance = FLT_MAX;

    int _numAvatarsSentLastFrame = 0;
    int _numFramesSinceAdjustment = 0;

    SimpleMovingAverage _otherAvatarStarves;
    SimpleMovingAverage _otherAvatarSkips;
    int _numOutOfOrderSends = 0;

    SimpleMovingAverage _avgOtherAvatarDataRate;
    std::unordered_set<QUuid> _radiusIgnoredOthers;
};

#endif // hifi_AvatarMixerClientData_h
