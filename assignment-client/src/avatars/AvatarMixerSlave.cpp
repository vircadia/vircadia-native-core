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


void AvatarMixerSlave::sendIdentityPacket(const AvatarMixerClientData* nodeData, const SharedNodePointer& destinationNode) {
    QByteArray individualData = nodeData->getConstAvatarData()->identityByteArray();
    auto identityPacket = NLPacket::create(PacketType::AvatarIdentity, individualData.size());
    individualData.replace(0, NUM_BYTES_RFC4122_UUID, nodeData->getNodeID().toRfc4122());
    identityPacket->write(individualData);
    DependencyManager::get<NodeList>()->sendPacket(std::move(identityPacket), *destinationNode);
    _stats.numIdentityPackets++;
}

static const int AVATAR_MIXER_BROADCAST_FRAMES_PER_SECOND = 45;

// only send extra avatar data (avatars out of view, ignored) every Nth AvatarData frame
// Extra avatar data will be sent (AVATAR_MIXER_BROADCAST_FRAMES_PER_SECOND/EXTRA_AVATAR_DATA_FRAME_RATIO) times
// per second.
// This value should be a power of two for performance purposes, as the mixer performs a modulo operation every frame
// to determine whether the extra data should be sent.
static const int EXTRA_AVATAR_DATA_FRAME_RATIO = 16;

// FIXME - There is some old logic (unchanged as of 2/17/17) that randomly decides to send an identity
// packet. That logic had the following comment about the constants it uses...
//
//         An 80% chance of sending a identity packet within a 5 second interval.
//         assuming 60 htz update rate.
//
// Assuming the calculation of the constant is in fact correct for 80% and 60hz and 5 seconds (an assumption
// that I have not verified) then the constant is definitely wrong now, since we send at 45hz.
const float IDENTITY_SEND_PROBABILITY = 1.0f / 187.0f;

