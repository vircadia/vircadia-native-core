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
                                float maxKbpsPerNode, float throttlingRatio,
                                float priorityReservedFraction) {
    _begin = begin;
    _end = end;
    _lastFrameTimestamp = lastFrameTimestamp;
    _maxKbpsPerNode = maxKbpsPerNode;
    _throttlingRatio = throttlingRatio;
    _avatarHeroFraction = priorityReservedFraction;
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

int AvatarMixerSlave::sendIdentityPacket(NLPacketList& packetList, const AvatarMixerClientData* nodeData, const Node& destinationNode) {
    if (destinationNode.getType() == NodeType::Agent && !destinationNode.isUpstream()) {
        QByteArray individualData = nodeData->getConstAvatarData()->identityByteArray();
        individualData.replace(0, NUM_BYTES_RFC4122_UUID, nodeData->getNodeID().toRfc4122()); // FIXME, this looks suspicious
        packetList.write(individualData);
        _stats.numIdentityPacketsSent++;
        _stats.numIdentityBytesSent += individualData.size();
        return individualData.size();
    } else {
        return 0;
    }
}

qint64 AvatarMixerSlave::addTraitsNodeHeader(AvatarMixerClientData* listeningNodeData,
                                             const AvatarMixerClientData* sendingNodeData,
                                             NLPacketList& traitsPacketList,
                                             qint64 bytesWritten) {
    if (bytesWritten == 0) {
        if (traitsPacketList.getNumPackets() == 0) {
            // This is the beginning of the traits packet, write out the sequence number.
            bytesWritten += traitsPacketList.writePrimitive(listeningNodeData->nextTraitsMessageSequence());
        }
        // This is the beginning of the traits for a node, write out the node id
        bytesWritten += traitsPacketList.write(sendingNodeData->getNodeID().toRfc4122());
    }
    return bytesWritten;
}

qint64 AvatarMixerSlave::addChangedTraitsToBulkPacket(AvatarMixerClientData* listeningNodeData,
                                                      const AvatarMixerClientData* sendingNodeData,
                                                      NLPacketList& traitsPacketList) {

    // Avatar Traits flow control marks each outgoing avatar traits packet with a
    // sequence number. The mixer caches the traits sent in the traits packet.
    // Until an ack with the sequence number comes back, all updates to _traits
    // in that packet_ are ignored.  Updates to traits not in that packet will
    // be sent.

    auto sendingNodeLocalID = sendingNodeData->getNodeLocalID();

    // Perform a simple check with two server clock time points
    // to see if there is any new traits data for this avatar that we need to send
    auto timeOfLastTraitsSent = listeningNodeData->getLastOtherAvatarTraitsSendPoint(sendingNodeLocalID);
    auto timeOfLastTraitsChange = sendingNodeData->getLastReceivedTraitsChange();
    bool allTraitsUpdated = true;

    qint64 bytesWritten = 0;

    if (timeOfLastTraitsChange > timeOfLastTraitsSent) {
        // there is definitely new traits data to send

        auto sendingAvatar = sendingNodeData->getAvatarSharedPointer();

        // compare trait versions so we can see what exactly needs to go out
        auto& lastSentVersions = listeningNodeData->getLastSentTraitVersions(sendingNodeLocalID);
        auto& lastAckedVersions = listeningNodeData->getLastAckedTraitVersions(sendingNodeLocalID);
        const auto& lastReceivedVersions = sendingNodeData->getLastReceivedTraitVersions();

        auto simpleReceivedIt = lastReceivedVersions.simpleCBegin();
        while (simpleReceivedIt != lastReceivedVersions.simpleCEnd()) {
            auto traitType = static_cast<AvatarTraits::TraitType>(std::distance(lastReceivedVersions.simpleCBegin(),
                                                                                simpleReceivedIt));
            auto lastReceivedVersion = *simpleReceivedIt;
            auto& lastSentVersionRef = lastSentVersions[traitType];
            auto& lastAckedVersionRef = lastAckedVersions[traitType];

            // hold sending more traits until we've been acked that the last one we sent was received
            if (lastSentVersionRef == lastAckedVersionRef) {
                if (lastReceivedVersion > lastSentVersionRef) {
                    bytesWritten += addTraitsNodeHeader(listeningNodeData, sendingNodeData, traitsPacketList, bytesWritten);
                    // there is an update to this trait, add it to the traits packet
                    bytesWritten += AvatarTraits::packVersionedTrait(traitType, traitsPacketList,
                                                                     lastReceivedVersion, *sendingAvatar);
                    // update the last sent version
                    lastSentVersionRef = lastReceivedVersion;
                    // Remember which versions we sent in this particular packet
                    // so we can verify when it's acked.
                    auto& pendingTraitVersions = listeningNodeData->getPendingTraitVersions(listeningNodeData->getTraitsMessageSequence(), sendingNodeLocalID);
                    pendingTraitVersions[traitType] = lastReceivedVersion;
                }
            } else {
                allTraitsUpdated = false;
            }

            ++simpleReceivedIt;
        }

        // enumerate the received instanced trait versions
        auto instancedReceivedIt = lastReceivedVersions.instancedCBegin();
        while (instancedReceivedIt != lastReceivedVersions.instancedCEnd()) {
            auto traitType = instancedReceivedIt->traitType;

            // get or create the sent trait versions for this trait type
            auto& sentIDValuePairs = lastSentVersions.getInstanceIDValuePairs(traitType);
            auto& ackIDValuePairs = lastAckedVersions.getInstanceIDValuePairs(traitType);

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
                // look for existing acked version for this instance
                auto ackedInstanceIt = std::find_if(ackIDValuePairs.begin(), ackIDValuePairs.end(),
                                                    [instanceID](auto& ackInstance) { return ackInstance.id == instanceID; });
                
                // if we have a sent version, then we must have an acked instance of the same trait with the same
                // version to go on, otherwise we drop the received trait
                if (sentInstanceIt != sentIDValuePairs.end() &&
                    (ackedInstanceIt == ackIDValuePairs.end() || sentInstanceIt->value != ackedInstanceIt->value)) {
                    allTraitsUpdated = false;
                    continue;
                }
                if (!isDeleted && (sentInstanceIt == sentIDValuePairs.end() || receivedVersion > sentInstanceIt->value)) {
                    bytesWritten += addTraitsNodeHeader(listeningNodeData, sendingNodeData, traitsPacketList, bytesWritten);

                    // this instance version exists and has never been sent or is newer so we need to send it
                    bytesWritten += AvatarTraits::packVersionedTraitInstance(traitType, instanceID, traitsPacketList,
                                                                             receivedVersion, *sendingAvatar);

                    if (sentInstanceIt != sentIDValuePairs.end()) {
                        sentInstanceIt->value = receivedVersion;
                    } else {
                        sentIDValuePairs.emplace_back(instanceID, receivedVersion);
                    }

                    auto& pendingTraitVersions =
                        listeningNodeData->getPendingTraitVersions(listeningNodeData->getTraitsMessageSequence(),
                                                                   sendingNodeLocalID);
                    pendingTraitVersions.instanceInsert(traitType, instanceID, receivedVersion);

                } else if (isDeleted && sentInstanceIt != sentIDValuePairs.end() && absoluteReceivedVersion > sentInstanceIt->value) {
                    bytesWritten += addTraitsNodeHeader(listeningNodeData, sendingNodeData, traitsPacketList, bytesWritten);

                    // this instance version was deleted and we haven't sent the delete to this client yet
                    bytesWritten += AvatarTraits::packInstancedTraitDelete(traitType, instanceID, traitsPacketList, absoluteReceivedVersion);

                    // update the last sent version for this trait instance to the absolute value of the deleted version
                    sentInstanceIt->value = absoluteReceivedVersion;

                    auto& pendingTraitVersions =
                        listeningNodeData->getPendingTraitVersions(listeningNodeData->getTraitsMessageSequence(),
                                                                   sendingNodeLocalID);
                    pendingTraitVersions.instanceInsert(traitType, instanceID, absoluteReceivedVersion);

                }
            }

            ++instancedReceivedIt;
        }
        if (bytesWritten) {
            // write a null trait type to mark the end of trait data for this avatar
            bytesWritten += traitsPacketList.writePrimitive(AvatarTraits::NullTrait);
            // since we send all traits for this other avatar, update the time of last traits sent
            // to match the time of last traits change
            if (allTraitsUpdated) {
                listeningNodeData->setLastOtherAvatarTraitsSendPoint(sendingNodeLocalID, timeOfLastTraitsChange);
            }
        }
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
        _stats.numIdentityPacketsSent++;
        _stats.numIdentityBytesSent += individualData.size();
        return individualData.size();
    } else {
        return 0;
    }
}

