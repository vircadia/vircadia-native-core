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

#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtCore/QJsonObject>
#include <QtCore/QTimer>
#include <QtCore/QThread>

#include <LogHandler.h>
#include <NodeList.h>
#include <udt/PacketHeaders.h>
#include <SharedUtil.h>
#include <UUID.h>
#include <TryLocker.h>

#include "AvatarMixerClientData.h"
#include "AvatarMixer.h"

const QString AVATAR_MIXER_LOGGING_NAME = "avatar-mixer";

const int AVATAR_MIXER_BROADCAST_FRAMES_PER_SECOND = 60;
const unsigned int AVATAR_DATA_SEND_INTERVAL_MSECS = (1.0f / (float) AVATAR_MIXER_BROADCAST_FRAMES_PER_SECOND) * 1000;

AvatarMixer::AvatarMixer(NLPacket& packet) :
    ThreadedAssignment(packet),
    _broadcastThread(),
    _lastFrameTimestamp(QDateTime::currentMSecsSinceEpoch()),
    _trailingSleepRatio(1.0f),
    _performanceThrottlingRatio(0.0f),
    _sumListeners(0),
    _numStatFrames(0),
    _sumBillboardPackets(0),
    _sumIdentityPackets(0)
{
    // make sure we hear about node kills so we can tell the other nodes
    connect(DependencyManager::get<NodeList>().data(), &NodeList::nodeKilled, this, &AvatarMixer::nodeKilled);

    auto& packetReceiver = DependencyManager::get<NodeList>()->getPacketReceiver();
    packetReceiver.registerListener(PacketType::AvatarData, this, "handleAvatarDataPacket");
    packetReceiver.registerListener(PacketType::AvatarIdentity, this, "handleAvatarIdentityPacket");
    packetReceiver.registerListener(PacketType::AvatarBillboard, this, "handleAvatarBillboardPacket");
    packetReceiver.registerListener(PacketType::KillAvatar, this, "handleKillAvatarPacket");
}

AvatarMixer::~AvatarMixer() {
    if (_broadcastTimer) {
        _broadcastTimer->deleteLater();
    }

    _broadcastThread.quit();
    _broadcastThread.wait();
}

const float BILLBOARD_AND_IDENTITY_SEND_PROBABILITY = 1.0f / 300.0f;

