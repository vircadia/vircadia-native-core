//
//  AvatarMixerClientData.cpp
//  assignment-client/src/avatars
//
//  Created by Stephen Birarda on 2/4/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <PacketHeaders.h>

#include "AvatarMixerClientData.h"

int AvatarMixerClientData::parseData(const QByteArray& packet) {
    // compute the offset to the data payload
    int offset = numBytesForPacketHeader(packet);
    return _avatar.parseDataAtOffset(packet, offset);
}

bool AvatarMixerClientData::checkAndSetHasReceivedFirstPackets() {
    bool oldValue = _hasReceivedFirstPackets;
    _hasReceivedFirstPackets = true;
    return oldValue;
}

void AvatarMixerClientData::loadJSONStats(QJsonObject& jsonObject) const {
    jsonObject["display_name"] = _avatar.getDisplayName();
    jsonObject["full_rate_distance"] = _fullRateDistance;
    jsonObject["max_avatar_distance"] = _maxAvatarDistance;
    jsonObject["num_avatars_sent_last_frame"] = _numAvatarsSentLastFrame;
    
    jsonObject[OUTBOUND_AVATAR_DATA_STATS_KEY] = getOutboundAvatarDataKbps();
}
