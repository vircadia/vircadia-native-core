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

#include "AvatarMixerClientData.h"

#include <udt/PacketHeaders.h>

#include <DependencyManager.h>
#include <NodeList.h>

AvatarMixerClientData::AvatarMixerClientData(const QUuid& nodeID) :
    NodeData(nodeID)
{
    // in case somebody calls getSessionUUID on the AvatarData instance, make sure it has the right ID
    _avatar->setID(nodeID);
}

uint64_t AvatarMixerClientData::getLastOtherAvatarEncodeTime(QUuid otherAvatar) const {
    std::unordered_map<QUuid, uint64_t>::const_iterator itr = _lastOtherAvatarEncodeTime.find(otherAvatar);
    if (itr != _lastOtherAvatarEncodeTime.end()) {
        return itr->second;
    }
    return 0;
}

void AvatarMixerClientData::setLastOtherAvatarEncodeTime(const QUuid& otherAvatar, uint64_t time) {
    std::unordered_map<QUuid, uint64_t>::iterator itr = _lastOtherAvatarEncodeTime.find(otherAvatar);
    if (itr != _lastOtherAvatarEncodeTime.end()) {
        itr->second = time;
    } else {
        _lastOtherAvatarEncodeTime.emplace(std::pair<QUuid, uint64_t>(otherAvatar, time));
    }
}

void AvatarMixerClientData::queuePacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer node) {
    if (!_packetQueue.node) {
        _packetQueue.node = node;
    }
    _packetQueue.push(message);
}

int AvatarMixerClientData::processPackets() {
    int packetsProcessed = 0;
    SharedNodePointer node = _packetQueue.node;
    assert(_packetQueue.empty() || node);
    _packetQueue.node.clear();

    while (!_packetQueue.empty()) {
        auto& packet = _packetQueue.front();

        packetsProcessed++;

        switch (packet->getType()) {
            case PacketType::AvatarData:
                parseData(*packet);
                break;
            default:
                Q_UNREACHABLE();
        }
        _packetQueue.pop();
    }
    assert(_packetQueue.empty());

    return packetsProcessed;
}

int AvatarMixerClientData::parseData(ReceivedMessage& message) {

    // pull the sequence number from the data first
    uint16_t sequenceNumber;

    message.readPrimitive(&sequenceNumber);
    
    if (sequenceNumber < _lastReceivedSequenceNumber && _lastReceivedSequenceNumber != UINT16_MAX) {
        incrementNumOutOfOrderSends();
    }
    _lastReceivedSequenceNumber = sequenceNumber;

    // compute the offset to the data payload
    return _avatar->parseDataFromBuffer(message.readWithoutCopy(message.getBytesLeftToRead()));
}
uint64_t AvatarMixerClientData::getLastBroadcastTime(const QUuid& nodeUUID) const {
    // return the matching PacketSequenceNumber, or the default if we don't have it
    auto nodeMatch = _lastBroadcastTimes.find(nodeUUID);
    if (nodeMatch != _lastBroadcastTimes.end()) {
        return nodeMatch->second;
    }
    return 0;
}

uint16_t AvatarMixerClientData::getLastBroadcastSequenceNumber(const QUuid& nodeUUID) const {
    // return the matching PacketSequenceNumber, or the default if we don't have it
    auto nodeMatch = _lastBroadcastSequenceNumbers.find(nodeUUID);
    if (nodeMatch != _lastBroadcastSequenceNumbers.end()) {
        return nodeMatch->second;
    }
    return 0;
}

void AvatarMixerClientData::ignoreOther(SharedNodePointer self, SharedNodePointer other) {
    if (!isRadiusIgnoring(other->getUUID())) {
        addToRadiusIgnoringSet(other->getUUID());
        auto killPacket = NLPacket::create(PacketType::KillAvatar, NUM_BYTES_RFC4122_UUID + sizeof(KillAvatarReason), true);
        killPacket->write(other->getUUID().toRfc4122());
        if (self->isIgnoreRadiusEnabled()) {
            killPacket->writePrimitive(KillAvatarReason::TheirAvatarEnteredYourBubble);
        } else {
            killPacket->writePrimitive(KillAvatarReason::YourAvatarEnteredTheirBubble);
        }
        setLastBroadcastTime(other->getUUID(), 0);
        DependencyManager::get<NodeList>()->sendPacket(std::move(killPacket), *self);
    }
}

void AvatarMixerClientData::removeFromRadiusIgnoringSet(SharedNodePointer self, const QUuid& other) {
    if (isRadiusIgnoring(other)) {
        _radiusIgnoredOthers.erase(other);
    }
}

void AvatarMixerClientData::readViewFrustumPacket(const QByteArray& message) {
    _currentViewFrustums.clear();

    auto sourceBuffer = reinterpret_cast<const unsigned char*>(message.constData());
    
    uint8_t numFrustums = 0;
    memcpy(&numFrustums, sourceBuffer, sizeof(numFrustums));
    sourceBuffer += sizeof(numFrustums);

    for (uint8_t i = 0; i < numFrustums; ++i) {
        ConicalViewFrustum frustum;
        sourceBuffer += frustum.deserialize(sourceBuffer);

        _currentViewFrustums.push_back(frustum);
    }
}

bool AvatarMixerClientData::otherAvatarInView(const AABox& otherAvatarBox) {
    return std::any_of(std::begin(_currentViewFrustums), std::end(_currentViewFrustums),
                       [&](const ConicalViewFrustum& viewFrustum) {
        return viewFrustum.intersects(otherAvatarBox);
    });
}

void AvatarMixerClientData::loadJSONStats(QJsonObject& jsonObject) const {
    jsonObject["display_name"] = _avatar->getDisplayName();
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
