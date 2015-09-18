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

#include <udt/PacketHeaders.h>

#include "AvatarMixerClientData.h"

int AvatarMixerClientData::parseData(NLPacket& packet) {
    // pull the sequence number from the data first
    packet.readPrimitive(&_lastReceivedSequenceNumber);
    
    // compute the offset to the data payload
    return _avatar.parseDataFromBuffer(packet.readWithoutCopy(packet.bytesLeftToRead()));
}

bool AvatarMixerClientData::checkAndSetHasReceivedFirstPackets() {
    bool oldValue = _hasReceivedFirstPackets;
    _hasReceivedFirstPackets = true;
    return oldValue;
}

uint16_t AvatarMixerClientData::getLastBroadcastSequenceNumber(const QUuid& nodeUUID) const {
    // return the matching PacketSequenceNumber, or the default if we don't have it
    auto nodeMatch = _lastBroadcastSequenceNumbers.find(nodeUUID);
    if (nodeMatch != _lastBroadcastSequenceNumbers.end()) {
        return nodeMatch->second;
    } else {
        return 0;
    }
}

void AvatarMixerClientData::loadJSONStats(QJsonObject& jsonObject) const {
    jsonObject["display_name"] = _avatar.getDisplayName();
    jsonObject["full_rate_distance"] = _fullRateDistance;
    jsonObject["max_avatar_distance"] = _maxAvatarDistance;
    jsonObject["num_avatars_sent_last_frame"] = _numAvatarsSentLastFrame;
    jsonObject["avg_other_avatar_starves_per_second"] = getAvgNumOtherAvatarStarvesPerSecond();
    jsonObject["avg_other_avatar_skips_per_second"] = getAvgNumOtherAvatarSkipsPerSecond();
    jsonObject["total_num_out_of_order_sends"] = _numOutOfOrderSends;
    
    jsonObject[OUTBOUND_AVATAR_DATA_STATS_KEY] = getOutboundAvatarDataKbps();
}
