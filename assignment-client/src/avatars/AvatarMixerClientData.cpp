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

#include <algorithm>
#include <udt/PacketHeaders.h>

#include <DependencyManager.h>
#include <NodeList.h>

#include "AvatarMixerSlave.h"

AvatarMixerClientData::AvatarMixerClientData(const QUuid& nodeID, Node::LocalID nodeLocalID) :
    NodeData(nodeID, nodeLocalID)
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

int AvatarMixerClientData::processPackets(const SlaveSharedData& slaveSharedData) {
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
            case PacketType::SetAvatarTraits:
                processSetTraitsMessage(*packet, slaveSharedData, *node);
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

void AvatarMixerClientData::processSetTraitsMessage(ReceivedMessage& message,
                                                    const SlaveSharedData& slaveSharedData, Node& sendingNode) {
    // pull the trait version from the message
    AvatarTraits::TraitVersion packetTraitVersion;
    message.readPrimitive(&packetTraitVersion);

    bool anyTraitsChanged = false;

    while (message.getBytesLeftToRead() > 0) {
        // for each trait in the packet, apply it if the trait version is newer than what we have

        AvatarTraits::TraitType traitType;
        message.readPrimitive(&traitType);

        if (AvatarTraits::isSimpleTrait(traitType)) {
            AvatarTraits::TraitWireSize traitSize;
            message.readPrimitive(&traitSize);

            if (packetTraitVersion > _lastReceivedTraitVersions[traitType]) {
                _avatar->processTrait(traitType, message.read(traitSize));
                _lastReceivedTraitVersions[traitType] = packetTraitVersion;

                if (traitType == AvatarTraits::SkeletonModelURL) {
                    // special handling for skeleton model URL, since we need to make sure it is in the whitelist
                    checkSkeletonURLAgainstWhitelist(slaveSharedData, sendingNode, packetTraitVersion);
                }

                anyTraitsChanged = true;
            } else {
                message.seek(message.getPosition() + traitSize);
            }
        } else {
            AvatarTraits::TraitInstanceID instanceID = QUuid::fromRfc4122(message.readWithoutCopy(NUM_BYTES_RFC4122_UUID));

            AvatarTraits::TraitWireSize traitSize;
            message.readPrimitive(&traitSize);

            auto& instanceVersionRef = _lastReceivedTraitVersions.getInstanceValueRef(traitType, instanceID);

            if (packetTraitVersion > instanceVersionRef) {
                if (traitSize == AvatarTraits::DELETED_TRAIT_SIZE) {
                    _avatar->processDeletedTraitInstance(traitType, instanceID);

                    // to track a deleted instance but keep version information
                    // the avatar mixer uses the negative value of the sent version
                    instanceVersionRef = -packetTraitVersion;
                } else {
                    _avatar->processTraitInstance(traitType, instanceID, message.read(traitSize));
                    instanceVersionRef = packetTraitVersion;
                }

                anyTraitsChanged = true;
            } else {
                message.seek(message.getPosition() + traitSize);
            }
        }
    }

    if (anyTraitsChanged) {
        _lastReceivedTraitsChange = std::chrono::steady_clock::now();
    }
}

void AvatarMixerClientData::checkSkeletonURLAgainstWhitelist(const SlaveSharedData &slaveSharedData, Node& sendingNode,
                                                             AvatarTraits::TraitVersion traitVersion) {
    const auto& whitelist = slaveSharedData.skeletonURLWhitelist;

    if (!whitelist.isEmpty()) {
        bool inWhitelist = false;
        auto avatarURL = _avatar->getSkeletonModelURL();

        // The avatar is in the whitelist if:
        // 1. The avatar's URL's host matches one of the hosts of the URLs in the whitelist AND
        // 2. The avatar's URL's path starts with the path of that same URL in the whitelist
        for (const auto& whiteListedPrefix : whitelist) {
            auto whiteListURL = QUrl::fromUserInput(whiteListedPrefix);
            // check if this script URL matches the whitelist domain and, optionally, is beneath the path
            if (avatarURL.host().compare(whiteListURL.host(), Qt::CaseInsensitive) == 0 &&
                avatarURL.path().startsWith(whiteListURL.path(), Qt::CaseInsensitive)) {
                inWhitelist = true;

                break;
            }
        }

        if (!inWhitelist) {
            // make sure we're not unecessarily overriding the default avatar with the default avatar
            if (_avatar->getWireSafeSkeletonModelURL() != slaveSharedData.skeletonReplacementURL) {
                // we need to change this avatar's skeleton URL, and send them a traits packet informing them of the change
                qDebug() << "Overwriting avatar URL" << _avatar->getWireSafeSkeletonModelURL()
                    << "to replacement" << slaveSharedData.skeletonReplacementURL << "for" << sendingNode.getUUID();
                _avatar->setSkeletonModelURL(slaveSharedData.skeletonReplacementURL);

                auto packet = NLPacket::create(PacketType::SetAvatarTraits, -1, true);

                // the returned set traits packet uses the trait version from the incoming packet
                // so the client knows they should not overwrite if they have since changed the trait
                _avatar->packTrait(AvatarTraits::SkeletonModelURL, *packet, traitVersion);

                auto nodeList = DependencyManager::get<NodeList>();
                nodeList->sendPacket(std::move(packet), sendingNode);
            }
        }
    }
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
    ignoreOther(self.data(), other.data());
}

void AvatarMixerClientData::ignoreOther(const Node* self, const Node* other) {
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

        resetSentTraitData(other->getLocalID());

        DependencyManager::get<NodeList>()->sendPacket(std::move(killPacket), *self);
    }
}

bool AvatarMixerClientData::isRadiusIgnoring(const QUuid& other) const {
    return std::find(_radiusIgnoredOthers.cbegin(), _radiusIgnoredOthers.cend(), other) != _radiusIgnoredOthers.cend();
}

void AvatarMixerClientData::addToRadiusIgnoringSet(const QUuid& other) {
    if (!isRadiusIgnoring(other)) {
        _radiusIgnoredOthers.push_back(other);
    }
}

void AvatarMixerClientData::removeFromRadiusIgnoringSet(const QUuid& other) {
    auto ignoredOtherIter = std::find(_radiusIgnoredOthers.cbegin(), _radiusIgnoredOthers.cend(), other);
    if (ignoredOtherIter != _radiusIgnoredOthers.cend()) {
        _radiusIgnoredOthers.erase(ignoredOtherIter);
    }
}

void AvatarMixerClientData::resetSentTraitData(Node::LocalID nodeLocalID) {
    _lastSentTraitsTimestamps[nodeLocalID] = TraitsCheckTimestamp();
    _sentTraitVersions[nodeLocalID].reset();
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

AvatarMixerClientData::TraitsCheckTimestamp AvatarMixerClientData::getLastOtherAvatarTraitsSendPoint(Node::LocalID otherAvatar) const {
    auto it = _lastSentTraitsTimestamps.find(otherAvatar);

    if (it != _lastSentTraitsTimestamps.end()) {
        return it->second;
    } else {
        return TraitsCheckTimestamp();
    }
}

void AvatarMixerClientData::cleanupKilledNode(const QUuid& nodeUUID, Node::LocalID nodeLocalID) {
    removeLastBroadcastSequenceNumber(nodeUUID);
    removeLastBroadcastTime(nodeUUID);
    _lastSentTraitsTimestamps.erase(nodeLocalID);
    _sentTraitVersions.erase(nodeLocalID);
}
