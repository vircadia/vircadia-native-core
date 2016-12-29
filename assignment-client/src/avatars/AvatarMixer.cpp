//
//  AvatarMixer.cpp
//  assignment-client/src/avatars
//
//  Created by Stephen Birarda on 9/5/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <cfloat>
#include <random>
#include <memory>

#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtCore/QJsonObject>
#include <QtCore/QRegularExpression>
#include <QtCore/QTimer>
#include <QtCore/QThread>

#include <AABox.h>
#include <LogHandler.h>
#include <NodeList.h>
#include <udt/PacketHeaders.h>
#include <SharedUtil.h>
#include <UUID.h>
#include <TryLocker.h>

#include "AvatarMixer.h"

const QString AVATAR_MIXER_LOGGING_NAME = "avatar-mixer";

// FIXME - what we'd actually like to do is send to users at ~50% of their present rate down to 30hz. Assume 90 for now.
const int AVATAR_MIXER_BROADCAST_FRAMES_PER_SECOND = 45;
const unsigned int AVATAR_DATA_SEND_INTERVAL_MSECS = (1.0f / (float) AVATAR_MIXER_BROADCAST_FRAMES_PER_SECOND) * 1000;

AvatarMixer::AvatarMixer(ReceivedMessage& message) :
    ThreadedAssignment(message),
    _broadcastThread()
{
    // make sure we hear about node kills so we can tell the other nodes
    connect(DependencyManager::get<NodeList>().data(), &NodeList::nodeKilled, this, &AvatarMixer::nodeKilled);

    auto& packetReceiver = DependencyManager::get<NodeList>()->getPacketReceiver();
    packetReceiver.registerListener(PacketType::ViewFrustum, this, "handleViewFrustumPacket");
    packetReceiver.registerListener(PacketType::AvatarData, this, "handleAvatarDataPacket");
    packetReceiver.registerListener(PacketType::AvatarIdentity, this, "handleAvatarIdentityPacket");
    packetReceiver.registerListener(PacketType::KillAvatar, this, "handleKillAvatarPacket");
    packetReceiver.registerListener(PacketType::NodeIgnoreRequest, this, "handleNodeIgnoreRequestPacket");
    packetReceiver.registerListener(PacketType::RadiusIgnoreRequest, this, "handleRadiusIgnoreRequestPacket");
    packetReceiver.registerListener(PacketType::RequestsDomainListData, this, "handleRequestsDomainListDataPacket");

    auto nodeList = DependencyManager::get<NodeList>();
    connect(nodeList.data(), &NodeList::packetVersionMismatch, this, &AvatarMixer::handlePacketVersionMismatch);
}

AvatarMixer::~AvatarMixer() {
    if (_broadcastTimer) {
        _broadcastTimer->deleteLater();
    }

    _broadcastThread.quit();
    _broadcastThread.wait();
}

// An 80% chance of sending a identity packet within a 5 second interval.
// assuming 60 htz update rate.
const float IDENTITY_SEND_PROBABILITY = 1.0f / 187.0f;

void AvatarMixer::sendIdentityPacket(AvatarMixerClientData* nodeData, const SharedNodePointer& destinationNode) {
    QByteArray individualData = nodeData->getAvatar().identityByteArray();

    auto identityPacket = NLPacket::create(PacketType::AvatarIdentity, individualData.size());

    individualData.replace(0, NUM_BYTES_RFC4122_UUID, nodeData->getNodeID().toRfc4122());

    identityPacket->write(individualData);

    DependencyManager::get<NodeList>()->sendPacket(std::move(identityPacket), *destinationNode);

    ++_sumIdentityPackets;
}