void AvatarMixerSlave::broadcastAvatarData(const SharedNodePointer& node) {
    quint64 start = usecTimestampNow();

    auto nodeList = DependencyManager::get<NodeList>();

    // setup for distributed random floating point values
    std::random_device randomDevice;
    std::mt19937 generator(randomDevice());
    std::uniform_real_distribution<float> distribution;

    if (node->getLinkedData() && (node->getType() == NodeType::Agent) && node->getActiveSocket()) {
        _stats.nodesBroadcastedTo++;

        AvatarMixerClientData* nodeData = reinterpret_cast<AvatarMixerClientData*>(node->getLinkedData());

        nodeData->resetInViewStats();

        const AvatarData& avatar = nodeData->getAvatar();
        glm::vec3 myPosition = avatar.getClientGlobalPosition();

        // reset the internal state for correct random number distribution
        distribution.reset();

        // reset the max distance for this frame
        float maxAvatarDistanceThisFrame = 0.0f;

        // reset the number of sent avatars
        nodeData->resetNumAvatarsSentLastFrame();

        // keep a counter of the number of considered avatars
        int numOtherAvatars = 0;

        // keep track of outbound data rate specifically for avatar data
        int numAvatarDataBytes = 0;

        // keep track of the number of other avatars held back in this frame
        int numAvatarsHeldBack = 0;

        // keep track of the number of other avatar frames skipped
        int numAvatarsWithSkippedFrames = 0;

        // use the data rate specifically for avatar data for FRD adjustment checks
        float avatarDataRateLastSecond = nodeData->getOutboundAvatarDataKbps();

        // When this is true, the AvatarMixer will send Avatar data to a client about avatars that are not in the view frustrum
        bool getsOutOfView = nodeData->getRequestsDomainListData();

        // When this is true, the AvatarMixer will send Avatar data to a client about avatars that they've ignored
        bool getsIgnoredByMe = getsOutOfView;

        // When this is true, the AvatarMixer will send Avatar data to a client about avatars that have ignored them
        bool getsAnyIgnored = getsIgnoredByMe && node->getCanKick();

        // Check if it is time to adjust what we send this client based on the observed
        // bandwidth to this node. We do this once a second, which is also the window for
        // the bandwidth reported by node->getOutboundBandwidth();
        if (nodeData->getNumFramesSinceFRDAdjustment() > AVATAR_MIXER_BROADCAST_FRAMES_PER_SECOND) {

            const float FRD_ADJUSTMENT_ACCEPTABLE_RATIO = 0.8f;
            const float HYSTERISIS_GAP = (1 - FRD_ADJUSTMENT_ACCEPTABLE_RATIO);
            const float HYSTERISIS_MIDDLE_PERCENTAGE = (1 - (HYSTERISIS_GAP * 0.5f));

            // get the current full rate distance so we can work with it
            float currentFullRateDistance = nodeData->getFullRateDistance();

            if (avatarDataRateLastSecond > _maxKbpsPerNode) {

                // is the FRD greater than the farthest avatar?
                // if so, before we calculate anything, set it to that distance
                currentFullRateDistance = std::min(currentFullRateDistance, nodeData->getMaxAvatarDistance());

                // we're adjusting the full rate distance to target a bandwidth in the middle
                // of the hysterisis gap
                currentFullRateDistance *= (_maxKbpsPerNode * HYSTERISIS_MIDDLE_PERCENTAGE) / avatarDataRateLastSecond;

                nodeData->setFullRateDistance(currentFullRateDistance);
                nodeData->resetNumFramesSinceFRDAdjustment();
            } else if (currentFullRateDistance < nodeData->getMaxAvatarDistance()
                && avatarDataRateLastSecond < _maxKbpsPerNode * FRD_ADJUSTMENT_ACCEPTABLE_RATIO) {
                // we are constrained AND we've recovered to below the acceptable ratio
                // lets adjust the full rate distance to target a bandwidth in the middle of the hyterisis gap
                currentFullRateDistance *= (_maxKbpsPerNode * HYSTERISIS_MIDDLE_PERCENTAGE) / avatarDataRateLastSecond;

                nodeData->setFullRateDistance(currentFullRateDistance);
                nodeData->resetNumFramesSinceFRDAdjustment();
            }
        } else {
            nodeData->incrementNumFramesSinceFRDAdjustment();
        }

        // setup a PacketList for the avatarPackets
        auto avatarPacketList = NLPacketList::create(PacketType::BulkAvatarData);

        // this is an AGENT we have received head data from
        // send back a packet with other active node data to this node
        std::for_each(_begin, _end, [&](const SharedNodePointer& otherNode) {

            bool shouldConsider = false;
            quint64 startIgnoreCalculation = usecTimestampNow();

            // make sure we have data for this avatar, that it isn't the same node,
            // and isn't an avatar that the viewing node has ignored
            // or that has ignored the viewing node
            if (!otherNode->getLinkedData()
                || otherNode->getUUID() == node->getUUID()
                || (node->isIgnoringNodeWithID(otherNode->getUUID()) && !getsIgnoredByMe)
                || (otherNode->isIgnoringNodeWithID(node->getUUID()) && !getsAnyIgnored)) {

                shouldConsider = false;

            } else {
                const AvatarMixerClientData* otherData = reinterpret_cast<AvatarMixerClientData*>(otherNode->getLinkedData());

                shouldConsider = true; // assume we will consider...

                // Check to see if the space bubble is enabled
                if (node->isIgnoreRadiusEnabled() || otherNode->isIgnoreRadiusEnabled()) {
                    // Define the minimum bubble size
                    static const glm::vec3 minBubbleSize = glm::vec3(0.3f, 1.3f, 0.3f);
                    // Define the scale of the box for the current node
                    glm::vec3 nodeBoxScale = (nodeData->getPosition() - nodeData->getGlobalBoundingBoxCorner()) * 2.0f;
                    // Define the scale of the box for the current other node
                    glm::vec3 otherNodeBoxScale = (otherData->getPosition() - otherData->getGlobalBoundingBoxCorner()) * 2.0f;

                    // Set up the bounding box for the current node
                    AABox nodeBox(nodeData->getGlobalBoundingBoxCorner(), nodeBoxScale);
                    // Clamp the size of the bounding box to a minimum scale
                    if (glm::any(glm::lessThan(nodeBoxScale, minBubbleSize))) {
                        nodeBox.setScaleStayCentered(minBubbleSize);
                    }
                    // Set up the bounding box for the current other node
                    AABox otherNodeBox(otherData->getGlobalBoundingBoxCorner(), otherNodeBoxScale);
                    // Clamp the size of the bounding box to a minimum scale
                    if (glm::any(glm::lessThan(otherNodeBoxScale, minBubbleSize))) {
                        otherNodeBox.setScaleStayCentered(minBubbleSize);
                    }
                    // Quadruple the scale of both bounding boxes
                    nodeBox.embiggen(4.0f);
                    otherNodeBox.embiggen(4.0f);

                    // Perform the collision check between the two bounding boxes
                    if (nodeBox.touches(otherNodeBox)) {
                        nodeData->ignoreOther(node, otherNode);
                        shouldConsider = getsAnyIgnored;
                    }
                }
                // Not close enough to ignore
                if (shouldConsider) {
                    nodeData->removeFromRadiusIgnoringSet(node, otherNode->getUUID());
                }

                quint64 endIgnoreCalculation = usecTimestampNow();
                _stats.ignoreCalculationElapsedTime += (endIgnoreCalculation - startIgnoreCalculation);
            }

            if (shouldConsider) {
                quint64 startAvatarDataPacking = usecTimestampNow();

                ++numOtherAvatars;

                const AvatarMixerClientData* otherNodeData = reinterpret_cast<const AvatarMixerClientData*>(otherNode->getLinkedData());

                // make sure we send out identity packets to and from new arrivals.
                bool forceSend = !nodeData->checkAndSetHasReceivedFirstPacketsFrom(otherNode->getUUID());

                // FIXME - this clause seems suspicious "... || otherNodeData->getIdentityChangeTimestamp() > _lastFrameTimestamp ..."
                if (otherNodeData->getIdentityChangeTimestamp().time_since_epoch().count() > 0
                    && (forceSend
                    || otherNodeData->getIdentityChangeTimestamp() > _lastFrameTimestamp
                    || distribution(generator) < IDENTITY_SEND_PROBABILITY)) {

                    sendIdentityPacket(otherNodeData, node);
                }

                const AvatarData* otherAvatar = otherNodeData->getConstAvatarData();
                //  Decide whether to send this avatar's data based on it's distance from us

                //  The full rate distance is the distance at which EVERY update will be sent for this avatar
                //  at twice the full rate distance, there will be a 50% chance of sending this avatar's update
                glm::vec3 otherPosition = otherAvatar->getClientGlobalPosition();
                float distanceToAvatar = glm::length(myPosition - otherPosition);

                // potentially update the max full rate distance for this frame
                maxAvatarDistanceThisFrame = std::max(maxAvatarDistanceThisFrame, distanceToAvatar);

                // This code handles the random dropping of avatar data based on the ratio of
                // "getFullRateDistance" to actual distance.
                //
                // NOTE: If the recieving node is in "PAL mode" then it's asked to get things even that
                //       are out of view, this also appears to disable this random distribution.
                if (distanceToAvatar != 0.0f
                    && !getsOutOfView
                    && distribution(generator) > (nodeData->getFullRateDistance() / distanceToAvatar)) {

                    quint64 endAvatarDataPacking = usecTimestampNow();
                    _stats.avatarDataPackingElapsedTime += (endAvatarDataPacking - startAvatarDataPacking);
                    shouldConsider = false;
                }

                if (shouldConsider) {
                    AvatarDataSequenceNumber lastSeqToReceiver = nodeData->getLastBroadcastSequenceNumber(otherNode->getUUID());
                    AvatarDataSequenceNumber lastSeqFromSender = otherNodeData->getLastReceivedSequenceNumber();

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

                        quint64 endAvatarDataPacking = usecTimestampNow();
                        _stats.avatarDataPackingElapsedTime += (endAvatarDataPacking - startAvatarDataPacking);
                        shouldConsider = false;
                    } else if (lastSeqFromSender - lastSeqToReceiver > 1) {
                        // this is a skip - we still send the packet but capture the presence of the skip so we see it happening
                        ++numAvatarsWithSkippedFrames;
                    }

                    // we're going to send this avatar
                    if (shouldConsider) {

                        // determine if avatar is in view, to determine how much data to include...
                        glm::vec3 otherNodeBoxScale = (otherPosition - otherNodeData->getGlobalBoundingBoxCorner()) * 2.0f;
                        AABox otherNodeBox(otherNodeData->getGlobalBoundingBoxCorner(), otherNodeBoxScale);
                        bool isInView = nodeData->otherAvatarInView(otherNodeBox);

                        // this throttles the extra data to only be sent every Nth message
                        if (!isInView && !getsOutOfView && (lastSeqToReceiver % EXTRA_AVATAR_DATA_FRAME_RATIO > 0)) {
                            quint64 endAvatarDataPacking = usecTimestampNow();

                            _stats.avatarDataPackingElapsedTime += (endAvatarDataPacking - startAvatarDataPacking);
                            shouldConsider = false;
                        }

                        if (shouldConsider) {
                            // start a new segment in the PacketList for this avatar
                            avatarPacketList->startSegment();

                            AvatarData::AvatarDataDetail detail;
                            if (!isInView && !getsOutOfView) {
                                detail = AvatarData::MinimumData;
                                nodeData->incrementAvatarOutOfView();
                            } else {
                                detail = distribution(generator) < AVATAR_SEND_FULL_UPDATE_RATIO
                                    ? AvatarData::SendAllData : AvatarData::CullSmallData;
                                nodeData->incrementAvatarInView();
                            }

                            {
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
                                    }

                                    if (bytes.size() > MAX_ALLOWED_AVATAR_DATA) {
                                        qCWarning(avatars) << "otherAvatar.toByteArray() MinimumData resulted in very large buffer:" << bytes.size() << "... FAIL!!";
                                        includeThisAvatar = false;
                                    }
                                }

                                if (includeThisAvatar) {
                                    numAvatarDataBytes += avatarPacketList->write(otherNode->getUUID().toRfc4122());
                                    numAvatarDataBytes += avatarPacketList->write(bytes);
                                    _stats.numOthersIncluded++;

                                    // increment the number of avatars sent to this reciever
                                    nodeData->incrementNumAvatarsSentLastFrame();

                                    // set the last sent sequence number for this sender on the receiver
                                    nodeData->setLastBroadcastSequenceNumber(otherNode->getUUID(),
                                        otherNodeData->getLastReceivedSequenceNumber());

                                }
                            }

                            avatarPacketList->endSegment();

                            quint64 endAvatarDataPacking = usecTimestampNow();
                            _stats.avatarDataPackingElapsedTime += (endAvatarDataPacking - startAvatarDataPacking);
                        }
                    }
                }
            }
        });

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

        if (numOtherAvatars == 0) {
            // update the full rate distance to FLOAT_MAX since we didn't have any other avatars to send
            nodeData->setMaxAvatarDistance(FLT_MAX);
        } else {
            nodeData->setMaxAvatarDistance(maxAvatarDistanceThisFrame);
        }

        quint64 endPacketSending = usecTimestampNow();
        _stats.packetSendingElapsedTime += (endPacketSending - startPacketSending);
    }

    quint64 end = usecTimestampNow();
    _stats.jobElapsedTime += (end - start);
}

