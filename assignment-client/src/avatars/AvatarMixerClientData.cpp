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

#include <DependencyManager.h>
#include <NodeList.h>

#include "AvatarMixerClientData.h"

int AvatarMixerClientData::parseData(ReceivedMessage& message) {
    // pull the sequence number from the data first
    message.readPrimitive(&_lastReceivedSequenceNumber);
    
    // compute the offset to the data payload
    //qDebug() << __FUNCTION__ "about to call parseDataFromBuffer() for:" << getNodeID();
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

void AvatarMixerClientData::ignoreOther(SharedNodePointer self, SharedNodePointer other) {
    if (!isRadiusIgnoring(other->getUUID())) {
        addToRadiusIgnoringSet(other->getUUID());
        auto killPacket = NLPacket::create(PacketType::KillAvatar, NUM_BYTES_RFC4122_UUID + sizeof(KillAvatarReason));
        killPacket->write(other->getUUID().toRfc4122());
        if (self->isIgnoreRadiusEnabled()) {
            killPacket->writePrimitive(KillAvatarReason::TheirAvatarEnteredYourBubble);
        } else {
            killPacket->writePrimitive(KillAvatarReason::YourAvatarEnteredTheirBubble);
        }
        DependencyManager::get<NodeList>()->sendUnreliablePacket(*killPacket, *self);
        _hasReceivedFirstPacketsFrom.erase(other->getUUID());
    }
}

void AvatarMixerClientData::readViewFrustumPacket(const QByteArray& message) {
    _currentViewFrustum.fromByteArray(message);
}

bool AvatarMixerClientData::otherAvatarInView(const AABox& otherAvatarBox) {
    return _currentViewFrustum.boxIntersectsKeyhole(otherAvatarBox);
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
    jsonObject["recent_other_av_in_view"] = _recentOtherAvatarsInView;
    jsonObject["recent_other_av_out_of_view"] = _recentOtherAvatarsOutOfView;
}