// NOTE: some additional optimizations to consider.
//    1) use the view frustum to cull those avatars that are out of view. Since avatar data doesn't need to be present
//       if the avatar is not in view or in the keyhole.
void AvatarMixer::broadcastAvatarData() {
    _broadcastRate.increment();

    int idleTime = AVATAR_DATA_SEND_INTERVAL_MSECS;

    if (_lastFrameTimestamp.time_since_epoch().count() > 0) {
        auto idleDuration = p_high_resolution_clock::now() - _lastFrameTimestamp;
        idleTime = std::chrono::duration_cast<std::chrono::microseconds>(idleDuration).count();
    }

    ++_numStatFrames;

    const float STRUGGLE_TRIGGER_SLEEP_PERCENTAGE_THRESHOLD = 0.10f;
    const float BACK_OFF_TRIGGER_SLEEP_PERCENTAGE_THRESHOLD = 0.20f;

    const float RATIO_BACK_OFF = 0.02f;

    const int TRAILING_AVERAGE_FRAMES = 100;
    int framesSinceCutoffEvent = TRAILING_AVERAGE_FRAMES;

    const float CURRENT_FRAME_RATIO = 1.0f / TRAILING_AVERAGE_FRAMES;
    const float PREVIOUS_FRAMES_RATIO = 1.0f - CURRENT_FRAME_RATIO;

    // only send extra avatar data (avatars out of view, ignored) every Nth AvatarData frame
    const int EXTRA_AVATAR_DATA_FRAME_RATIO = 16; 
    
    // NOTE: The following code calculates the _performanceThrottlingRatio based on how much the avatar-mixer was
    // able to sleep. This will eventually be used to ask for an additional avatar-mixer to help out. Currently the value
    // is unused as it is assumed this should not be hit before the avatar-mixer hits the desired bandwidth limit per client.
    // It is reported in the domain-server stats for the avatar-mixer.

    _trailingSleepRatio = (PREVIOUS_FRAMES_RATIO * _trailingSleepRatio)
        + (idleTime * CURRENT_FRAME_RATIO / (float) AVATAR_DATA_SEND_INTERVAL_MSECS);

    float lastCutoffRatio = _performanceThrottlingRatio;
    bool hasRatioChanged = false;

    if (framesSinceCutoffEvent >= TRAILING_AVERAGE_FRAMES) {
        if (_trailingSleepRatio <= STRUGGLE_TRIGGER_SLEEP_PERCENTAGE_THRESHOLD) {
            // we're struggling - change our performance throttling ratio
            _performanceThrottlingRatio = _performanceThrottlingRatio + (0.5f * (1.0f - _performanceThrottlingRatio));

            qDebug() << "Mixer is struggling, sleeping" << _trailingSleepRatio * 100 << "% of frame time. Old cutoff was"
                << lastCutoffRatio << "and is now" << _performanceThrottlingRatio;
            hasRatioChanged = true;
        } else if (_trailingSleepRatio >= BACK_OFF_TRIGGER_SLEEP_PERCENTAGE_THRESHOLD && _performanceThrottlingRatio != 0) {
            // we've recovered and can back off the performance throttling
            _performanceThrottlingRatio = _performanceThrottlingRatio - RATIO_BACK_OFF;

            if (_performanceThrottlingRatio < 0) {
                _performanceThrottlingRatio = 0;
            }

            qDebug() << "Mixer is recovering, sleeping" << _trailingSleepRatio * 100 << "% of frame time. Old cutoff was"
                << lastCutoffRatio << "and is now" << _performanceThrottlingRatio;
            hasRatioChanged = true;
        }

        if (hasRatioChanged) {
            framesSinceCutoffEvent = 0;
        }
    }

    if (!hasRatioChanged) {
        ++framesSinceCutoffEvent;
    }

    auto nodeList = DependencyManager::get<NodeList>();

    // setup for distributed random floating point values
    std::random_device randomDevice;
    std::mt19937 generator(randomDevice());
    std::uniform_real_distribution<float> distribution;

    nodeList->eachMatchingNode(
        [&](const SharedNodePointer& node)->bool {
            if (!node->getLinkedData()) {
                return false;
            }
            if (node->getType() != NodeType::Agent) {
                return false;
            }
            if (!node->getActiveSocket()) {
                return false;
            }
            return true;
        },
        [&](const SharedNodePointer& node) {
            AvatarMixerClientData* nodeData = reinterpret_cast<AvatarMixerClientData*>(node->getLinkedData());
            MutexTryLocker lock(nodeData->getMutex());
            if (!lock.isLocked()) {
                return;
            }
            ++_sumListeners;
            nodeData->resetInViewStats();

            AvatarData& avatar = nodeData->getAvatar();
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
                const float HYSTERISIS_MIDDLE_PERCENTAGE =  (1 - (HYSTERISIS_GAP * 0.5f));

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

            if (avatar.getSessionDisplayName().isEmpty() &&  // We haven't set it yet...
                nodeData->getReceivedIdentity()) { // ... but we have processed identity (with possible displayName).
                QString baseName = avatar.getDisplayName().trimmed();
                const QRegularExpression curses{ "fuck|shit|damn|cock|cunt" }; // POC. We may eventually want something much more elaborate (subscription?).
                baseName = baseName.replace(curses, "*"); // Replace rather than remove, so that people have a clue that the person's a jerk.
                const QRegularExpression trailingDigits{ "\\s*_\\d+$" }; // whitespace "_123"
                baseName = baseName.remove(trailingDigits);
                if (baseName.isEmpty()) {
                    baseName = "anonymous";
                }

                QPair<int, int>& soFar = _sessionDisplayNames[baseName]; // Inserts and answers 0, 0 if not already present, which is what we want.
                int& highWater = soFar.first;
                nodeData->setBaseDisplayName(baseName);
                avatar.setSessionDisplayName((highWater > 0) ? baseName + "_" + QString::number(highWater) : baseName);
                highWater++;
                soFar.second++; // refcount
                nodeData->flagIdentityChange();
                sendIdentityPacket(nodeData, node); // Tell new node about its sessionUUID. Others will find out below.
            }

            // this is an AGENT we have received head data from
            // send back a packet with other active node data to this node
            nodeList->eachMatchingNode(
                [&](const SharedNodePointer& otherNode)->bool {
                    // make sure we have data for this avatar, that it isn't the same node,
                    // and isn't an avatar that the viewing node has ignored
                    // or that has ignored the viewing node
                    if (!otherNode->getLinkedData()
                        || otherNode->getUUID() == node->getUUID()
                        || (node->isIgnoringNodeWithID(otherNode->getUUID()) && !getsIgnoredByMe)
                        || (otherNode->isIgnoringNodeWithID(node->getUUID()) && !getsAnyIgnored)) {
                        return false;
                    } else {
                        AvatarMixerClientData* otherData = reinterpret_cast<AvatarMixerClientData*>(otherNode->getLinkedData());
                        AvatarMixerClientData* nodeData = reinterpret_cast<AvatarMixerClientData*>(node->getLinkedData());
                        // Check to see if the space bubble is enabled
                        if ((node->isIgnoreRadiusEnabled() && !getsIgnoredByMe) || (otherNode->isIgnoreRadiusEnabled() && !getsAnyIgnored)) {
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
                                return false;
                            }
                        }
                        // Not close enough to ignore
                        nodeData->removeFromRadiusIgnoringSet(otherNode->getUUID());
                        return true;
                    }
                },
                [&](const SharedNodePointer& otherNode) {
                    ++numOtherAvatars;

                    AvatarMixerClientData* otherNodeData = reinterpret_cast<AvatarMixerClientData*>(otherNode->getLinkedData());
                    MutexTryLocker lock(otherNodeData->getMutex());
                    if (!lock.isLocked()) {
                        return;
                    }

                    // make sure we send out identity packets to and from new arrivals.
                    bool forceSend = !otherNodeData->checkAndSetHasReceivedFirstPacketsFrom(node->getUUID());

                    if (otherNodeData->getIdentityChangeTimestamp().time_since_epoch().count() > 0
                        && (forceSend
                            || otherNodeData->getIdentityChangeTimestamp() > _lastFrameTimestamp
                            || distribution(generator) < IDENTITY_SEND_PROBABILITY)) {
                        sendIdentityPacket(otherNodeData, node);
                    }

                    AvatarData& otherAvatar = otherNodeData->getAvatar();
                    //  Decide whether to send this avatar's data based on it's distance from us

                    //  The full rate distance is the distance at which EVERY update will be sent for this avatar
                    //  at twice the full rate distance, there will be a 50% chance of sending this avatar's update
                    glm::vec3 otherPosition = otherAvatar.getClientGlobalPosition();
                    float distanceToAvatar = glm::length(myPosition - otherPosition);

                    // potentially update the max full rate distance for this frame
                    maxAvatarDistanceThisFrame = std::max(maxAvatarDistanceThisFrame, distanceToAvatar);

                    if (distanceToAvatar != 0.0f
                        && !getsOutOfView
                        && distribution(generator) > (nodeData->getFullRateDistance() / distanceToAvatar)) {
                        return;
                    }

                    AvatarDataSequenceNumber lastSeqToReceiver = nodeData->getLastBroadcastSequenceNumber(otherNode->getUUID());
                    AvatarDataSequenceNumber lastSeqFromSender = otherNodeData->getLastReceivedSequenceNumber();
                    
                    if (lastSeqToReceiver > lastSeqFromSender && lastSeqToReceiver != UINT16_MAX) {
                        // we got out out of order packets from the sender, track it
                        otherNodeData->incrementNumOutOfOrderSends();
                    }

                    // make sure we haven't already sent this data from this sender to this receiver
                    // or that somehow we haven't sent
                    if (lastSeqToReceiver == lastSeqFromSender && lastSeqToReceiver != 0) {
                        ++numAvatarsHeldBack;
                        return;
                    } else if (lastSeqFromSender - lastSeqToReceiver > 1) {
                        // this is a skip - we still send the packet but capture the presence of the skip so we see it happening
                        ++numAvatarsWithSkippedFrames;
                    }

                    // we're going to send this avatar

                    // increment the number of avatars sent to this reciever
                    nodeData->incrementNumAvatarsSentLastFrame();

                    // set the last sent sequence number for this sender on the receiver
                    nodeData->setLastBroadcastSequenceNumber(otherNode->getUUID(),
                                                             otherNodeData->getLastReceivedSequenceNumber());

                    // determine if avatar is in view, to determine how much data to include...
                    glm::vec3 otherNodeBoxScale = (otherNodeData->getPosition() - otherNodeData->getGlobalBoundingBoxCorner()) * 2.0f;
                    AABox otherNodeBox(otherNodeData->getGlobalBoundingBoxCorner(), otherNodeBoxScale);
                    bool isInView = nodeData->otherAvatarInView(otherNodeBox);

                    // this throttles the extra data to only be sent every Nth message
                    if (!isInView && getsOutOfView && (lastSeqToReceiver % EXTRA_AVATAR_DATA_FRAME_RATIO > 0)) {
                        return;
                    }

                    // start a new segment in the PacketList for this avatar
                    avatarPacketList->startSegment();

                    AvatarData::AvatarDataDetail detail;
                    if (!isInView && !getsOutOfView) {
                        detail = AvatarData::MinimumData;
                        nodeData->incrementAvatarOutOfView();
                    } else {
                        detail = distribution(generator) < AVATAR_SEND_FULL_UPDATE_RATIO
                                        ? AvatarData::SendAllData : AvatarData::IncludeSmallData;
                        nodeData->incrementAvatarInView();
                    }

                    numAvatarDataBytes += avatarPacketList->write(otherNode->getUUID().toRfc4122());
                    numAvatarDataBytes += avatarPacketList->write(otherAvatar.toByteArray(detail));

                    avatarPacketList->endSegment();
            });

            // close the current packet so that we're always sending something
            avatarPacketList->closeCurrentPacket(true);

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
        }
    );

    // We're done encoding this version of the otherAvatars.  Update their "lastSent" joint-states so
    // that we can notice differences, next time around.
    //
    // FIXME - this seems suspicious, the code seems to consider all avatars, but not all avatars will
    // have had their joints sent, so actually we should consider the time since they actually were sent????
    nodeList->eachMatchingNode(
        [&](const SharedNodePointer& otherNode)->bool {
            if (!otherNode->getLinkedData()) {
                return false;
            }
            if (otherNode->getType() != NodeType::Agent) {
                return false;
            }
            if (!otherNode->getActiveSocket()) {
                return false;
            }
            return true;
        },
        [&](const SharedNodePointer& otherNode) {
            AvatarMixerClientData* otherNodeData = reinterpret_cast<AvatarMixerClientData*>(otherNode->getLinkedData());
            MutexTryLocker lock(otherNodeData->getMutex());
            if (!lock.isLocked()) {
                return;
            }
            AvatarData& otherAvatar = otherNodeData->getAvatar();
            otherAvatar.doneEncoding(false);
        });

    _lastFrameTimestamp = p_high_resolution_clock::now();