// NOTE: some additional optimizations to consider.
//    1) use the view frustum to cull those avatars that are out of view. Since avatar data doesn't need to be present
//       if the avatar is not in view or in the keyhole.
void AvatarMixer::broadcastAvatarData() {

    int idleTime = QDateTime::currentMSecsSinceEpoch() - _lastFrameTimestamp;

    ++_numStatFrames;

    const float STRUGGLE_TRIGGER_SLEEP_PERCENTAGE_THRESHOLD = 0.10f;
    const float BACK_OFF_TRIGGER_SLEEP_PERCENTAGE_THRESHOLD = 0.20f;

    const float RATIO_BACK_OFF = 0.02f;

    const int TRAILING_AVERAGE_FRAMES = 100;
    int framesSinceCutoffEvent = TRAILING_AVERAGE_FRAMES;

    const float CURRENT_FRAME_RATIO = 1.0f / TRAILING_AVERAGE_FRAMES;
    const float PREVIOUS_FRAMES_RATIO = 1.0f - CURRENT_FRAME_RATIO;

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

            AvatarData& avatar = nodeData->getAvatar();
            glm::vec3 myPosition = avatar.getPosition();

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
            NLPacketList avatarPacketList(PacketType::BulkAvatarData);

            // this is an AGENT we have received head data from
            // send back a packet with other active node data to this node
            nodeList->eachMatchingNode(
                [&](const SharedNodePointer& otherNode)->bool {
                    if (!otherNode->getLinkedData()) {
                        return false;
                    }
                    if (otherNode->getUUID() == node->getUUID()) {
                        return false;
                    }

                    return true;
                },
                [&](const SharedNodePointer& otherNode) {
                    ++numOtherAvatars;

                    AvatarMixerClientData* otherNodeData = reinterpret_cast<AvatarMixerClientData*>(otherNode->getLinkedData());
                    MutexTryLocker lock(otherNodeData->getMutex());
                    if (!lock.isLocked()) {
                        return;
                    }

                    AvatarData& otherAvatar = otherNodeData->getAvatar();
                    //  Decide whether to send this avatar's data based on it's distance from us

                    //  The full rate distance is the distance at which EVERY update will be sent for this avatar
                    //  at twice the full rate distance, there will be a 50% chance of sending this avatar's update
                    glm::vec3 otherPosition = otherAvatar.getPosition();
                    float distanceToAvatar = glm::length(myPosition - otherPosition);

                    // potentially update the max full rate distance for this frame
                    maxAvatarDistanceThisFrame = std::max(maxAvatarDistanceThisFrame, distanceToAvatar);

                    if (distanceToAvatar != 0.0f
                        && distribution(generator) > (nodeData->getFullRateDistance() / distanceToAvatar)) {
                        return;
                    }

                    uint16_t lastSeqToReceiver = nodeData->getLastBroadcastSequenceNumber(otherNode->getUUID());
                    uint16_t lastSeqFromSender = otherNodeData->getLastReceivedSequenceNumber();

                    if (lastSeqToReceiver > lastSeqFromSender) {
                        // Did we somehow get out of order packets from the sender?
                        // We don't expect this to happen - in RELEASE we add this to a trackable stat
                        // and in DEBUG we crash on the assert

                        otherNodeData->incrementNumOutOfOrderSends();

                        assert(false);
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

                    // start a new segment in the PacketList for this avatar
                    avatarPacketList.startSegment();

                    numAvatarDataBytes += avatarPacketList.write(otherNode->getUUID().toRfc4122());
                    numAvatarDataBytes += avatarPacketList.write(otherAvatar.toByteArray());

                    avatarPacketList.endSegment();

                    // if the receiving avatar has just connected make sure we send out the mesh and billboard
                    // for this avatar (assuming they exist)
                    bool forceSend = !nodeData->checkAndSetHasReceivedFirstPackets();

                    // we will also force a send of billboard or identity packet
                    // if either has changed in the last frame

                    if (otherNodeData->getBillboardChangeTimestamp() > 0
                        && (forceSend
                            || otherNodeData->getBillboardChangeTimestamp() > _lastFrameTimestamp
                            || randFloat() < BILLBOARD_AND_IDENTITY_SEND_PROBABILITY)) {

                        QByteArray rfcUUID = otherNode->getUUID().toRfc4122();
                        QByteArray billboard = otherNodeData->getAvatar().getBillboard();

                        auto billboardPacket = NLPacket::create(PacketType::AvatarBillboard, rfcUUID.size() + billboard.size());
                        billboardPacket->write(rfcUUID);
                        billboardPacket->write(billboard);

                        nodeList->sendPacket(std::move(billboardPacket), *node);

                        ++_sumBillboardPackets;
                    }

                    if (otherNodeData->getIdentityChangeTimestamp() > 0
                        && (forceSend
                            || otherNodeData->getIdentityChangeTimestamp() > _lastFrameTimestamp
                            || randFloat() < BILLBOARD_AND_IDENTITY_SEND_PROBABILITY)) {

                        QByteArray individualData = otherNodeData->getAvatar().identityByteArray();

                        auto identityPacket = NLPacket::create(PacketType::AvatarIdentity, individualData.size());

                        individualData.replace(0, NUM_BYTES_RFC4122_UUID, otherNode->getUUID().toRfc4122());

                        identityPacket->write(individualData);

                        nodeList->sendPacket(std::move(identityPacket), *node);

                        ++_sumIdentityPackets;
                    }
            });
            
            // close the current packet so that we're always sending something
            avatarPacketList.closeCurrentPacket(true);

            // send the avatar data PacketList
            nodeList->sendPacketList(avatarPacketList, *node);

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

    _lastFrameTimestamp = QDateTime::currentMSecsSinceEpoch();
}

void AvatarMixer::nodeKilled(SharedNodePointer killedNode) {
    if (killedNode->getType() == NodeType::Agent
        && killedNode->getLinkedData()) {
        auto nodeList = DependencyManager::get<NodeList>();

        // this was an avatar we were sending to other people
        // send a kill packet for it to our other nodes
        auto killPacket = NLPacket::create(PacketType::KillAvatar, NUM_BYTES_RFC4122_UUID);
        killPacket->write(killedNode->getUUID().toRfc4122());

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

void AvatarMixer::handleAvatarDataPacket(QSharedPointer<NLPacket> packet, SharedNodePointer senderNode) {
    auto nodeList = DependencyManager::get<NodeList>();
    nodeList->updateNodeWithDataFromPacket(packet, senderNode);
}

void AvatarMixer::handleAvatarIdentityPacket(QSharedPointer<NLPacket> packet, SharedNodePointer senderNode) {
    if (senderNode->getLinkedData()) {
        AvatarMixerClientData* nodeData = dynamic_cast<AvatarMixerClientData*>(senderNode->getLinkedData());
        if (nodeData != nullptr) {
            AvatarData& avatar = nodeData->getAvatar();

            // parse the identity packet and update the change timestamp if appropriate
            if (avatar.hasIdentityChangedAfterParsing(*packet)) {
                QMutexLocker nodeDataLocker(&nodeData->getMutex());
                nodeData->setIdentityChangeTimestamp(QDateTime::currentMSecsSinceEpoch());
            }
        }
    }
}

void AvatarMixer::handleAvatarBillboardPacket(QSharedPointer<NLPacket> packet, SharedNodePointer senderNode) {
    AvatarMixerClientData* nodeData = dynamic_cast<AvatarMixerClientData*>(senderNode->getLinkedData());
    if (nodeData) {
        AvatarData& avatar = nodeData->getAvatar();

        // parse the billboard packet and update the change timestamp if appropriate
        if (avatar.hasBillboardChangedAfterParsing(*packet)) {
            QMutexLocker nodeDataLocker(&nodeData->getMutex());
            nodeData->setBillboardChangeTimestamp(QDateTime::currentMSecsSinceEpoch());
        }

    }
}

void AvatarMixer::handleKillAvatarPacket(QSharedPointer<NLPacket> packet) {
    DependencyManager::get<NodeList>()->processKillNode(*packet);
}

void AvatarMixer::sendStatsPacket() {
    QJsonObject statsObject;
    statsObject["average_listeners_last_second"] = (float) _sumListeners / (float) _numStatFrames;

    statsObject["average_billboard_packets_per_frame"] = (float) _sumBillboardPackets / (float) _numStatFrames;
    statsObject["average_identity_packets_per_frame"] = (float) _sumIdentityPackets / (float) _numStatFrames;

    statsObject["trailing_sleep_percentage"] = _trailingSleepRatio * 100;
    statsObject["performance_throttling_ratio"] = _performanceThrottlingRatio;

    QJsonObject avatarsObject;

    auto nodeList = DependencyManager::get<NodeList>();
    // add stats for each listerner
    nodeList->eachNode([&](const SharedNodePointer& node) {
        QJsonObject avatarStats;

        const QString NODE_OUTBOUND_KBPS_STAT_KEY = "outbound_kbps";

        // add the key to ask the domain-server for a username replacement, if it has it
        avatarStats[USERNAME_UUID_REPLACEMENT_STATS_KEY] = uuidStringWithoutCurlyBraces(node->getUUID());
        avatarStats[NODE_OUTBOUND_KBPS_STAT_KEY] = node->getOutboundBandwidth();

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
    _sumBillboardPackets = 0;
    _sumIdentityPackets = 0;
    _numStatFrames = 0;
}

void AvatarMixer::run() {
    ThreadedAssignment::commonInit(AVATAR_MIXER_LOGGING_NAME, NodeType::AvatarMixer);

    auto nodeList = DependencyManager::get<NodeList>();
    nodeList->addNodeTypeToInterestSet(NodeType::Agent);

    nodeList->linkedDataCreateCallback = [] (Node* node) {
        node->setLinkedData(new AvatarMixerClientData());
    };

    // setup the timer that will be fired on the broadcast thread
    _broadcastTimer = new QTimer;
    _broadcastTimer->setInterval(AVATAR_DATA_SEND_INTERVAL_MSECS);
    _broadcastTimer->moveToThread(&_broadcastThread);

    // connect appropriate signals and slots
    connect(_broadcastTimer, &QTimer::timeout, this, &AvatarMixer::broadcastAvatarData, Qt::DirectConnection);
    connect(&_broadcastThread, SIGNAL(started()), _broadcastTimer, SLOT(start()));

    // wait until we have the domain-server settings, otherwise we bail
    DomainHandler& domainHandler = nodeList->getDomainHandler();

    qDebug() << "Waiting for domain settings from domain-server.";

    // block until we get the settingsRequestComplete signal
    
    QEventLoop loop;
    connect(&domainHandler, &DomainHandler::settingsReceived, &loop, &QEventLoop::quit);
    connect(&domainHandler, &DomainHandler::settingsReceiveFail, &loop, &QEventLoop::quit);
    domainHandler.requestDomainSettings();
    loop.exec();

    if (domainHandler.getSettingsObject().isEmpty()) {
        qDebug() << "Failed to retreive settings object from domain-server. Bailing on assignment.";
        setFinished(true);
        return;
    }

    // parse the settings to pull out the values we need
    parseDomainServerSettings(domainHandler.getSettingsObject());

    // start the broadcastThread
    _broadcastThread.start();
}

void AvatarMixer::parseDomainServerSettings(const QJsonObject& domainSettings) {
    const QString AVATAR_MIXER_SETTINGS_KEY = "avatar_mixer";
    const QString NODE_SEND_BANDWIDTH_KEY = "max_node_send_bandwidth";

    const float DEFAULT_NODE_SEND_BANDWIDTH = 1.0f;
    QJsonValue nodeBandwidthValue = domainSettings[AVATAR_MIXER_SETTINGS_KEY].toObject()[NODE_SEND_BANDWIDTH_KEY];
    if (!nodeBandwidthValue.isDouble()) {
        qDebug() << NODE_SEND_BANDWIDTH_KEY << "is not a double - will continue with default value";
    }

    _maxKbpsPerNode = nodeBandwidthValue.toDouble(DEFAULT_NODE_SEND_BANDWIDTH) * KILO_PER_MEGA;
    qDebug() << "The maximum send bandwidth per node is" << _maxKbpsPerNode << "kbps.";
}
