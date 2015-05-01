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

#include <QtCore/QJsonObject>
#include <QtCore/QUrl>

#include <AvatarData.h>
#include <NodeData.h>
#include <NumericalConstants.h>
#include <SimpleMovingAverage.h>

const QString OUTBOUND_AVATAR_DATA_STATS_KEY = "outbound_av_data_kbps";

class AvatarMixerClientData : public NodeData {
    Q_OBJECT
public:
    int parseData(const QByteArray& packet);
    AvatarData& getAvatar() { return _avatar; }
    
    bool checkAndSetHasReceivedFirstPackets();
    
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

    int getNumFramesSinceFRDAdjustment() const { return _numFramesSinceAdjustment; }
    void incrementNumFramesSinceFRDAdjustment() { ++_numFramesSinceAdjustment; }
    void resetNumFramesSinceFRDAdjustment() { _numFramesSinceAdjustment = 0; }

    void recordSentAvatarData(int numBytes) { _avgOtherAvatarDataRate.updateAverage((float) numBytes); }
   
    float getOutboundAvatarDataKbps() const 
        { return _avgOtherAvatarDataRate.getAverageSampleValuePerSecond() / (float) BYTES_PER_KILOBIT; }
    
    void loadJSONStats(QJsonObject& jsonObject) const;
private:
    AvatarData _avatar;
    bool _hasReceivedFirstPackets = false;
    quint64 _billboardChangeTimestamp = 0;
    quint64 _identityChangeTimestamp = 0;
    float _fullRateDistance = FLT_MAX;
    float _maxAvatarDistance = FLT_MAX;
    int _numAvatarsSentLastFrame = 0;
    int _numFramesSinceAdjustment = 0;
    SimpleMovingAverage _avgOtherAvatarDataRate;
};

#endif // hifi_AvatarMixerClientData_h