#ifdef WANT_DEBUG
    auto sinceLastDebug = p_high_resolution_clock::now() - _lastDebugMessage;
    auto sinceLastDebugUsecs = std::chrono::duration_cast<std::chrono::microseconds>(sinceLastDebug).count();
    quint64 DEBUG_INTERVAL = USECS_PER_SECOND * 5;

    if (sinceLastDebugUsecs > DEBUG_INTERVAL) {
        qDebug() << "broadcast rate:" << _broadcastRate.rate() << "hz";
        _lastDebugMessage = p_high_resolution_clock::now();
    }
#endif

}

void AvatarMixer::nodeKilled(SharedNodePointer killedNode) {
    if (killedNode->getType() == NodeType::Agent
        && killedNode->getLinkedData()) {
        auto nodeList = DependencyManager::get<NodeList>();

        {  // decrement sessionDisplayNames table and possibly remove
           QMutexLocker nodeDataLocker(&killedNode->getLinkedData()->getMutex());
           AvatarMixerClientData* nodeData = dynamic_cast<AvatarMixerClientData*>(killedNode->getLinkedData());
           const QString& baseDisplayName = nodeData->getBaseDisplayName();
           // No sense guarding against very rare case of a node with no entry, as this will work without the guard and do one less lookup in the common case.
           if (--_sessionDisplayNames[baseDisplayName].second <= 0) {
               _sessionDisplayNames.remove(baseDisplayName);
           }
        }

        // this was an avatar we were sending to other people
        // send a kill packet for it to our other nodes
        auto killPacket = NLPacket::create(PacketType::KillAvatar, NUM_BYTES_RFC4122_UUID + sizeof(KillAvatarReason));
        killPacket->write(killedNode->getUUID().toRfc4122());
        killPacket->writePrimitive(KillAvatarReason::AvatarDisconnected);

        nodeList->broadcastToNodes(std::move(killPacket), NodeSet() << NodeType::Agent);

        // we also want to remove sequence number data for this avatar on our other avatars
        // so invoke the appropriate method on the AvatarMixerClientData for other avatars
        nodeList->eachMatchingNode(
            [&](const SharedNodePointer& node)->bool {
                if (!node->getLinkedData()) {
                    return false;
                }

                if (node->getUUID() == killedNode->getUUID()) {
                    return false;
                }

                return true;
            },
            [&](const SharedNodePointer& node) {
                QMetaObject::invokeMethod(node->getLinkedData(),
                                          "removeLastBroadcastSequenceNumber",
                                          Qt::AutoConnection,
                                          Q_ARG(const QUuid&, QUuid(killedNode->getUUID())));
            }
        );
    }
}

