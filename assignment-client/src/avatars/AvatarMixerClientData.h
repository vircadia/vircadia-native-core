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

#include <QtCore/QJsonObject>
#include <QtCore/QUrl>

#include <AvatarData.h>
#include <NodeData.h>
#include <NumericalConstants.h>
#include <udt/PacketHeaders.h>
#include <SimpleMovingAverage.h>
#include <UUIDHasher.h>

const QString OUTBOUND_AVATAR_DATA_STATS_KEY = "outbound_av_data_kbps";

class AvatarMixerClientData : public NodeData {
    Q_OBJECT
public:
    int parseData(NLPacket& packet);
    AvatarData& getAvatar() { return _avatar; }
    
    bool checkAndSetHasReceivedFirstPackets();

    uint16_t getLastBroadcastSequenceNumber(const QUuid& nodeUUID) const;
    void setLastBroadcastSequenceNumber(const QUuid& nodeUUID, uint16_t sequenceNumber)
        { _lastBroadcastSequenceNumbers[nodeUUID] = sequenceNumber; }
    Q_INVOKABLE void removeLastBroadcastSequenceNumber(const QUuid& nodeUUID) { _lastBroadcastSequenceNumbers.erase(nodeUUID); }
    
    uint16_t getLastReceivedSequenceNumber() const { return _lastReceivedSequenceNumber; }

    quint64 getBillboardChangeTimestamp() const { return _billboardChangeTimestamp; }
    void setBillboardChangeTimestamp(quint64 billboardChangeTimestamp) { _billboardChangeTimestamp = billboardChangeTimestamp; }
    
    quint64 getIdentityChangeTimestamp() const { return _identityChangeTimestamp; }
    void setIdentityChangeTimestamp(quint64 identityChangeTimestamp) { _identityChangeTimestamp = identityChangeTimestamp; }
   
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
private:
    AvatarData _avatar;
    
    uint16_t _lastReceivedSequenceNumber { 0 };
    std::unordered_map<QUuid, uint16_t> _lastBroadcastSequenceNumbers;

    bool _hasReceivedFirstPackets = false;
    quint64 _billboardChangeTimestamp = 0;
    quint64 _identityChangeTimestamp = 0;
    
    float _fullRateDistance = FLT_MAX;
    float _maxAvatarDistance = FLT_MAX;
    
    int _numAvatarsSentLastFrame = 0;
    int _numFramesSinceAdjustment = 0;

    SimpleMovingAverage _otherAvatarStarves;
    SimpleMovingAverage _otherAvatarSkips;
    int _numOutOfOrderSends = 0;
    
    SimpleMovingAverage _avgOtherAvatarDataRate;
};

#endif // hifi_AvatarMixerClientData_h
