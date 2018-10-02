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

#include "AvatarMixerSlave.h"

#include <algorithm>
#include <random>
#include <chrono>

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

namespace chrono = std::chrono;

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
        _stats.packetsProcessed += nodeData->processPackets(*_sharedData);
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

qint64 AvatarMixerSlave::addChangedTraitsToBulkPacket(AvatarMixerClientData* listeningNodeData,
                                                      const AvatarMixerClientData* sendingNodeData,
                                                      NLPacketList& traitsPacketList) {

    auto otherNodeLocalID = sendingNodeData->getNodeLocalID();

    // Perform a simple check with two server clock time points
    // to see if there is any new traits data for this avatar that we need to send
    auto timeOfLastTraitsSent = listeningNodeData->getLastOtherAvatarTraitsSendPoint(otherNodeLocalID);
    auto timeOfLastTraitsChange = sendingNodeData->getLastReceivedTraitsChange();

    qint64 bytesWritten = 0;

    if (timeOfLastTraitsChange > timeOfLastTraitsSent) {
        // there is definitely new traits data to send

        // add the avatar ID to mark the beginning of traits for this avatar
        bytesWritten += traitsPacketList.write(sendingNodeData->getNodeID().toRfc4122());

        auto sendingAvatar = sendingNodeData->getAvatarSharedPointer();

        // compare trait versions so we can see what exactly needs to go out
        auto& lastSentVersions = listeningNodeData->getLastSentTraitVersions(otherNodeLocalID);
        const auto& lastReceivedVersions = sendingNodeData->getLastReceivedTraitVersions();

        auto simpleReceivedIt = lastReceivedVersions.simpleCBegin();
        while (simpleReceivedIt != lastReceivedVersions.simpleCEnd()) {
            auto traitType = static_cast<AvatarTraits::TraitType>(std::distance(lastReceivedVersions.simpleCBegin(),
                                                                                simpleReceivedIt));

            auto lastReceivedVersion = *simpleReceivedIt;
            auto& lastSentVersionRef = lastSentVersions[traitType];

            if (lastReceivedVersions[traitType] > lastSentVersionRef) {
                // there is an update to this trait, add it to the traits packet
                bytesWritten += sendingAvatar->packTrait(traitType, traitsPacketList, lastReceivedVersion);

                // update the last sent version
                lastSentVersionRef = lastReceivedVersion;
            }

            ++simpleReceivedIt;
        }

        // enumerate the received instanced trait versions
        auto instancedReceivedIt = lastReceivedVersions.instancedCBegin();
        while (instancedReceivedIt != lastReceivedVersions.instancedCEnd()) {
            auto traitType = instancedReceivedIt->traitType;

            // get or create the sent trait versions for this trait type
            auto& sentIDValuePairs = lastSentVersions.getInstanceIDValuePairs(traitType);

            // enumerate each received instance
            for (auto& receivedInstance : instancedReceivedIt->instances) {
                auto instanceID = receivedInstance.id;
                const auto receivedVersion = receivedInstance.value;

                // to track deletes and maintain version information for traits
                // the mixer stores the negative value of the received version when a trait instance is deleted
                bool isDeleted = receivedVersion < 0;
                const auto absoluteReceivedVersion = std::abs(receivedVersion);

                // look for existing sent version for this instance
                auto sentInstanceIt = std::find_if(sentIDValuePairs.begin(), sentIDValuePairs.end(),
                                                   [instanceID](auto& sentInstance)
                                                   {
                                                       return sentInstance.id == instanceID;
                                                   });

                if (!isDeleted && (sentInstanceIt == sentIDValuePairs.end() || receivedVersion > sentInstanceIt->value)) {
                    // this instance version exists and has never been sent or is newer so we need to send it
                    bytesWritten += sendingAvatar->packTraitInstance(traitType, instanceID, traitsPacketList, receivedVersion);

                    if (sentInstanceIt != sentIDValuePairs.end()) {
                        sentInstanceIt->value = receivedVersion;
                    } else {
                        sentIDValuePairs.emplace_back(instanceID, receivedVersion);
                    }
                } else if (isDeleted && sentInstanceIt != sentIDValuePairs.end() && absoluteReceivedVersion > sentInstanceIt->value) {
                    // this instance version was deleted and we haven't sent the delete to this client yet
                    bytesWritten += AvatarTraits::packInstancedTraitDelete(traitType, instanceID, traitsPacketList, absoluteReceivedVersion);

                    // update the last sent version for this trait instance to the absolute value of the deleted version
                    sentInstanceIt->value = absoluteReceivedVersion;
                }
            }

            ++instancedReceivedIt;
        }

        // write a null trait type to mark the end of trait data for this avatar
        bytesWritten += traitsPacketList.writePrimitive(AvatarTraits::NullTrait);

        // since we send all traits for this other avatar, update the time of last traits sent
        // to match the time of last traits change
        listeningNodeData->setLastOtherAvatarTraitsSendPoint(otherNodeLocalID, timeOfLastTraitsChange);
    }

    return bytesWritten;
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

AABox computeBubbleBox(const AvatarData& avatar, float bubbleExpansionFactor) {
    AABox box = avatar.getGlobalBoundingBox();
    glm::vec3 scale = box.getScale();
    scale *= bubbleExpansionFactor;
    const glm::vec3 MIN_BUBBLE_SCALE(0.3f, 1.3f, 0.3);
    scale = glm::max(scale, MIN_BUBBLE_SCALE);
    box.setScaleStayCentered(glm::max(scale, MIN_BUBBLE_SCALE));
    return box;
}

void AvatarMixerSlave::broadcastAvatarDataToAgent(const SharedNodePointer& node) {
    const Node* destinationNode = node.data();

    auto nodeList = DependencyManager::get<NodeList>();

    // setup for distributed random floating point values
    std::random_device randomDevice;
    std::mt19937 generator(randomDevice());
    std::uniform_real_distribution<float> distribution;

    _stats.nodesBroadcastedTo++;

    AvatarMixerClientData* nodeData = reinterpret_cast<AvatarMixerClientData*>(destinationNode->getLinkedData());

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
    int traitBytesSent = 0;

    // max number of avatarBytes per frame
    int maxAvatarBytesPerFrame = int(_maxKbpsPerNode * BYTES_PER_KILOBIT / AVATAR_MIXER_BROADCAST_FRAMES_PER_SECOND);


    // keep track of the number of other avatars held back in this frame
    int numAvatarsHeldBack = 0;

    // keep track of the number of other avatar frames skipped
    int numAvatarsWithSkippedFrames = 0;

    // When this is true, the AvatarMixer will send Avatar data to a client
    // about avatars they've ignored or that are out of view
    bool PALIsOpen = nodeData->getRequestsDomainListData();

    // When this is true, the AvatarMixer will send Avatar data to a client about avatars that have ignored them
    bool getsAnyIgnored = PALIsOpen && destinationNode->getCanKick();

    // Bandwidth allowance for data that must be sent.
    int minimumBytesPerAvatar = PALIsOpen ? AvatarDataPacket::AVATAR_HAS_FLAGS_SIZE + NUM_BYTES_RFC4122_UUID +
        sizeof(AvatarDataPacket::AvatarGlobalPosition) + sizeof(AvatarDataPacket::AudioLoudness) : 0;

    // setup a PacketList for the avatarPackets
    auto avatarPacketList = NLPacketList::create(PacketType::BulkAvatarData);
    static auto maxAvatarDataBytes = avatarPacketList->getMaxSegmentSize() - NUM_BYTES_RFC4122_UUID;

    // compute node bounding box
    const float MY_AVATAR_BUBBLE_EXPANSION_FACTOR = 4.0f; // magic number determined emperically
    AABox nodeBox = computeBubbleBox(avatar, MY_AVATAR_BUBBLE_EXPANSION_FACTOR);

    class SortableAvatar: public PrioritySortUtil::Sortable {
    public:
        SortableAvatar() = delete;
        SortableAvatar(const AvatarData* avatar, const Node* avatarNode, uint64_t lastEncodeTime)
            : _avatar(avatar), _node(avatarNode), _lastEncodeTime(lastEncodeTime) {}
        glm::vec3 getPosition() const override { return _avatar->getClientGlobalPosition(); }
        float getRadius() const override {
            glm::vec3 nodeBoxScale = _avatar->getGlobalBoundingBox().getScale();
            return 0.5f * glm::max(nodeBoxScale.x, glm::max(nodeBoxScale.y, nodeBoxScale.z));
        }
        uint64_t getTimestamp() const override {
            return _lastEncodeTime;
        }
        const Node* getNode() const { return _node; }

    private:
        const AvatarData* _avatar;
        const Node* _node;
        uint64_t _lastEncodeTime;
    };

    // prepare to sort
    const auto& cameraViews = nodeData->getViewFrustums();
    PrioritySortUtil::PriorityQueue<SortableAvatar> sortedAvatars(cameraViews,
            AvatarData::_avatarSortCoefficientSize,
            AvatarData::_avatarSortCoefficientCenter,
            AvatarData::_avatarSortCoefficientAge);
    sortedAvatars.reserve(_end - _begin);

    for (auto listedNode = _begin; listedNode != _end; ++listedNode) {
        Node* otherNodeRaw = (*listedNode).data();
        if (otherNodeRaw->getType() != NodeType::Agent
            || !otherNodeRaw->getLinkedData()
            || otherNodeRaw == destinationNode) {
            continue;
        }

        auto avatarNode = otherNodeRaw;

        bool shouldIgnore = false;
        // We ignore other nodes for a couple of reasons:
        //   1) ignore bubbles and ignore specific node
        //   2) the node hasn't really updated it's frame data recently, this can
        //      happen if for example the avatar is connected on a desktop and sending
        //      updates at ~30hz. So every 3 frames we skip a frame.

        assert(avatarNode); // we can't have gotten here without the avatarData being a valid key in the map

        const AvatarMixerClientData* avatarClientNodeData = reinterpret_cast<const AvatarMixerClientData*>(avatarNode->getLinkedData());
        assert(avatarClientNodeData); // we can't have gotten here without avatarNode having valid data
        quint64 startIgnoreCalculation = usecTimestampNow();

        // make sure we have data for this avatar, that it isn't the same node,
        // and isn't an avatar that the viewing node has ignored
        // or that has ignored the viewing node
        if ((destinationNode->isIgnoringNodeWithID(avatarNode->getUUID()) && !PALIsOpen)
            || (avatarNode->isIgnoringNodeWithID(destinationNode->getUUID()) && !getsAnyIgnored)) {
            shouldIgnore = true;
        } else {
            // Check to see if the space bubble is enabled
            // Don't bother with these checks if the other avatar has their bubble enabled and we're gettingAnyIgnored
            if (destinationNode->isIgnoreRadiusEnabled() || (avatarNode->isIgnoreRadiusEnabled() && !getsAnyIgnored)) {
                // Perform the collision check between the two bounding boxes
                const float OTHER_AVATAR_BUBBLE_EXPANSION_FACTOR = 2.4f; // magic number determined empirically
                AABox otherNodeBox = computeBubbleBox(avatarClientNodeData->getAvatar(), OTHER_AVATAR_BUBBLE_EXPANSION_FACTOR);
                if (nodeBox.touches(otherNodeBox)) {
                    nodeData->ignoreOther(destinationNode, avatarNode);
                    shouldIgnore = !getsAnyIgnored;
                }
            }
            // Not close enough to ignore
            if (!shouldIgnore) {
                nodeData->removeFromRadiusIgnoringSet(avatarNode->getUUID());
            }
        }

        if (!shouldIgnore) {
            AvatarDataSequenceNumber lastSeqToReceiver = nodeData->getLastBroadcastSequenceNumber(avatarNode->getUUID());
            AvatarDataSequenceNumber lastSeqFromSender = avatarClientNodeData->getLastReceivedSequenceNumber();

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
            const AvatarData* avatarNodeData = avatarClientNodeData->getConstAvatarData();
            auto lastEncodeTime = nodeData->getLastOtherAvatarEncodeTime(avatarNodeData->getSessionUUID());

            sortedAvatars.push(SortableAvatar(avatarNodeData, avatarNode, lastEncodeTime));
        }
    }

    // loop through our sorted avatars and allocate our bandwidth to them accordingly

    int remainingAvatars = (int)sortedAvatars.size();
    auto traitsPacketList = NLPacketList::create(PacketType::BulkAvatarTraits, QByteArray(), true, true);

    const auto& sortedAvatarVector = sortedAvatars.getSortedVector();
    for (const auto& sortedAvatar : sortedAvatarVector) {
        const Node* otherNode = sortedAvatar.getNode();
        auto lastEncodeForOther = sortedAvatar.getTimestamp();

        assert(otherNode); // we can't have gotten here without the avatarData being a valid key in the map

        AvatarData::AvatarDataDetail detail = AvatarData::NoData;

        // NOTE: Here's where we determine if we are over budget and drop remaining avatars,
        // or send minimal avatar data in uncommon case of PALIsOpen.
        int minimRemainingAvatarBytes = minimumBytesPerAvatar * remainingAvatars;
        bool overBudget = (identityBytesSent + numAvatarDataBytes + minimRemainingAvatarBytes) > maxAvatarBytesPerFrame;
        if (overBudget) {
            if (PALIsOpen) {
                _stats.overBudgetAvatars++;
                detail = AvatarData::PALMinimum;
            } else {
                _stats.overBudgetAvatars += remainingAvatars;
                break;
            }
        }

        auto startAvatarDataPacking = chrono::high_resolution_clock::now();

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

        // Typically all out-of-view avatars but such avatars' priorities will rise with time:
        bool isLowerPriority = sortedAvatar.getPriority() <= OUT_OF_VIEW_THRESHOLD;

        if (isLowerPriority) {
            detail = PALIsOpen ? AvatarData::PALMinimum : AvatarData::MinimumData;
            nodeData->incrementAvatarOutOfView();
        } else if (!overBudget) {
            detail = distribution(generator) < AVATAR_SEND_FULL_UPDATE_RATIO ? AvatarData::SendAllData : AvatarData::CullSmallData;
            nodeData->incrementAvatarInView();
        }

        bool includeThisAvatar = true;
        QVector<JointData>& lastSentJointsForOther = nodeData->getLastOtherAvatarSentJoints(otherNode->getUUID());

        lastSentJointsForOther.resize(otherAvatar->getJointCount());

        bool distanceAdjust = true;
        glm::vec3 viewerPosition = myPosition;
        AvatarDataPacket::HasFlags hasFlagsOut; // the result of the toByteArray
        bool dropFaceTracking = false;

        auto startSerialize = chrono::high_resolution_clock::now();
        QByteArray bytes = otherAvatar->toByteArray(detail, lastEncodeForOther, lastSentJointsForOther,
                                                    hasFlagsOut, dropFaceTracking, distanceAdjust, viewerPosition,
                                                    &lastSentJointsForOther);
        auto endSerialize = chrono::high_resolution_clock::now();
        _stats.toByteArrayElapsedTime +=
            (quint64) chrono::duration_cast<chrono::microseconds>(endSerialize - startSerialize).count();

        if (bytes.size() > maxAvatarDataBytes) {
            qCWarning(avatars) << "otherAvatar.toByteArray() for" << otherNode->getUUID()
                << "resulted in very large buffer of" << bytes.size() << "bytes - dropping facial data";

            dropFaceTracking = true; // first try dropping the facial data
            bytes = otherAvatar->toByteArray(detail, lastEncodeForOther, lastSentJointsForOther,
                                             hasFlagsOut, dropFaceTracking, distanceAdjust, viewerPosition, &lastSentJointsForOther);

            if (bytes.size() > maxAvatarDataBytes) {
                qCWarning(avatars) << "otherAvatar.toByteArray() for" << otherNode->getUUID()
                    << "without facial data resulted in very large buffer of" << bytes.size()
                    << "bytes - reducing to MinimumData";
                bytes = otherAvatar->toByteArray(AvatarData::MinimumData, lastEncodeForOther, lastSentJointsForOther,
                                                 hasFlagsOut, dropFaceTracking, distanceAdjust, viewerPosition, &lastSentJointsForOther);

                if (bytes.size() > maxAvatarDataBytes) {
                    qCWarning(avatars) << "otherAvatar.toByteArray() for" << otherNode->getUUID()
                        << "MinimumData resulted in very large buffer of" << bytes.size()
                        << "bytes - refusing to send avatar";
                    includeThisAvatar = false;
                }
            }
        }

        if (includeThisAvatar) {
            // start a new segment in the PacketList for this avatar
            avatarPacketList->startSegment();
            numAvatarDataBytes += avatarPacketList->write(otherNode->getUUID().toRfc4122());
            numAvatarDataBytes += avatarPacketList->write(bytes);
            avatarPacketList->endSegment();

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

        auto endAvatarDataPacking = chrono::high_resolution_clock::now();
        _stats.avatarDataPackingElapsedTime +=
            (quint64) chrono::duration_cast<chrono::microseconds>(endAvatarDataPacking - startAvatarDataPacking).count();

        // use helper to add any changed traits to our packet list
        traitBytesSent += addChangedTraitsToBulkPacket(nodeData, otherNodeData, *traitsPacketList);
        remainingAvatars--;
    }

    quint64 startPacketSending = usecTimestampNow();

    // close the current packet so that we're always sending something
    avatarPacketList->closeCurrentPacket(true);

    _stats.numPacketsSent += (int)avatarPacketList->getNumPackets();
    _stats.numBytesSent += numAvatarDataBytes;

    // send the avatar data PacketList
    nodeList->sendPacketList(std::move(avatarPacketList), *destinationNode);

    // record the bytes sent for other avatar data in the AvatarMixerClientData
    nodeData->recordSentAvatarData(numAvatarDataBytes);

    // close the current traits packet list
    traitsPacketList->closeCurrentPacket();

    if (traitsPacketList->getNumPackets() >= 1) {
        // send the traits packet list
        nodeList->sendPacketList(std::move(traitsPacketList), *destinationNode);
    }

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