void AvatarMixer::handleViewFrustumPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode) {
    auto nodeList = DependencyManager::get<NodeList>();
    nodeList->getOrCreateLinkedData(senderNode);

    if (senderNode->getLinkedData()) {
        AvatarMixerClientData* nodeData = dynamic_cast<AvatarMixerClientData*>(senderNode->getLinkedData());
        if (nodeData != nullptr) {
            nodeData->readViewFrustumPacket(message->getMessage());
        }
    }
}

void AvatarMixer::handleRequestsDomainListDataPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode) {
    auto nodeList = DependencyManager::get<NodeList>();
    nodeList->getOrCreateLinkedData(senderNode);

    if (senderNode->getLinkedData()) {
        AvatarMixerClientData* nodeData = dynamic_cast<AvatarMixerClientData*>(senderNode->getLinkedData());
        if (nodeData != nullptr) {
            bool isRequesting;
            message->readPrimitive(&isRequesting);
            nodeData->setRequestsDomainListData(isRequesting);
        }
    }
}

void AvatarMixer::handleAvatarDataPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode) {
    auto nodeList = DependencyManager::get<NodeList>();
    nodeList->updateNodeWithDataFromPacket(message, senderNode);
}

void AvatarMixer::handleAvatarIdentityPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode) {
    auto nodeList = DependencyManager::get<NodeList>();
    nodeList->getOrCreateLinkedData(senderNode);

    if (senderNode->getLinkedData()) {
        AvatarMixerClientData* nodeData = dynamic_cast<AvatarMixerClientData*>(senderNode->getLinkedData());
        if (nodeData != nullptr) {
            AvatarData& avatar = nodeData->getAvatar();

            // parse the identity packet and update the change timestamp if appropriate
            AvatarData::Identity identity;
            AvatarData::parseAvatarIdentityPacket(message->getMessage(), identity);
            if (avatar.processAvatarIdentity(identity)) {
                QMutexLocker nodeDataLocker(&nodeData->getMutex());
                nodeData->flagIdentityChange();
                nodeData->setReceivedIdentity();
            }
        }
    }
}

