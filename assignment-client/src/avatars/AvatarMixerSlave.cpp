//
//  AvatarMixerSlave.cpp
//  assignment-client/src/avatar
//
//  Created by Brad Hefta-Gaub on 2/14/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <algorithm>
#include <random>

#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtx/vector_angle.hpp>

#include <AvatarLogging.h>
#include <LogHandler.h>
#include <NetworkAccessManager.h>
#include <NodeList.h>
#include <Node.h>
#include <OctreeConstants.h>
#include <PrioritySortUtil.h>
#include <udt/PacketHeaders.h>
#include <SharedUtil.h>
#include <StDev.h>
#include <UUID.h>


#include "AvatarMixer.h"
#include "AvatarMixerClientData.h"
#include "AvatarMixerSlave.h"

void AvatarMixerSlave::configure(ConstIter begin, ConstIter end) {
    _begin = begin;
    _end = end;
}

void AvatarMixerSlave::configureBroadcast(ConstIter begin, ConstIter end, 
                                p_high_resolution_clock::time_point lastFrameTimestamp,
                                float maxKbpsPerNode, float throttlingRatio) {
    _begin = begin;
    _end = end;
    _lastFrameTimestamp = lastFrameTimestamp;
    _maxKbpsPerNode = maxKbpsPerNode;
    _throttlingRatio = throttlingRatio;
}

void AvatarMixerSlave::harvestStats(AvatarMixerSlaveStats& stats) {
    stats = _stats;
    _stats.reset();
}


void AvatarMixerSlave::processIncomingPackets(const SharedNodePointer& node) {
    auto start = usecTimestampNow();
    auto nodeData = dynamic_cast<AvatarMixerClientData*>(node->getLinkedData());
    if (nodeData) {
        _stats.nodesProcessed++;
        _stats.packetsProcessed += nodeData->processPackets();
    }
    auto end = usecTimestampNow();
    _stats.processIncomingPacketsElapsedTime += (end - start);
}

int AvatarMixerSlave::sendIdentityPacket(const AvatarMixerClientData* nodeData, const SharedNodePointer& destinationNode) {
    if (destinationNode->getType() == NodeType::Agent && !destinationNode->isUpstream()) {
        QByteArray individualData = nodeData->getConstAvatarData()->identityByteArray();
        individualData.replace(0, NUM_BYTES_RFC4122_UUID, nodeData->getNodeID().toRfc4122()); // FIXME, this looks suspicious
        auto identityPackets = NLPacketList::create(PacketType::AvatarIdentity, QByteArray(), true, true);
        identityPackets->write(individualData);
        DependencyManager::get<NodeList>()->sendPacketList(std::move(identityPackets), *destinationNode);
        _stats.numIdentityPackets++;
        return individualData.size();
    } else {
        return 0;
    }
}

int AvatarMixerSlave::sendReplicatedIdentityPacket(const Node& agentNode, const AvatarMixerClientData* nodeData, const Node& destinationNode) {
    if (AvatarMixer::shouldReplicateTo(agentNode, destinationNode)) {
        QByteArray individualData = nodeData->getConstAvatarData()->identityByteArray(true);
        individualData.replace(0, NUM_BYTES_RFC4122_UUID, nodeData->getNodeID().toRfc4122()); // FIXME, this looks suspicious
        auto identityPacket = NLPacketList::create(PacketType::ReplicatedAvatarIdentity, QByteArray(), true, true);
        identityPacket->write(individualData);
        DependencyManager::get<NodeList>()->sendPacketList(std::move(identityPacket), destinationNode);
        _stats.numIdentityPackets++;
        return individualData.size();
    } else {
        return 0;
    }
}

static const int AVATAR_MIXER_BROADCAST_FRAMES_PER_SECOND = 45;

void AvatarMixerSlave::broadcastAvatarData(const SharedNodePointer& node) {
    quint64 start = usecTimestampNow();

    if (node->getType() == NodeType::Agent && node->getLinkedData() && node->getActiveSocket() && !node->isUpstream()) {
        broadcastAvatarDataToAgent(node);
    } else if (node->getType() == NodeType::DownstreamAvatarMixer) {
        broadcastAvatarDataToDownstreamMixer(node);
    }

    quint64 end = usecTimestampNow();
    _stats.jobElapsedTime += (end - start);
}