static const int AVATAR_MIXER_BROADCAST_FRAMES_PER_SECOND = 45;

void AvatarMixerSlave::broadcastAvatarData(const SharedNodePointer& node) {
    quint64 start = usecTimestampNow();

    if ((node->getType() == NodeType::Agent || node->getType() == NodeType::EntityScriptServer) && node->getLinkedData() && node->getActiveSocket() && !node->isUpstream()) {
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

namespace {
    class SortableAvatar : public PrioritySortUtil::Sortable {
    public:
        SortableAvatar() = delete;
        SortableAvatar(const MixerAvatar* avatar, const Node* avatarNode, uint64_t lastEncodeTime)
            : _avatar(avatar), _node(avatarNode), _lastEncodeTime(lastEncodeTime) {
        }
        glm::vec3 getPosition() const override { return _avatar->getClientGlobalPosition(); }
        float getRadius() const override {
            glm::vec3 nodeBoxScale = _avatar->getGlobalBoundingBox().getScale();
            return 0.5f * glm::max(nodeBoxScale.x, glm::max(nodeBoxScale.y, nodeBoxScale.z));
        }
        uint64_t getTimestamp() const override {
            return _lastEncodeTime;
        }
        const Node* getNode() const { return _node; }
        const MixerAvatar* getAvatar() const { return _avatar; }

    private:
        const MixerAvatar* _avatar;
        const Node* _node;
        uint64_t _lastEncodeTime;
    };

}  // Close anonymous namespace.

void AvatarMixerSlave::broadcastAvatarDataToAgent(const SharedNodePointer& node) {
    const Node* destinationNode = node.data();

    auto nodeList = DependencyManager::get<NodeList>();

    // setup for distributed random floating point values
    std::random_device randomDevice;
    std::mt19937 generator(randomDevice());
    std::uniform_real_distribution<float> distribution;

    _stats.nodesBroadcastedTo++;

    AvatarMixerClientData* destinationNodeData = reinterpret_cast<AvatarMixerClientData*>(destinationNode->getLinkedData());

    destinationNodeData->resetInViewStats();

    const AvatarData& avatar = destinationNodeData->getAvatar();
    glm::vec3 destinationPosition = avatar.getClientGlobalPosition();

    // reset the internal state for correct random number distribution
    distribution.reset();

    // Estimate number to sort on number sent last frame (with min. of 20).
    const int numToSendEst = std::max(int(destinationNodeData->getNumAvatarsSentLastFrame() * 2.5f), 20);

    // reset the number of sent avatars
    destinationNodeData->resetNumAvatarsSentLastFrame();

    // keep track of outbound data rate specifically for avatar data
    int numAvatarDataBytes = 0;
    int identityBytesSent = 0;
    int traitBytesSent = 0;

    // max number of avatarBytes per frame (13 900, typical)
    const int maxAvatarBytesPerFrame = int(_maxKbpsPerNode * BYTES_PER_KILOBIT / AVATAR_MIXER_BROADCAST_FRAMES_PER_SECOND);
    const int maxHeroBytesPerFrame = int(maxAvatarBytesPerFrame * _avatarHeroFraction);  // 5555, typical

    // keep track of the number of other avatars held back in this frame
    int numAvatarsHeldBack = 0;

    // keep track of the number of other avatar frames skipped
    int numAvatarsWithSkippedFrames = 0;

    // When this is true, the AvatarMixer will send Avatar data to a client
    // about avatars they've ignored or that are out of view
    bool PALIsOpen = destinationNodeData->getRequestsDomainListData();
    bool PALWasOpen = destinationNodeData->getPrevRequestsDomainListData();

    // When this is true, the AvatarMixer will send Avatar data to a client about avatars that have ignored them
    bool getsAnyIgnored = PALIsOpen && destinationNode->getCanKick();

    // Bandwidth allowance for data that must be sent.
    int minimumBytesPerAvatar = PALIsOpen ? AvatarDataPacket::AVATAR_HAS_FLAGS_SIZE + NUM_BYTES_RFC4122_UUID +
        sizeof(AvatarDataPacket::AvatarGlobalPosition) + sizeof(AvatarDataPacket::AudioLoudness) : 0;

    // compute node bounding box
    const float MY_AVATAR_BUBBLE_EXPANSION_FACTOR = 4.0f; // magic number determined emperically
    AABox destinationNodeBox = computeBubbleBox(avatar, MY_AVATAR_BUBBLE_EXPANSION_FACTOR);

    // prepare to sort
    const auto& cameraViews = destinationNodeData->getViewFrustums();

    using AvatarPriorityQueue = PrioritySortUtil::PriorityQueue<SortableAvatar>;
    // Keep two independent queues, one for heroes and one for the riff-raff.
    enum PriorityVariants { kHero, kNonhero };
    AvatarPriorityQueue avatarPriorityQueues[2] =
    {
        {cameraViews, AvatarData::_avatarSortCoefficientSize, 
            AvatarData::_avatarSortCoefficientCenter, AvatarData::_avatarSortCoefficientAge},
        {cameraViews, AvatarData::_avatarSortCoefficientSize,
            AvatarData::_avatarSortCoefficientCenter, AvatarData::_avatarSortCoefficientAge}
    };

    avatarPriorityQueues[kNonhero].reserve(_end - _begin);

    for (auto listedNode = _begin; listedNode != _end; ++listedNode) {
        Node* otherNodeRaw = (*listedNode).data();
        if (otherNodeRaw->getType() != NodeType::Agent
            || !otherNodeRaw->getLinkedData()
            || otherNodeRaw == destinationNode) {
            continue;
        }

        auto sourceAvatarNode = otherNodeRaw;

        bool sendAvatar = true;  // We will consider this source avatar for sending.
        // We ignore other nodes for a couple of reasons:
        //   1) ignore bubbles and ignore specific node
        //   2) the node hasn't really updated it's frame data recently, this can
        //      happen if for example the avatar is connected on a desktop and sending
        //      updates at ~30hz. So every 3 frames we skip a frame.

        assert(sourceAvatarNode); // we can't have gotten here without the avatarData being a valid key in the map

        const AvatarMixerClientData* sourceAvatarNodeData = reinterpret_cast<const AvatarMixerClientData*>(sourceAvatarNode->getLinkedData());
        assert(sourceAvatarNodeData); // we can't have gotten here without sourceAvatarNode having valid data
        quint64 startIgnoreCalculation = usecTimestampNow();

        // make sure we have data for this avatar, that it isn't the same node,
        // and isn't an avatar that the viewing node has ignored
        // or that has ignored the viewing node
        if ((destinationNode->isIgnoringNodeWithID(sourceAvatarNode->getUUID()) && !PALIsOpen)
            || (sourceAvatarNode->isIgnoringNodeWithID(destinationNode->getUUID()) && !getsAnyIgnored)) {
            sendAvatar = false;
        } else {
            // Check to see if the space bubble is enabled
            // Don't bother with these checks if the other avatar has their bubble enabled and we're gettingAnyIgnored
            if (destinationNodeData->isIgnoreRadiusEnabled() || (sourceAvatarNodeData->isIgnoreRadiusEnabled() && !getsAnyIgnored)) {
                // Perform the collision check between the two bounding boxes
                AABox sourceNodeBox = sourceAvatarNodeData->getAvatar().getDefaultBubbleBox();
                if (destinationNodeBox.touches(sourceNodeBox)) {
                    destinationNodeData->ignoreOther(destinationNode, sourceAvatarNode);
                    sendAvatar = getsAnyIgnored;
                }
            }
            // Not close enough to ignore
            if (sendAvatar) {
                destinationNodeData->removeFromRadiusIgnoringSet(sourceAvatarNode->getUUID());
            }
        }

        // The source avatar should be removing its avatar entities. However, provide a back-up.
        if (sendAvatar) {
            if (!sourceAvatarNode->getCanRezAvatarEntities()) {
                auto sourceAvatarNodeData = reinterpret_cast<AvatarMixerClientData*>(sourceAvatarNode->getLinkedData());
                auto avatarEntityIDs = sourceAvatarNodeData->getAvatar().getAvatarEntityIDs();
                if (avatarEntityIDs.count() > 0) {
                    sourceAvatarNodeData->emulateDeleteEntitiesTraitsMessage(avatarEntityIDs);
                }
            }
        }

        if (sendAvatar) {
            AvatarDataSequenceNumber lastSeqToReceiver = destinationNodeData->getLastBroadcastSequenceNumber(sourceAvatarNode->getLocalID());
            AvatarDataSequenceNumber lastSeqFromSender = sourceAvatarNodeData->getLastReceivedSequenceNumber();

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
                sendAvatar = false;
            } else if (lastSeqFromSender == 0) {
                // We have have not yet received any data about this avatar. Ignore it for now
                // This is important for Agent scripts that are not avatar
                // so that they don't appear to be an avatar at the origin
                sendAvatar = false;
            } else if (lastSeqFromSender - lastSeqToReceiver > 1) {
                // this is a skip - we still send the packet but capture the presence of the skip so we see it happening
                ++numAvatarsWithSkippedFrames;
            }
        }

        quint64 endIgnoreCalculation = usecTimestampNow();
        _stats.ignoreCalculationElapsedTime += (endIgnoreCalculation - startIgnoreCalculation);

        if (sendAvatar) {
            // sort this one for later
            const MixerAvatar* avatarNodeData = sourceAvatarNodeData->getConstAvatarData();
            auto lastEncodeTime = destinationNodeData->getLastOtherAvatarEncodeTime(sourceAvatarNode->getLocalID());

            avatarPriorityQueues[avatarNodeData->getHasPriority() ? kHero : kNonhero].push(
                SortableAvatar(avatarNodeData, sourceAvatarNode, lastEncodeTime));
        }
        
        // If Node A's PAL WAS open but is no longer open, AND
        // Node A is ignoring Avatar B OR Node B is ignoring Avatar A...
        //
        // This is a bit heavy-handed still - there are cases where a kill packet
        // will be sent when it doesn't need to be (but where it _should_ be OK to send).
        // However, it's less heavy-handed than using `shouldIgnore`.
        if (PALWasOpen && !PALIsOpen &&
            (destinationNode->isIgnoringNodeWithID(sourceAvatarNode->getUUID()) ||
                sourceAvatarNode->isIgnoringNodeWithID(destinationNode->getUUID()))) {
            // ...send a Kill Packet to Node A, instructing Node A to kill Avatar B,
            // then have Node A cleanup the killed Node B.
            auto packet = NLPacket::create(PacketType::KillAvatar, NUM_BYTES_RFC4122_UUID + sizeof(KillAvatarReason), true);
            packet->write(sourceAvatarNode->getUUID().toRfc4122());
            packet->writePrimitive(KillAvatarReason::AvatarIgnored);
            nodeList->sendPacket(std::move(packet), *destinationNode);
            destinationNodeData->cleanupKilledNode(sourceAvatarNode->getUUID(), sourceAvatarNode->getLocalID());
        }

        destinationNodeData->setPrevRequestsDomainListData(PALIsOpen);
    }

    // loop through our sorted avatars and allocate our bandwidth to them accordingly

    int remainingAvatars = (int)avatarPriorityQueues[kHero].size() + (int)avatarPriorityQueues[kNonhero].size();
    auto traitsPacketList = NLPacketList::create(PacketType::BulkAvatarTraits, QByteArray(), true, true);

    auto avatarPacket = NLPacket::create(PacketType::BulkAvatarData);
    const int avatarPacketCapacity = avatarPacket->getPayloadCapacity();
    int avatarSpaceAvailable = avatarPacketCapacity;
    int numPacketsSent = 0;
    int numAvatarsSent = 0;
    auto identityPacketList = NLPacketList::create(PacketType::AvatarIdentity, QByteArray(), true, true);

    // Loop over two priorities - hero avatars then everyone else:
    for (PriorityVariants currentVariant = kHero; currentVariant <= kNonhero; ++((int&)currentVariant)) {
        const auto& sortedAvatarVector = avatarPriorityQueues[currentVariant].getSortedVector(numToSendEst);
        for (const auto& sortedAvatar : sortedAvatarVector) {
            const Node* sourceNode = sortedAvatar.getNode();
            auto lastEncodeForOther = sortedAvatar.getTimestamp();

            assert(sourceNode); // we can't have gotten here without the avatarData being a valid key in the map

            AvatarData::AvatarDataDetail detail = AvatarData::NoData;

            // NOTE: Here's where we determine if we are over budget and drop remaining avatars,
            // or send minimal avatar data in uncommon case of PALIsOpen.
            int minimRemainingAvatarBytes = minimumBytesPerAvatar * remainingAvatars;
            auto frameByteEstimate = identityBytesSent + traitBytesSent + numAvatarDataBytes + minimRemainingAvatarBytes;
            bool overBudget = frameByteEstimate > maxAvatarBytesPerFrame;
            if (overBudget) {
                if (PALIsOpen) {
                    _stats.overBudgetAvatars++;
                    detail = AvatarData::PALMinimum;
                } else {
                    _stats.overBudgetAvatars += remainingAvatars;
                    break;
                }
            }

            bool overHeroBudget = currentVariant == kHero && numAvatarDataBytes > maxHeroBytesPerFrame;
            if (overHeroBudget) {
                break;  // No more heroes (this frame).
            }

            auto startAvatarDataPacking = chrono::high_resolution_clock::now();

            const AvatarMixerClientData* sourceNodeData = reinterpret_cast<const AvatarMixerClientData*>(sourceNode->getLinkedData());
            const MixerAvatar* sourceAvatar = sourceNodeData->getConstAvatarData();

            // Typically all out-of-view avatars but such avatars' priorities will rise with time:
            bool isLowerPriority = sortedAvatar.getPriority() <= OUT_OF_VIEW_THRESHOLD;

            if (isLowerPriority) {
                detail = PALIsOpen ? AvatarData::PALMinimum : AvatarData::MinimumData;
                destinationNodeData->incrementAvatarOutOfView();
            } else if (!overBudget) {
                detail = distribution(generator) < AVATAR_SEND_FULL_UPDATE_RATIO ? AvatarData::SendAllData : AvatarData::CullSmallData;
                destinationNodeData->incrementAvatarInView();

                // If the time that the mixer sent AVATAR DATA about Avatar B to Node A is BEFORE OR EQUAL TO
                // the time that Avatar B flagged an IDENTITY DATA change, send IDENTITY DATA about Avatar B to Node A.
                if (sourceAvatar->hasProcessedFirstIdentity()
                    && destinationNodeData->getLastBroadcastTime(sourceNode->getLocalID()) <= sourceNodeData->getIdentityChangeTimestamp()) {
                    identityBytesSent += sendIdentityPacket(*identityPacketList, sourceNodeData, *destinationNode);

                    // remember the last time we sent identity details about this other node to the receiver
                    destinationNodeData->setLastBroadcastTime(sourceNode->getLocalID(), usecTimestampNow());
                }
            }

            QVector<JointData>& lastSentJointsForOther = destinationNodeData->getLastOtherAvatarSentJoints(sourceNode->getLocalID());

            const bool distanceAdjust = true;
            const bool dropFaceTracking = false;
            AvatarDataPacket::SendStatus sendStatus;
            sendStatus.sendUUID = true;

            do {
                auto startSerialize = chrono::high_resolution_clock::now();
                QByteArray bytes = sourceAvatar->toByteArray(detail, lastEncodeForOther, lastSentJointsForOther,
                    sendStatus, dropFaceTracking, distanceAdjust, destinationPosition,
                    &lastSentJointsForOther, avatarSpaceAvailable);
                auto endSerialize = chrono::high_resolution_clock::now();
                _stats.toByteArrayElapsedTime +=
                    (quint64)chrono::duration_cast<chrono::microseconds>(endSerialize - startSerialize).count();

                avatarPacket->write(bytes);
                avatarSpaceAvailable -= bytes.size();
                numAvatarDataBytes += bytes.size();
                if (!sendStatus || avatarSpaceAvailable < (int)AvatarDataPacket::MIN_BULK_PACKET_SIZE) {
                    // Weren't able to fit everything.
                    nodeList->sendPacket(std::move(avatarPacket), *destinationNode);
                    ++numPacketsSent;
                    avatarPacket = NLPacket::create(PacketType::BulkAvatarData);
                    avatarSpaceAvailable = avatarPacketCapacity;
                }
            } while (!sendStatus);

            if (detail != AvatarData::NoData) {
                _stats.numOthersIncluded++;
                if (sourceAvatar->getHasPriority()) {
                    _stats.numHeroesIncluded++;
                }

                // increment the number of avatars sent to this receiver
                destinationNodeData->incrementNumAvatarsSentLastFrame();

                // set the last sent sequence number for this sender on the receiver
                destinationNodeData->setLastBroadcastSequenceNumber(sourceNode->getLocalID(),
                    sourceNodeData->getLastReceivedSequenceNumber());
                destinationNodeData->setLastOtherAvatarEncodeTime(sourceNode->getLocalID(), usecTimestampNow());
            }

            auto endAvatarDataPacking = chrono::high_resolution_clock::now();
            _stats.avatarDataPackingElapsedTime +=
                (quint64)chrono::duration_cast<chrono::microseconds>(endAvatarDataPacking - startAvatarDataPacking).count();

            if (!overBudget) {
                // use helper to add any changed traits to our packet list
                traitBytesSent += addChangedTraitsToBulkPacket(destinationNodeData, sourceNodeData, *traitsPacketList);
            }
            numAvatarsSent++;
            remainingAvatars--;
        }

        if (currentVariant == kHero) {  // Dump any remaining heroes into the commoners.
            for (auto avIter = sortedAvatarVector.begin() + numAvatarsSent; avIter < sortedAvatarVector.end(); ++avIter) {
                avatarPriorityQueues[kNonhero].push(*avIter);
            }
        }
    }

    if (destinationNodeData->getNumAvatarsSentLastFrame() > numToSendEst) {
        qCWarning(avatars) << "More avatars sent than upper estimate" << destinationNodeData->getNumAvatarsSentLastFrame()
            << " / " << numToSendEst;
    }

    quint64 startPacketSending = usecTimestampNow();

    if (avatarPacket->getPayloadSize() != 0) {
        nodeList->sendPacket(std::move(avatarPacket), *destinationNode);
        ++numPacketsSent;
    }

    _stats.numDataPacketsSent += numPacketsSent;
    _stats.numDataBytesSent += numAvatarDataBytes;

    // close the current traits packet list
    traitsPacketList->closeCurrentPacket();

    if (traitsPacketList->getNumPackets() >= 1) {
        // send the traits packet list
        _stats.numTraitsBytesSent += traitBytesSent;
        _stats.numTraitsPacketsSent += (int) traitsPacketList->getNumPackets();
        nodeList->sendPacketList(std::move(traitsPacketList), *destinationNode);
    }

    // Send any AvatarIdentity packets:
    identityPacketList->closeCurrentPacket();
    if (identityBytesSent > 0) {
        nodeList->sendPacketList(std::move(identityPacketList), *destinationNode);
    }

    // record the bytes sent for other avatar data in the AvatarMixerClientData
    destinationNodeData->recordSentAvatarData(numAvatarDataBytes, traitBytesSent);


    // record the number of avatars held back this frame
    destinationNodeData->recordNumOtherAvatarStarves(numAvatarsHeldBack);
    destinationNodeData->recordNumOtherAvatarSkips(numAvatarsWithSkippedFrames);

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
            AvatarDataPacket::SendStatus sendStatus;

            QVector<JointData> emptyLastJointSendData { otherAvatar->getJointCount() };

            QByteArray avatarByteArray = otherAvatar->toByteArray(AvatarData::SendAllData, 0, emptyLastJointSendData,
                sendStatus, false, false, glm::vec3(0), nullptr, 0);
            quint64 end = usecTimestampNow();
            _stats.toByteArrayElapsedTime += (end - start);

            auto lastBroadcastTime = nodeData->getLastBroadcastTime(agentNode->getLocalID());
            if (lastBroadcastTime <= agentNodeData->getIdentityChangeTimestamp()
                || (start - lastBroadcastTime) >= REBROADCAST_IDENTITY_TO_DOWNSTREAM_EVERY_US) {
                sendReplicatedIdentityPacket(*agentNode, agentNodeData, *node);
                nodeData->setLastBroadcastTime(agentNode->getLocalID(), start);
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
                    sendStatus, true, false, glm::vec3(0), nullptr, 0);

                if (avatarByteArray.size() > maxAvatarByteArraySize) {
                    qCWarning(avatars) << "Replicated avatar data without facial data still too large for"
                        << otherAvatar->getSessionUUID() << "-" << avatarByteArray.size() << "bytes";

                    avatarByteArray = otherAvatar->toByteArray(AvatarData::MinimumData, 0, emptyLastJointSendData,
                        sendStatus, true, false, glm::vec3(0), nullptr, 0);
                }
            }

            if (avatarByteArray.size() <= maxAvatarByteArraySize) {
                // increment the number of avatars sent to this reciever
                nodeData->incrementNumAvatarsSentLastFrame();

                // set the last sent sequence number for this sender on the receiver
                nodeData->setLastBroadcastSequenceNumber(agentNode->getLocalID(),
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

        _stats.numDataPacketsSent += (int)avatarPacketList->getNumPackets();
        _stats.numDataBytesSent += numAvatarDataBytes;

        // send the replicated bulk avatar data
        auto nodeList = DependencyManager::get<NodeList>();
        nodeList->sendPacketList(std::move(avatarPacketList), node->getPublicSocket());

        // record the bytes sent for other avatar data in the AvatarMixerClientData
        nodeData->recordSentAvatarData(numAvatarDataBytes);

        quint64 endPacketSending = usecTimestampNow();
        _stats.packetSendingElapsedTime += (endPacketSending - startPacketSending);
    }
}