void AvatarMixer::handleKillAvatarPacket(QSharedPointer<ReceivedMessage> message) {
    DependencyManager::get<NodeList>()->processKillNode(*message);
}

void AvatarMixer::handleNodeIgnoreRequestPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode) {
    senderNode->parseIgnoreRequestMessage(message);
}

void AvatarMixer::handleRadiusIgnoreRequestPacket(QSharedPointer<ReceivedMessage> packet, SharedNodePointer sendingNode) {
    sendingNode->parseIgnoreRadiusRequestMessage(packet);
}

void AvatarMixer::sendStatsPacket() {
    QJsonObject statsObject;
    statsObject["average_listeners_last_second"] = (float) _sumListeners / (float) _numStatFrames;

    statsObject["average_identity_packets_per_frame"] = (float) _sumIdentityPackets / (float) _numStatFrames;

    statsObject["trailing_sleep_percentage"] = _trailingSleepRatio * 100;
    statsObject["performance_throttling_ratio"] = _performanceThrottlingRatio;
    statsObject["broadcast_loop_rate"] = _broadcastRate.rate();

    QJsonObject avatarsObject;

    auto nodeList = DependencyManager::get<NodeList>();
    // add stats for each listerner
    nodeList->eachNode([&](const SharedNodePointer& node) {
        QJsonObject avatarStats;

        const QString NODE_OUTBOUND_KBPS_STAT_KEY = "outbound_kbps";
        const QString NODE_INBOUND_KBPS_STAT_KEY = "inbound_kbps";

        // add the key to ask the domain-server for a username replacement, if it has it
        avatarStats[USERNAME_UUID_REPLACEMENT_STATS_KEY] = uuidStringWithoutCurlyBraces(node->getUUID());

        avatarStats[NODE_OUTBOUND_KBPS_STAT_KEY] = node->getOutboundBandwidth();
        avatarStats[NODE_INBOUND_KBPS_STAT_KEY] = node->getInboundBandwidth();

        AvatarMixerClientData* clientData = static_cast<AvatarMixerClientData*>(node->getLinkedData());
        if (clientData) {
            MutexTryLocker lock(clientData->getMutex());
            if (lock.isLocked()) {
                clientData->loadJSONStats(avatarStats);

                // add the diff between the full outbound bandwidth and the measured bandwidth for AvatarData send only
                avatarStats["delta_full_vs_avatar_data_kbps"] =
                    avatarStats[NODE_OUTBOUND_KBPS_STAT_KEY].toDouble() - avatarStats[OUTBOUND_AVATAR_DATA_STATS_KEY].toDouble();
            }
        }

        avatarsObject[uuidStringWithoutCurlyBraces(node->getUUID())] = avatarStats;
    });

    statsObject["avatars"] = avatarsObject;
    ThreadedAssignment::addPacketStatsAndSendStatsPacket(statsObject);

    _sumListeners = 0;
    _sumIdentityPackets = 0;
    _numStatFrames = 0;
}

