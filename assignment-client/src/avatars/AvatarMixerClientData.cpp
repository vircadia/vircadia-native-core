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

int AvatarMixerClientData::parseData(ReceivedMessage& message) {
    // pull the sequence number from the data first
    message.readPrimitive(&_lastReceivedSequenceNumber);
    
    // compute the offset to the data payload
    return _avatar->parseDataFromBuffer(message.readWithoutCopy(message.getBytesLeftToRead()));
}

bool AvatarMixerClientData::checkAndSetHasReceivedFirstPacketsFrom(const QUuid& uuid) {
    if (_hasReceivedFirstPacketsFrom.find(uuid) == _hasReceivedFirstPacketsFrom.end()) {
        _hasReceivedFirstPacketsFrom.insert(uuid);
        return false;
    }
    return true;
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
    jsonObject["display_name"] = _avatar->getDisplayName();
    jsonObject["full_rate_distance"] = _fullRateDistance;
    jsonObject["max_av_distance"] = _maxAvatarDistance;
    jsonObject["num_avs_sent_last_frame"] = _numAvatarsSentLastFrame;
    jsonObject["avg_other_av_starves_per_second"] = getAvgNumOtherAvatarStarvesPerSecond();
    jsonObject["avg_other_av_skips_per_second"] = getAvgNumOtherAvatarSkipsPerSecond();
    jsonObject["total_num_out_of_order_sends"] = _numOutOfOrderSends;

    jsonObject[OUTBOUND_AVATAR_DATA_STATS_KEY] = getOutboundAvatarDataKbps();
    jsonObject[INBOUND_AVATAR_DATA_STATS_KEY] = _avatar->getAverageBytesReceivedPerSecond() / (float) BYTES_PER_KILOBIT;

    jsonObject["av_data_receive_rate"] = _avatar->getReceiveRate();
}