void AvatarMixerSlave::broadcastAvatarDataToAgent(const SharedNodePointer& node) {

    auto nodeList = DependencyManager::get<NodeList>();

    // setup for distributed random floating point values
    std::random_device randomDevice;
    std::mt19937 generator(randomDevice());
    std::uniform_real_distribution<float> distribution;

    _stats.nodesBroadcastedTo++;

    AvatarMixerClientData* nodeData = reinterpret_cast<AvatarMixerClientData*>(node->getLinkedData());

    nodeData->resetInViewStats();

    const AvatarData& avatar = nodeData->getAvatar();
    glm::vec3 myPosition = avatar.getClientGlobalPosition();

    // reset the internal state for correct random number distribution
    distribution.reset();

    // reset the number of sent avatars
    nodeData->resetNumAvatarsSentLastFrame();

    // keep a counter of the number of considered avatars
    int numOtherAvatars = 0;

    // keep track of outbound data rate specifically for avatar data
    int numAvatarDataBytes = 0;
    int identityBytesSent = 0;

    // max number of avatarBytes per frame
    auto maxAvatarBytesPerFrame = (_maxKbpsPerNode * BYTES_PER_KILOBIT) / AVATAR_MIXER_BROADCAST_FRAMES_PER_SECOND;

    // FIXME - find a way to not send the sessionID for every avatar
    int minimumBytesPerAvatar = AvatarDataPacket::AVATAR_HAS_FLAGS_SIZE + NUM_BYTES_RFC4122_UUID;

    int overBudgetAvatars = 0;

    // keep track of the number of other avatars held back in this frame
    int numAvatarsHeldBack = 0;

    // keep track of the number of other avatar frames skipped
    int numAvatarsWithSkippedFrames = 0;

    // When this is true, the AvatarMixer will send Avatar data to a client
    // about avatars they've ignored or that are out of view
    bool PALIsOpen = nodeData->getRequestsDomainListData();

    // When this is true, the AvatarMixer will send Avatar data to a client about avatars that have ignored them
    bool getsAnyIgnored = PALIsOpen && node->getCanKick();

    if (PALIsOpen) {
        // Increase minimumBytesPerAvatar if the PAL is open
        minimumBytesPerAvatar += sizeof(AvatarDataPacket::AvatarGlobalPosition) +
        sizeof(AvatarDataPacket::AudioLoudness);
    }

    // setup a PacketList for the avatarPackets
    auto avatarPacketList = NLPacketList::create(PacketType::BulkAvatarData);

    // Define the minimum bubble size
    static const glm::vec3 minBubbleSize = avatar.getSensorToWorldScale() * glm::vec3(0.3f, 1.3f, 0.3f);
    // Define the scale of the box for the current node
    glm::vec3 nodeBoxScale = (nodeData->getPosition() - nodeData->getGlobalBoundingBoxCorner()) * 2.0f * avatar.getSensorToWorldScale();
    // Set up the bounding box for the current node
    AABox nodeBox(nodeData->getGlobalBoundingBoxCorner(), nodeBoxScale);
    // Clamp the size of the bounding box to a minimum scale
    if (glm::any(glm::lessThan(nodeBoxScale, minBubbleSize))) {
        nodeBox.setScaleStayCentered(minBubbleSize);
    }
    // Quadruple the scale of both bounding boxes
    nodeBox.embiggen(4.0f);


    // setup list of AvatarData as well as maps to map betweeen the AvatarData and the original nodes
    std::vector<AvatarSharedPointer> avatarsToSort;
    std::unordered_map<AvatarSharedPointer, SharedNodePointer> avatarDataToNodes;
    std::unordered_map<QUuid, uint64_t> avatarEncodeTimes;
    std::for_each(_begin, _end, [&](const SharedNodePointer& otherNode) {
        // make sure this is an agent that we have avatar data for before considering it for inclusion
        if (otherNode->getType() == NodeType::Agent
            && otherNode->getLinkedData()) {
            const AvatarMixerClientData* otherNodeData = reinterpret_cast<const AvatarMixerClientData*>(otherNode->getLinkedData());

            AvatarSharedPointer otherAvatar = otherNodeData->getAvatarSharedPointer();
            avatarsToSort.push_back(otherAvatar);
            avatarDataToNodes[otherAvatar] = otherNode;
            QUuid id = otherAvatar->getSessionUUID();
            avatarEncodeTimes[id] = nodeData->getLastOtherAvatarEncodeTime(id);
        }
    });

    class SortableAvatar: public PrioritySortUtil::Sortable {
    public:
        SortableAvatar() = delete;
        SortableAvatar(const AvatarSharedPointer& avatar, uint64_t lastEncodeTime)
            : _avatar(avatar), _lastEncodeTime(lastEncodeTime) {}
        glm::vec3 getPosition() const override { return _avatar->getWorldPosition(); }
        float getRadius() const override {
            glm::vec3 nodeBoxHalfScale = (_avatar->getWorldPosition() - _avatar->getGlobalBoundingBoxCorner() * _avatar->getSensorToWorldScale());
            return glm::max(nodeBoxHalfScale.x, glm::max(nodeBoxHalfScale.y, nodeBoxHalfScale.z));
        }
        uint64_t getTimestamp() const override {
            return _lastEncodeTime;
        }
        const AvatarSharedPointer& getAvatar() const { return _avatar; }

    private:
        AvatarSharedPointer _avatar;
        uint64_t _lastEncodeTime;
    };

    // prepare to sort
    ViewFrustum cameraView = nodeData->getViewFrustum();
    PrioritySortUtil::PriorityQueue<SortableAvatar> sortedAvatars(cameraView,
            AvatarData::_avatarSortCoefficientSize,
            AvatarData::_avatarSortCoefficientCenter,
            AvatarData::_avatarSortCoefficientAge);

    // ignore or sort
    const AvatarSharedPointer& thisAvatar = nodeData->getAvatarSharedPointer();
    for (const auto& avatar : avatarsToSort) {
        if (avatar == thisAvatar) {
            // don't echo updates to self
            continue;
        }

        bool shouldIgnore = false;
        // We ignore other nodes for a couple of reasons:
        //   1) ignore bubbles and ignore specific node
        //   2) the node hasn't really updated it's frame data recently, this can
        //      happen if for example the avatar is connected on a desktop and sending
        //      updates at ~30hz. So every 3 frames we skip a frame.

        auto avatarNode = avatarDataToNodes[avatar];
        assert(avatarNode); // we can't have gotten here without the avatarData being a valid key in the map

        const AvatarMixerClientData* avatarNodeData = reinterpret_cast<const AvatarMixerClientData*>(avatarNode->getLinkedData());
        assert(avatarNodeData); // we can't have gotten here without avatarNode having valid data
        quint64 startIgnoreCalculation = usecTimestampNow();

        // make sure we have data for this avatar, that it isn't the same node,
        // and isn't an avatar that the viewing node has ignored
        // or that has ignored the viewing node
        if (!avatarNode->getLinkedData()
            || avatarNode->getUUID() == node->getUUID()
            || (node->isIgnoringNodeWithID(avatarNode->getUUID()) && !PALIsOpen)
            || (avatarNode->isIgnoringNodeWithID(node->getUUID()) && !getsAnyIgnored)) {
            shouldIgnore = true;
        } else {
            // Check to see if the space bubble is enabled
            // Don't bother with these checks if the other avatar has their bubble enabled and we're gettingAnyIgnored
            if (node->isIgnoreRadiusEnabled() || (avatarNode->isIgnoreRadiusEnabled() && !getsAnyIgnored)) {
                float sensorToWorldScale = avatarNodeData->getAvatarSharedPointer()->getSensorToWorldScale();
                // Define the scale of the box for the current other node
                glm::vec3 otherNodeBoxScale = (avatarNodeData->getPosition() - avatarNodeData->getGlobalBoundingBoxCorner()) * 2.0f * sensorToWorldScale;
                // Set up the bounding box for the current other node
                AABox otherNodeBox(avatarNodeData->getGlobalBoundingBoxCorner(), otherNodeBoxScale);
                // Clamp the size of the bounding box to a minimum scale
                if (glm::any(glm::lessThan(otherNodeBoxScale, minBubbleSize))) {
                    otherNodeBox.setScaleStayCentered(minBubbleSize);
                }
                // Quadruple the scale of both bounding boxes
                otherNodeBox.embiggen(4.0f);

                // Perform the collision check between the two bounding boxes
                if (nodeBox.touches(otherNodeBox)) {
                    nodeData->ignoreOther(node, avatarNode);
                    shouldIgnore = !getsAnyIgnored;
                }
            }
            // Not close enough to ignore
            if (!shouldIgnore) {
                nodeData->removeFromRadiusIgnoringSet(node, avatarNode->getUUID());
            }
        }

        if (!shouldIgnore) {
            AvatarDataSequenceNumber lastSeqToReceiver = nodeData->getLastBroadcastSequenceNumber(avatarNode->getUUID());
            AvatarDataSequenceNumber lastSeqFromSender = avatarNodeData->getLastReceivedSequenceNumber();

            // FIXME - This code does appear to be working. But it seems brittle.
            //         It supports determining if the frame of data for this "other"
            //         avatar has already been sent to the reciever. This has been
            //         verified to work on a desktop display that renders at 60hz and
            //         therefore sends to mixer at 30hz. Each second you'd expect to
            //         have 15 (45hz-30hz) duplicate frames. In this case, the stat
            //         avg_other_av_skips_per_second does report 15.
            //
            // make sure we haven't already sent this data from this sender to this receiver
            // or that somehow we haven't sent
            if (lastSeqToReceiver == lastSeqFromSender && lastSeqToReceiver != 0) {
                ++numAvatarsHeldBack;
                shouldIgnore = true;
            } else if (lastSeqFromSender - lastSeqToReceiver > 1) {
                // this is a skip - we still send the packet but capture the presence of the skip so we see it happening
                ++numAvatarsWithSkippedFrames;
            }
        }
        quint64 endIgnoreCalculation = usecTimestampNow();
        _stats.ignoreCalculationElapsedTime += (endIgnoreCalculation - startIgnoreCalculation);

        if (!shouldIgnore) {
            // sort this one for later
            uint64_t lastEncodeTime = 0;
            std::unordered_map<QUuid, uint64_t>::const_iterator itr = avatarEncodeTimes.find(avatar->getSessionUUID());
            if (itr != avatarEncodeTimes.end()) {
                lastEncodeTime = itr->second;
            }
            sortedAvatars.push(SortableAvatar(avatar, lastEncodeTime));
        }
    }

    // loop through our sorted avatars and allocate our bandwidth to them accordingly

    int remainingAvatars = (int)sortedAvatars.size();
    while (!sortedAvatars.empty()) {
        const auto& avatarData = sortedAvatars.top().getAvatar();
        sortedAvatars.pop();
        remainingAvatars--;

        auto otherNode = avatarDataToNodes[avatarData];
        assert(otherNode); // we can't have gotten here without the avatarData being a valid key in the map

        // NOTE: Here's where we determine if we are over budget and drop to bare minimum data
        int minimRemainingAvatarBytes = minimumBytesPerAvatar * remainingAvatars;
        bool overBudget = (identityBytesSent + numAvatarDataBytes + minimRemainingAvatarBytes) > maxAvatarBytesPerFrame;

        quint64 startAvatarDataPacking = usecTimestampNow();

        ++numOtherAvatars;

        const AvatarMixerClientData* otherNodeData = reinterpret_cast<const AvatarMixerClientData*>(otherNode->getLinkedData());
        const AvatarData* otherAvatar = otherNodeData->getConstAvatarData();

        // If the time that the mixer sent AVATAR DATA about Avatar B to Avatar A is BEFORE OR EQUAL TO
        // the time that Avatar B flagged an IDENTITY DATA change, send IDENTITY DATA about Avatar B to Avatar A.
        if (otherAvatar->hasProcessedFirstIdentity()
            && nodeData->getLastBroadcastTime(otherNode->getUUID()) <= otherNodeData->getIdentityChangeTimestamp()) {
            identityBytesSent += sendIdentityPacket(otherNodeData, node);

            // remember the last time we sent identity details about this other node to the receiver
            nodeData->setLastBroadcastTime(otherNode->getUUID(), usecTimestampNow());
        }

        // determine if avatar is in view which determines how much data to send
        glm::vec3 otherPosition = otherAvatar->getClientGlobalPosition();
        glm::vec3 otherNodeBoxScale = (otherPosition - otherNodeData->getGlobalBoundingBoxCorner()) * 2.0f * otherAvatar->getSensorToWorldScale();
        AABox otherNodeBox(otherNodeData->getGlobalBoundingBoxCorner(), otherNodeBoxScale);
        bool isInView = nodeData->otherAvatarInView(otherNodeBox);

        // start a new segment in the PacketList for this avatar
        avatarPacketList->startSegment();

        AvatarData::AvatarDataDetail detail;

        if (overBudget) {
            overBudgetAvatars++;
            _stats.overBudgetAvatars++;
            detail = PALIsOpen ? AvatarData::PALMinimum : AvatarData::NoData;
        } else if (!isInView) {
            detail = PALIsOpen ? AvatarData::PALMinimum : AvatarData::MinimumData;
            nodeData->incrementAvatarOutOfView();
        } else {
            detail = distribution(generator) < AVATAR_SEND_FULL_UPDATE_RATIO
            ? AvatarData::SendAllData : AvatarData::CullSmallData;
            nodeData->incrementAvatarInView();
        }

        bool includeThisAvatar = true;
        auto lastEncodeForOther = nodeData->getLastOtherAvatarEncodeTime(otherNode->getUUID());
        QVector<JointData>& lastSentJointsForOther = nodeData->getLastOtherAvatarSentJoints(otherNode->getUUID());
        bool distanceAdjust = true;
        glm::vec3 viewerPosition = myPosition;
        AvatarDataPacket::HasFlags hasFlagsOut; // the result of the toByteArray
        bool dropFaceTracking = false;

        quint64 start = usecTimestampNow();
        QByteArray bytes = otherAvatar->toByteArray(detail, lastEncodeForOther, lastSentJointsForOther,
                                                    hasFlagsOut, dropFaceTracking, distanceAdjust, viewerPosition, &lastSentJointsForOther);
        quint64 end = usecTimestampNow();
        _stats.toByteArrayElapsedTime += (end - start);

        static const int MAX_ALLOWED_AVATAR_DATA = (1400 - NUM_BYTES_RFC4122_UUID);
        if (bytes.size() > MAX_ALLOWED_AVATAR_DATA) {
            qCWarning(avatars) << "otherAvatar.toByteArray() resulted in very large buffer:" << bytes.size() << "... attempt to drop facial data";

            dropFaceTracking = true; // first try dropping the facial data
            bytes = otherAvatar->toByteArray(detail, lastEncodeForOther, lastSentJointsForOther,
                                             hasFlagsOut, dropFaceTracking, distanceAdjust, viewerPosition, &lastSentJointsForOther);

            if (bytes.size() > MAX_ALLOWED_AVATAR_DATA) {
                qCWarning(avatars) << "otherAvatar.toByteArray() without facial data resulted in very large buffer:" << bytes.size() << "... reduce to MinimumData";
                bytes = otherAvatar->toByteArray(AvatarData::MinimumData, lastEncodeForOther, lastSentJointsForOther,
                                                 hasFlagsOut, dropFaceTracking, distanceAdjust, viewerPosition, &lastSentJointsForOther);

                if (bytes.size() > MAX_ALLOWED_AVATAR_DATA) {
                    qCWarning(avatars) << "otherAvatar.toByteArray() MinimumData resulted in very large buffer:" << bytes.size() << "... FAIL!!";
                    includeThisAvatar = false;
                }
            }
        }

        if (includeThisAvatar) {
            numAvatarDataBytes += avatarPacketList->write(otherNode->getUUID().toRfc4122());
            numAvatarDataBytes += avatarPacketList->write(bytes);

            if (detail != AvatarData::NoData) {
                _stats.numOthersIncluded++;

                // increment the number of avatars sent to this reciever
                nodeData->incrementNumAvatarsSentLastFrame();

                // set the last sent sequence number for this sender on the receiver
                nodeData->setLastBroadcastSequenceNumber(otherNode->getUUID(),
                                                         otherNodeData->getLastReceivedSequenceNumber());
                nodeData->setLastOtherAvatarEncodeTime(otherNode->getUUID(), usecTimestampNow());
            }
        } else {
            // TODO? this avatar is not included now, and will probably not be included next frame.
            // It would be nice if we could tweak its future sort priority to put it at the back of the list.
        }

        avatarPacketList->endSegment();

        quint64 endAvatarDataPacking = usecTimestampNow();
        _stats.avatarDataPackingElapsedTime += (endAvatarDataPacking - startAvatarDataPacking);
    }

    quint64 startPacketSending = usecTimestampNow();

    // close the current packet so that we're always sending something
    avatarPacketList->closeCurrentPacket(true);

    _stats.numPacketsSent += (int)avatarPacketList->getNumPackets();
    _stats.numBytesSent += numAvatarDataBytes;

    // send the avatar data PacketList
    nodeList->sendPacketList(std::move(avatarPacketList), *node);

    // record the bytes sent for other avatar data in the AvatarMixerClientData
    nodeData->recordSentAvatarData(numAvatarDataBytes);

    // record the number of avatars held back this frame
    nodeData->recordNumOtherAvatarStarves(numAvatarsHeldBack);
    nodeData->recordNumOtherAvatarSkips(numAvatarsWithSkippedFrames);

    quint64 endPacketSending = usecTimestampNow();
    _stats.packetSendingElapsedTime += (endPacketSending - startPacketSending);
}