void AvatarMixer::run() {
    qDebug() << "Waiting for connection to domain to request settings from domain-server.";
    
    // wait until we have the domain-server settings, otherwise we bail
    DomainHandler& domainHandler = DependencyManager::get<NodeList>()->getDomainHandler();
    connect(&domainHandler, &DomainHandler::settingsReceived, this, &AvatarMixer::domainSettingsRequestComplete);
    connect(&domainHandler, &DomainHandler::settingsReceiveFail, this, &AvatarMixer::domainSettingsRequestFailed);
    
    ThreadedAssignment::commonInit(AVATAR_MIXER_LOGGING_NAME, NodeType::AvatarMixer);

    // setup the timer that will be fired on the broadcast thread
    _broadcastTimer = new QTimer;
    _broadcastTimer->setTimerType(Qt::PreciseTimer);
    _broadcastTimer->setInterval(AVATAR_DATA_SEND_INTERVAL_MSECS);
    _broadcastTimer->moveToThread(&_broadcastThread);

    // connect appropriate signals and slots
    connect(_broadcastTimer, &QTimer::timeout, this, &AvatarMixer::broadcastAvatarData, Qt::DirectConnection);
    connect(&_broadcastThread, SIGNAL(started()), _broadcastTimer, SLOT(start()));
}

