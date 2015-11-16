//
//  MessagesMixerClientData.cpp
//  assignment-client/src/messages
//
//  Created by Brad hefta-Gaub on 11/16/2015.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <udt/PacketHeaders.h>

#include "MessagesMixerClientData.h"

int MessagesMixerClientData::parseData(NLPacket& packet) {
    // pull the sequence number from the data first
    packet.readPrimitive(&_lastReceivedSequenceNumber);

    // compute the offset to the data payload
    return _avatar.parseDataFromBuffer(packet.readWithoutCopy(packet.bytesLeftToRead()));
}

bool MessagesMixerClientData::checkAndSetHasReceivedFirstPacketsFrom(const QUuid& uuid) {
    if (_hasReceivedFirstPacketsFrom.find(uuid) == _hasReceivedFirstPacketsFrom.end()) {
        _hasReceivedFirstPacketsFrom.insert(uuid);
        return false;
    }
    return true;
}

uint16_t MessagesMixerClientData::getLastBroadcastSequenceNumber(const QUuid& nodeUUID) const {
    // return the matching PacketSequenceNumber, or the default if we don't have it
    auto nodeMatch = _lastBroadcastSequenceNumbers.find(nodeUUID);
    if (nodeMatch != _lastBroadcastSequenceNumbers.end()) {
        return nodeMatch->second;
    } else {
        return 0;
    }
}

void MessagesMixerClientData::loadJSONStats(QJsonObject& jsonObject) const {
    jsonObject["display_name"] = _avatar.getDisplayName();
    jsonObject["full_rate_distance"] = _fullRateDistance;
    jsonObject["max_av_distance"] = _maxAvatarDistance;
    jsonObject["num_avs_sent_last_frame"] = _numAvatarsSentLastFrame;
    jsonObject["avg_other_av_starves_per_second"] = getAvgNumOtherAvatarStarvesPerSecond();
    jsonObject["avg_other_av_skips_per_second"] = getAvgNumOtherAvatarSkipsPerSecond();
    jsonObject["total_num_out_of_order_sends"] = _numOutOfOrderSends;

    jsonObject[OUTBOUND_MESSAGES_DATA_STATS_KEY] = getOutboundAvatarDataKbps();
    jsonObject[INBOUND_MESSAGES_DATA_STATS_KEY] = _avatar.getAverageBytesReceivedPerSecond() / (float) BYTES_PER_KILOBIT;

    jsonObject["av_data_receive_rate"] = _avatar.getReceiveRate();
}