uint64_t REBROADCAST_IDENTITY_TO_DOWNSTREAM_EVERY_US = 5 * 1000 * 1000;

void AvatarMixerSlave::broadcastAvatarDataToDownstreamMixer(const SharedNodePointer& node) {
    _stats.downstreamMixersBroadcastedTo++;

    AvatarMixerClientData* nodeData = reinterpret_cast<AvatarMixerClientData*>(node->getLinkedData());
    if (!nodeData) {
        return;
    }

    // setup a PacketList for the replicated bulk avatar data
    auto avatarPacketList = NLPacketList::create(PacketType::ReplicatedBulkAvatarData);

    int numAvatarDataBytes = 0;

    // reset the number of sent avatars
    nodeData->resetNumAvatarsSentLastFrame();

    std::for_each(_begin, _end, [&](const SharedNodePointer& agentNode) {
        if (!AvatarMixer::shouldReplicateTo(*agentNode, *node)) {
            return;
        }
        
        // collect agents that we have avatar data for that we are supposed to replicate
        if (agentNode->getType() == NodeType::Agent && agentNode->getLinkedData() && agentNode->isReplicated()) {
            const AvatarMixerClientData* agentNodeData = reinterpret_cast<const AvatarMixerClientData*>(agentNode->getLinkedData());

            AvatarSharedPointer otherAvatar = agentNodeData->getAvatarSharedPointer();

            quint64 startAvatarDataPacking = usecTimestampNow();

            // we cannot send a downstream avatar mixer any updates that expect them to have previous state for this avatar
            // since we have no idea if they're online and receiving our packets

            // so we always send a full update for this avatar
            
            quint64 start = usecTimestampNow();
            AvatarDataPacket::HasFlags flagsOut;

            QVector<JointData> emptyLastJointSendData { otherAvatar->getJointCount() };

            QByteArray avatarByteArray = otherAvatar->toByteArray(AvatarData::SendAllData, 0, emptyLastJointSendData,
                                                                  flagsOut, false, false, glm::vec3(0), nullptr);
            quint64 end = usecTimestampNow();
            _stats.toByteArrayElapsedTime += (end - start);

            auto lastBroadcastTime = nodeData->getLastBroadcastTime(agentNode->getUUID());
            if (lastBroadcastTime <= agentNodeData->getIdentityChangeTimestamp()
                || (start - lastBroadcastTime) >= REBROADCAST_IDENTITY_TO_DOWNSTREAM_EVERY_US) {
                sendReplicatedIdentityPacket(*agentNode, agentNodeData, *node);
                nodeData->setLastBroadcastTime(agentNode->getUUID(), start);
            }

            // figure out how large our avatar byte array can be to fit in the packet list
            // given that we need it and the avatar UUID and the size of the byte array (16 bit)
            // to fit in a segment of the packet list
            auto maxAvatarByteArraySize = avatarPacketList->getMaxSegmentSize();
            maxAvatarByteArraySize -= NUM_BYTES_RFC4122_UUID;
            maxAvatarByteArraySize -= sizeof(quint16);

            auto sequenceNumberSize = sizeof(agentNodeData->getLastReceivedSequenceNumber());
            maxAvatarByteArraySize -= sequenceNumberSize;

            if (avatarByteArray.size() > maxAvatarByteArraySize) {
                qCWarning(avatars) << "Replicated avatar data too large for" << otherAvatar->getSessionUUID()
                    << "-" << avatarByteArray.size() << "bytes";

                avatarByteArray = otherAvatar->toByteArray(AvatarData::SendAllData, 0, emptyLastJointSendData,
                                                           flagsOut, true, false, glm::vec3(0), nullptr);

                if (avatarByteArray.size() > maxAvatarByteArraySize) {
                    qCWarning(avatars) << "Replicated avatar data without facial data still too large for"
                        << otherAvatar->getSessionUUID() << "-" << avatarByteArray.size() << "bytes";

                    avatarByteArray = otherAvatar->toByteArray(AvatarData::MinimumData, 0, emptyLastJointSendData,
                                                               flagsOut, true, false, glm::vec3(0), nullptr);
                }
            }

            if (avatarByteArray.size() <= maxAvatarByteArraySize) {
                // increment the number of avatars sent to this reciever
                nodeData->incrementNumAvatarsSentLastFrame();

                // set the last sent sequence number for this sender on the receiver
                nodeData->setLastBroadcastSequenceNumber(agentNode->getUUID(),
                                                         agentNodeData->getLastReceivedSequenceNumber());

                // increment the number of avatars sent to this reciever
                nodeData->incrementNumAvatarsSentLastFrame();

                // start a new segment in the packet list for this avatar
                avatarPacketList->startSegment();

                // write the node's UUID, the size of the replicated avatar data,
                // the sequence number of the replicated avatar data, and the replicated avatar data
                numAvatarDataBytes += avatarPacketList->write(agentNode->getUUID().toRfc4122());
                numAvatarDataBytes += avatarPacketList->writePrimitive((quint16) (avatarByteArray.size() + sequenceNumberSize));
                numAvatarDataBytes += avatarPacketList->writePrimitive(agentNodeData->getLastReceivedSequenceNumber());
                numAvatarDataBytes += avatarPacketList->write(avatarByteArray);

                avatarPacketList->endSegment();

            } else {
                qCWarning(avatars) << "Could not fit minimum data avatar for" << otherAvatar->getSessionUUID()
                    << "to packet list -" << avatarByteArray.size() << "bytes";
            }

            quint64 endAvatarDataPacking = usecTimestampNow();
            _stats.avatarDataPackingElapsedTime += (endAvatarDataPacking - startAvatarDataPacking);
        }
    });

    if (avatarPacketList->getNumPackets() > 0) {
        quint64 startPacketSending = usecTimestampNow();

        // close the current packet so that we're always sending something
        avatarPacketList->closeCurrentPacket(true);

        _stats.numPacketsSent += (int)avatarPacketList->getNumPackets();
        _stats.numBytesSent += numAvatarDataBytes;

        // send the replicated bulk avatar data
        auto nodeList = DependencyManager::get<NodeList>();
        nodeList->sendPacketList(std::move(avatarPacketList), node->getPublicSocket());

        // record the bytes sent for other avatar data in the AvatarMixerClientData
        nodeData->recordSentAvatarData(numAvatarDataBytes);

        quint64 endPacketSending = usecTimestampNow();
        _stats.packetSendingElapsedTime += (endPacketSending - startPacketSending);
    }
}