void AvatarMixer::domainSettingsRequestComplete() {
    auto nodeList = DependencyManager::get<NodeList>();
    nodeList->addNodeTypeToInterestSet(NodeType::Agent);
    
    // parse the settings to pull out the values we need
    parseDomainServerSettings(nodeList->getDomainHandler().getSettingsObject());

    float domainMinimumScale = _domainMinimumScale;
    float domainMaximumScale = _domainMaximumScale;

    nodeList->linkedDataCreateCallback = [domainMinimumScale, domainMaximumScale] (Node* node) {
        auto clientData = std::unique_ptr<AvatarMixerClientData> { new AvatarMixerClientData(node->getUUID()) };
        clientData->getAvatar().setDomainMinimumScale(domainMinimumScale);
        clientData->getAvatar().setDomainMaximumScale(domainMaximumScale);

        node->setLinkedData(std::move(clientData));
    };
    
    // start the broadcastThread
    _broadcastThread.start();
}

void AvatarMixer::handlePacketVersionMismatch(PacketType type, const HifiSockAddr& senderSockAddr, const QUuid& senderUUID) {
    // if this client is using packet versions we don't expect.
    if ((type == PacketTypeEnum::Value::AvatarIdentity || type == PacketTypeEnum::Value::AvatarData) && !senderUUID.isNull()) {
        // Echo an empty AvatarData packet back to that client.
        // This should trigger a version mismatch dialog on their side.
        auto nodeList = DependencyManager::get<NodeList>();
        auto node = nodeList->nodeWithUUID(senderUUID);
        if (node) {
            auto emptyPacket = NLPacket::create(PacketType::AvatarData, 0);
            nodeList->sendPacket(std::move(emptyPacket), *node);
        }
    }
}

void AvatarMixer::parseDomainServerSettings(const QJsonObject& domainSettings) {
    const QString AVATAR_MIXER_SETTINGS_KEY = "avatar_mixer";
    const QString NODE_SEND_BANDWIDTH_KEY = "max_node_send_bandwidth";

    const float DEFAULT_NODE_SEND_BANDWIDTH = 5.0f;
    QJsonValue nodeBandwidthValue = domainSettings[AVATAR_MIXER_SETTINGS_KEY].toObject()[NODE_SEND_BANDWIDTH_KEY];
    if (!nodeBandwidthValue.isDouble()) {
        qDebug() << NODE_SEND_BANDWIDTH_KEY << "is not a double - will continue with default value";
    }

    _maxKbpsPerNode = nodeBandwidthValue.toDouble(DEFAULT_NODE_SEND_BANDWIDTH) * KILO_PER_MEGA;
    qDebug() << "The maximum send bandwidth per node is" << _maxKbpsPerNode << "kbps.";

    const QString AVATARS_SETTINGS_KEY = "avatars";

    static const QString MIN_SCALE_OPTION = "min_avatar_scale";
    float settingMinScale = domainSettings[AVATARS_SETTINGS_KEY].toObject()[MIN_SCALE_OPTION].toDouble(MIN_AVATAR_SCALE);
    _domainMinimumScale = glm::clamp(settingMinScale, MIN_AVATAR_SCALE, MAX_AVATAR_SCALE);

    static const QString MAX_SCALE_OPTION = "max_avatar_scale";
    float settingMaxScale = domainSettings[AVATARS_SETTINGS_KEY].toObject()[MAX_SCALE_OPTION].toDouble(MAX_AVATAR_SCALE);
    _domainMaximumScale = glm::clamp(settingMaxScale, MIN_AVATAR_SCALE, MAX_AVATAR_SCALE);

    // make sure that the domain owner didn't flip min and max
    if (_domainMinimumScale > _domainMaximumScale) {
        std::swap(_domainMinimumScale, _domainMaximumScale);
    }

    qDebug() << "This domain requires a minimum avatar scale of" << _domainMinimumScale
        << "and a maximum avatar scale of" << _domainMaximumScale;
}
