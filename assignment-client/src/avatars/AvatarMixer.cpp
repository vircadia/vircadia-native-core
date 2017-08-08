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
#include <chrono>
#include <memory>
#include <random>
#include <thread>

#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtCore/QJsonObject>
#include <QtCore/QRegularExpression>
#include <QtCore/QTimer>
#include <QtCore/QThread>

#include <AABox.h>
#include <AvatarLogging.h>
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

AvatarMixer::AvatarMixer(ReceivedMessage& message) :
    ThreadedAssignment(message)
{
    // make sure we hear about node kills so we can tell the other nodes
    connect(DependencyManager::get<NodeList>().data(), &NodeList::nodeKilled, this, &AvatarMixer::nodeKilled);

    auto& packetReceiver = DependencyManager::get<NodeList>()->getPacketReceiver();
    packetReceiver.registerListener(PacketType::AvatarData, this, "queueIncomingPacket");
    packetReceiver.registerListener(PacketType::AdjustAvatarSorting, this, "handleAdjustAvatarSorting");
    packetReceiver.registerListener(PacketType::ViewFrustum, this, "handleViewFrustumPacket");
    packetReceiver.registerListener(PacketType::AvatarIdentity, this, "handleAvatarIdentityPacket");
    packetReceiver.registerListener(PacketType::KillAvatar, this, "handleKillAvatarPacket");
    packetReceiver.registerListener(PacketType::NodeIgnoreRequest, this, "handleNodeIgnoreRequestPacket");
    packetReceiver.registerListener(PacketType::RadiusIgnoreRequest, this, "handleRadiusIgnoreRequestPacket");
    packetReceiver.registerListener(PacketType::RequestsDomainListData, this, "handleRequestsDomainListDataPacket");

    packetReceiver.registerListenerForTypes({
        PacketType::ReplicatedAvatarIdentity,
        PacketType::ReplicatedKillAvatar
    }, this, "handleReplicatedPacket");

    packetReceiver.registerListener(PacketType::ReplicatedBulkAvatarData, this, "handleReplicatedBulkAvatarPacket");

    auto nodeList = DependencyManager::get<NodeList>();
    connect(nodeList.data(), &NodeList::packetVersionMismatch, this, &AvatarMixer::handlePacketVersionMismatch);
    connect(nodeList.data(), &NodeList::nodeAdded, this, [this](const SharedNodePointer& node) {
        if (node->getType() == NodeType::DownstreamAvatarMixer) {
            getOrCreateClientData(node);
        }
    });
}

SharedNodePointer addOrUpdateReplicatedNode(const QUuid& nodeID, const HifiSockAddr& senderSockAddr) {
    auto replicatedNode = DependencyManager::get<NodeList>()->addOrUpdateNode(nodeID, NodeType::Agent,
                                                                              senderSockAddr,
                                                                              senderSockAddr,
                                                                              true, true);

    replicatedNode->setLastHeardMicrostamp(usecTimestampNow());

    return replicatedNode;
}

void AvatarMixer::handleReplicatedPacket(QSharedPointer<ReceivedMessage> message) {
    auto nodeList = DependencyManager::get<NodeList>();
    auto nodeID = QUuid::fromRfc4122(message->peek(NUM_BYTES_RFC4122_UUID));

    SharedNodePointer replicatedNode;

    if (message->getType() == PacketType::ReplicatedKillAvatar) {
        // this is a kill packet, which we should only process if we already have the node in our list
        // since it of course does not make sense to add a node just to remove it an instant later
        replicatedNode = nodeList->nodeWithUUID(nodeID);

        if (!replicatedNode) {
            return;
        }
    } else {
        replicatedNode = addOrUpdateReplicatedNode(nodeID, message->getSenderSockAddr());
    }

    // we better have a node to work with at this point
    assert(replicatedNode);

    if (message->getType() == PacketType::ReplicatedAvatarIdentity) {
        handleAvatarIdentityPacket(message, replicatedNode);
    } else if (message->getType() == PacketType::ReplicatedKillAvatar) {
        handleKillAvatarPacket(message, replicatedNode);
    }
}

void AvatarMixer::handleReplicatedBulkAvatarPacket(QSharedPointer<ReceivedMessage> message) {
    while (message->getBytesLeftToRead()) {
        // first, grab the node ID for this replicated avatar
        auto nodeID = QUuid::fromRfc4122(message->readWithoutCopy(NUM_BYTES_RFC4122_UUID));

        // make sure we have an upstream replicated node that matches
        auto replicatedNode = addOrUpdateReplicatedNode(nodeID, message->getSenderSockAddr());

        // grab the size of the avatar byte array so we know how much to read
        quint16 avatarByteArraySize;
        message->readPrimitive(&avatarByteArraySize);

        // read the avatar byte array
        auto avatarByteArray = message->read(avatarByteArraySize);

        // construct a "fake" avatar data received message from the byte array and packet list information
        auto replicatedMessage = QSharedPointer<ReceivedMessage>::create(avatarByteArray, PacketType::AvatarData,
                                                                         versionForPacketType(PacketType::AvatarData),
                                                                         message->getSenderSockAddr(), nodeID);

        // queue up the replicated avatar data with the client data for the replicated node
        auto start = usecTimestampNow();
        getOrCreateClientData(replicatedNode)->queuePacket(replicatedMessage, replicatedNode);
        auto end = usecTimestampNow();
        _queueIncomingPacketElapsedTime += (end - start);
    }
}

void AvatarMixer::optionallyReplicatePacket(ReceivedMessage& message, const Node& node) {
    // first, make sure that this is a packet from a node we are supposed to replicate
    if (node.isReplicated()) {

        // check if this is a packet type we replicate
        // which means it must be a packet type present in REPLICATED_PACKET_MAPPING or must be the
        // replicated version of one of those packet types
        PacketType replicatedType = PacketTypeEnum::getReplicatedPacketMapping().value(message.getType());

        if (replicatedType == PacketType::Unknown) {
            if (PacketTypeEnum::getReplicatedPacketMapping().key(message.getType()) != PacketType::Unknown) {
                replicatedType = message.getType();
            } else {
                qDebug() << __FUNCTION__ << "called without replicatable packet type - returning";
                return;
            }
        }

        std::unique_ptr<NLPacket> packet;

        auto nodeList = DependencyManager::get<NodeList>();
        nodeList->eachMatchingNode([&](const SharedNodePointer& downstreamNode) {
            return shouldReplicateTo(node, *downstreamNode);
        }, [&](const SharedNodePointer& node) {
            if (!packet) {
                // construct an NLPacket to send to the replicant that has the contents of the received packet
                packet = NLPacket::create(replicatedType, message.getSize());
                packet->write(message.getMessage());
            }

            nodeList->sendUnreliablePacket(*packet, *node);
        });
    }
}

void AvatarMixer::queueIncomingPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer node) {
    auto start = usecTimestampNow();
    getOrCreateClientData(node)->queuePacket(message, node);
    auto end = usecTimestampNow();
    _queueIncomingPacketElapsedTime += (end - start);
}

void AvatarMixer::sendIdentityPacket(AvatarMixerClientData* nodeData, const SharedNodePointer& destinationNode) {
    if (destinationNode->getType() == NodeType::Agent && !destinationNode->isUpstream()) {
        QByteArray individualData = nodeData->getAvatar().identityByteArray();
        individualData.replace(0, NUM_BYTES_RFC4122_UUID, nodeData->getNodeID().toRfc4122());
        auto identityPackets = NLPacketList::create(PacketType::AvatarIdentity, QByteArray(), true, true);
        identityPackets->write(individualData);
        DependencyManager::get<NodeList>()->sendPacketList(std::move(identityPackets), *destinationNode);
        ++_sumIdentityPackets;
    }
}

std::chrono::microseconds AvatarMixer::timeFrame(p_high_resolution_clock::time_point& timestamp) {
    // advance the next frame
    auto nextTimestamp = timestamp + std::chrono::microseconds((int)((float)USECS_PER_SECOND / (float)AVATAR_MIXER_BROADCAST_FRAMES_PER_SECOND));
    auto now = p_high_resolution_clock::now();

    // compute how long the last frame took
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now - timestamp);

    // set the new frame timestamp
    timestamp = std::max(now, nextTimestamp);

    // sleep until the next frame should start
    // WIN32 sleep_until is broken until VS2015 Update 2
    // instead, std::max (above) guarantees that timestamp >= now, so we can sleep_for
    std::this_thread::sleep_for(timestamp - now);

    return duration;
}


void AvatarMixer::start() {

    auto nodeList = DependencyManager::get<NodeList>();

    unsigned int frame = 1;
    auto frameTimestamp = p_high_resolution_clock::now();

    while (!_isFinished) {

        auto frameDuration = timeFrame(frameTimestamp); // calculates last frame duration and sleeps remainder of target amount
        throttle(frameDuration, frame); // determines _throttlingRatio for upcoming mix frame

        int lockWait, nodeTransform, functor;

        // Allow nodes to process any pending/queued packets across our worker threads
        {
            auto start = usecTimestampNow();

            nodeList->nestedEach([&](NodeList::const_iterator cbegin, NodeList::const_iterator cend) {
                auto end = usecTimestampNow();
                _processQueuedAvatarDataPacketsLockWaitElapsedTime += (end - start);

                _slavePool.processIncomingPackets(cbegin, cend);
            }, &lockWait, &nodeTransform, &functor);
            auto end = usecTimestampNow();
            _processQueuedAvatarDataPacketsElapsedTime += (end - start);
        }

        // process pending display names... this doesn't currently run on multiple threads, because it
        // side-effects the mixer's data, which is fine because it's a very low cost operation
        {
            auto start = usecTimestampNow();
            nodeList->nestedEach([&](NodeList::const_iterator cbegin, NodeList::const_iterator cend) {
                std::for_each(cbegin, cend, [&](const SharedNodePointer& node) {
                    if (node->getType() == NodeType::Agent) {
                        manageIdentityData(node);
                    }

                    ++_sumListeners;
                });
            }, &lockWait, &nodeTransform, &functor);
            auto end = usecTimestampNow();
            _displayNameManagementElapsedTime += (end - start);
        }

        // this is where we need to put the real work...
        {
            auto start = usecTimestampNow();
            nodeList->nestedEach([&](NodeList::const_iterator cbegin, NodeList::const_iterator cend) {
                auto start = usecTimestampNow();
                _slavePool.broadcastAvatarData(cbegin, cend, _lastFrameTimestamp, _maxKbpsPerNode, _throttlingRatio);
                auto end = usecTimestampNow();
                _broadcastAvatarDataInner += (end - start);
            }, &lockWait, &nodeTransform, &functor);
            auto end = usecTimestampNow();
            _broadcastAvatarDataElapsedTime += (end - start);

            _broadcastAvatarDataLockWait += lockWait;
            _broadcastAvatarDataNodeTransform += nodeTransform;
            _broadcastAvatarDataNodeFunctor += functor;
        }

        ++frame;
        ++_numTightLoopFrames;
        _loopRate.increment();

        // play nice with qt event-looping
        {
            // since we're a while loop we need to yield to qt's event processing
            auto start = usecTimestampNow();
            QCoreApplication::processEvents();
            if (_isFinished) {
                // alert qt eventing that this is finished
                QCoreApplication::sendPostedEvents(this, QEvent::DeferredDelete);
                break;
            }
            auto end = usecTimestampNow();
            _processEventsElapsedTime += (end - start);
        }

        _lastFrameTimestamp = frameTimestamp;

    }
}


// NOTE: nodeData->getAvatar() might be side effected, must be called when access to node/nodeData
// is guaranteed to not be accessed by other thread
void AvatarMixer::manageIdentityData(const SharedNodePointer& node) {
    AvatarMixerClientData* nodeData = reinterpret_cast<AvatarMixerClientData*>(node->getLinkedData());

    // there is no need to manage identity data we haven't received yet
    // so bail early if we've never received an identity packet for this avatar
    if (!nodeData || !nodeData->getAvatar().hasProcessedFirstIdentity()) {
        return;
    }

    bool sendIdentity = false;
    if (nodeData && nodeData->getAvatarSessionDisplayNameMustChange()) {
        AvatarData& avatar = nodeData->getAvatar();
        const QString& existingBaseDisplayName = nodeData->getBaseDisplayName();
        if (--_sessionDisplayNames[existingBaseDisplayName].second <= 0) {
            _sessionDisplayNames.remove(existingBaseDisplayName);
        }

        QString baseName = avatar.getDisplayName().trimmed();
        const QRegularExpression curses { "fuck|shit|damn|cock|cunt" }; // POC. We may eventually want something much more elaborate (subscription?).
        baseName = baseName.replace(curses, "*"); // Replace rather than remove, so that people have a clue that the person's a jerk.
        const QRegularExpression trailingDigits { "\\s*(_\\d+\\s*)?(\\s*\\n[^$]*)?$" }; // trailing whitespace "_123" and any subsequent lines
        baseName = baseName.remove(trailingDigits);
        if (baseName.isEmpty()) {
            baseName = "anonymous";
        }

        QPair<int, int>& soFar = _sessionDisplayNames[baseName]; // Inserts and answers 0, 0 if not already present, which is what we want.
        int& highWater = soFar.first;
        nodeData->setBaseDisplayName(baseName);
        QString sessionDisplayName = (highWater > 0) ? baseName + "_" + QString::number(highWater) : baseName;
        avatar.setSessionDisplayName(sessionDisplayName);
        highWater++;
        soFar.second++; // refcount
        nodeData->flagIdentityChange();
        nodeData->setAvatarSessionDisplayNameMustChange(false);
        sendIdentity = true;
        qCDebug(avatars) << "Giving session display name" << sessionDisplayName << "to node with ID" << node->getUUID();
    }
    if (nodeData && nodeData->getAvatarSkeletonModelUrlMustChange()) { // never true for an empty _avatarWhitelist
        nodeData->setAvatarSkeletonModelUrlMustChange(false);
        AvatarData& avatar = nodeData->getAvatar();
        static const QUrl emptyURL("");
        QUrl url = avatar.cannonicalSkeletonModelURL(emptyURL);
        if (!isAvatarInWhitelist(url)) {
            qCDebug(avatars) << "Forbidden avatar" << nodeData->getNodeID() << avatar.getSkeletonModelURL() << "replaced with" << (_replacementAvatar.isEmpty() ? "default" : _replacementAvatar);
            avatar.setSkeletonModelURL(_replacementAvatar);
            sendIdentity = true;
        }
    }
    if (sendIdentity && !node->isUpstream()) {
        sendIdentityPacket(nodeData, node); // Tell node whose name changed about its new session display name or avatar.
        // since this packet includes a change to either the skeleton model URL or the display name
        // it needs a new sequence number
        nodeData->getAvatar().pushIdentitySequenceNumber();

        // tell node whose name changed about its new session display name or avatar.
        sendIdentityPacket(nodeData, node);
    }
}

bool AvatarMixer::isAvatarInWhitelist(const QUrl& url) {
    // The avatar is in the whitelist if:
    // 1. The avatar's URL's host matches one of the hosts of the URLs in the whitelist AND
    // 2. The avatar's URL's path starts with the path of that same URL in the whitelist
    for (const auto& whiteListedPrefix : _avatarWhitelist) {
        auto whiteListURL = QUrl::fromUserInput(whiteListedPrefix);
        // check if this script URL matches the whitelist domain and, optionally, is beneath the path
        if (url.host().compare(whiteListURL.host(), Qt::CaseInsensitive) == 0 &&
            url.path().startsWith(whiteListURL.path(), Qt::CaseInsensitive)) {
            return true;
        }
    }

    return false;
}

void AvatarMixer::throttle(std::chrono::microseconds duration, int frame) {
    // throttle using a modified proportional-integral controller
    const float FRAME_TIME = USECS_PER_SECOND / AVATAR_MIXER_BROADCAST_FRAMES_PER_SECOND;
    float mixRatio = duration.count() / FRAME_TIME;

    // constants are determined based on a "regular" 16-CPU EC2 server

    // target different mix and backoff ratios (they also have different backoff rates)
    // this is to prevent oscillation, and encourage throttling to find a steady state
    const float TARGET = 0.9f;
    // on a "regular" machine with 100 avatars, this is the largest value where
    // - overthrottling can be recovered
    // - oscillations will not occur after the recovery
    const float BACKOFF_TARGET = 0.44f;

    // the mixer is known to struggle at about 150 on a "regular" machine
    // so throttle 2/150 the streams to ensure smooth mixing (throttling is linear)
    const float STRUGGLES_AT = 150.0f;
    const float THROTTLE_RATE = 2 / STRUGGLES_AT;
    const float BACKOFF_RATE = THROTTLE_RATE / 4;

    // recovery should be bounded so that large changes in user count is a tolerable experience
    // throttling is linear, so most cases will not need a full recovery
    const int RECOVERY_TIME = 180;

    // weight more recent frames to determine if throttling is necessary,
    const int TRAILING_FRAMES = (int)(100 * RECOVERY_TIME * BACKOFF_RATE);
    const float CURRENT_FRAME_RATIO = 1.0f / TRAILING_FRAMES;
    const float PREVIOUS_FRAMES_RATIO = 1.0f - CURRENT_FRAME_RATIO;
    _trailingMixRatio = PREVIOUS_FRAMES_RATIO * _trailingMixRatio + CURRENT_FRAME_RATIO * mixRatio;

    if (frame % TRAILING_FRAMES == 0) {
        if (_trailingMixRatio > TARGET) {
            int proportionalTerm = 1 + (_trailingMixRatio - TARGET) / 0.1f;
            _throttlingRatio += THROTTLE_RATE * proportionalTerm;
            _throttlingRatio = std::min(_throttlingRatio, 1.0f);
            qDebug("avatar-mixer is struggling (%f mix/sleep) - throttling %f of streams",
                (double)_trailingMixRatio, (double)_throttlingRatio);
        }
        else if (_throttlingRatio > 0.0f && _trailingMixRatio <= BACKOFF_TARGET) {
            int proportionalTerm = 1 + (TARGET - _trailingMixRatio) / 0.2f;
            _throttlingRatio -= BACKOFF_RATE * proportionalTerm;
            _throttlingRatio = std::max(_throttlingRatio, 0.0f);
            qDebug("avatar-mixer is recovering (%f mix/sleep) - throttling %f of streams",
                (double)_trailingMixRatio, (double)_throttlingRatio);
        }
    }
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

        std::unique_ptr<NLPacket> killPacket;
        std::unique_ptr<NLPacket> replicatedKillPacket;

        // this was an avatar we were sending to other people
        // send a kill packet for it to our other nodes
        nodeList->eachMatchingNode([&](const SharedNodePointer& node) {
            // we relay avatar kill packets to agents that are not upstream
            // and downstream avatar mixers, if the node that was just killed was being replicated
            return (node->getType() == NodeType::Agent && !node->isUpstream()) ||
                   (killedNode->isReplicated() && shouldReplicateTo(*killedNode, *node));
        }, [&](const SharedNodePointer& node) {
            if (node->getType() == NodeType::Agent) {
                if (!killPacket) {
                    killPacket = NLPacket::create(PacketType::KillAvatar, NUM_BYTES_RFC4122_UUID + sizeof(KillAvatarReason));
                    killPacket->write(killedNode->getUUID().toRfc4122());
                    killPacket->writePrimitive(KillAvatarReason::AvatarDisconnected);
                }

                nodeList->sendUnreliablePacket(*killPacket, *node);
            } else {
                // send a replicated kill packet to the downstream avatar mixer
                if (!replicatedKillPacket) {
                    replicatedKillPacket = NLPacket::create(PacketType::ReplicatedKillAvatar,
                                                  NUM_BYTES_RFC4122_UUID + sizeof(KillAvatarReason));
                    replicatedKillPacket->write(killedNode->getUUID().toRfc4122());
                    replicatedKillPacket->writePrimitive(KillAvatarReason::AvatarDisconnected);
                }

                nodeList->sendUnreliablePacket(*replicatedKillPacket, *node);
            }
        });


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
                                         "cleanupKilledNode",
                                          Qt::AutoConnection,
                                          Q_ARG(const QUuid&, QUuid(killedNode->getUUID())));
            }
        );
    }
}


void AvatarMixer::handleAdjustAvatarSorting(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode) {
    auto start = usecTimestampNow();

    // only allow admins with kick rights to change this value...
    if (senderNode->getCanKick()) {
        message->readPrimitive(&AvatarData::_avatarSortCoefficientSize);
        message->readPrimitive(&AvatarData::_avatarSortCoefficientCenter);
        message->readPrimitive(&AvatarData::_avatarSortCoefficientAge);

        qCDebug(avatars) << "New avatar sorting... "
                            << "size:" << AvatarData::_avatarSortCoefficientSize
                            << "center:" << AvatarData::_avatarSortCoefficientCenter
                            << "age:" << AvatarData::_avatarSortCoefficientAge;
    }

    auto end = usecTimestampNow();
    _handleAdjustAvatarSortingElapsedTime += (end - start);
}


void AvatarMixer::handleViewFrustumPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode) {
    auto start = usecTimestampNow();
    getOrCreateClientData(senderNode);

    if (senderNode->getLinkedData()) {
        AvatarMixerClientData* nodeData = dynamic_cast<AvatarMixerClientData*>(senderNode->getLinkedData());
        if (nodeData != nullptr) {
            nodeData->readViewFrustumPacket(message->getMessage());
        }
    }

    auto end = usecTimestampNow();
    _handleViewFrustumPacketElapsedTime += (end - start);
}

void AvatarMixer::handleRequestsDomainListDataPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode) {
    auto start = usecTimestampNow();

    getOrCreateClientData(senderNode);

    if (senderNode->getLinkedData()) {
        AvatarMixerClientData* nodeData = dynamic_cast<AvatarMixerClientData*>(senderNode->getLinkedData());
        if (nodeData != nullptr) {
            bool isRequesting;
            message->readPrimitive(&isRequesting);
            nodeData->setRequestsDomainListData(isRequesting);
            qCDebug(avatars) << "node" << nodeData->getNodeID() << "requestsDomainListData" << isRequesting;

            // If we just opened the PAL...
            if (isRequesting) {
                // For each node in the NodeList...
                auto nodeList = DependencyManager::get<NodeList>();
                nodeList->eachMatchingNode(
                    // Discover the valid nodes we're ignoring...
                    [&](const SharedNodePointer& node)->bool {
                    if (node->getUUID() != senderNode->getUUID() &&
                        (nodeData->isRadiusIgnoring(node->getUUID()) ||
                        senderNode->isIgnoringNodeWithID(node->getUUID()))) {
                        return true;
                    }
                    return false;
                },
                    // ...For those nodes, reset the lastBroadcastTime to 0
                    // so that the AvatarMixer will send Identity data to us
                    [&](const SharedNodePointer& node) {
                    nodeData->setLastBroadcastTime(node->getUUID(), 0);
                }
                );
            }
        }
    }
    auto end = usecTimestampNow();
    _handleRequestsDomainListDataPacketElapsedTime += (end - start);
}

void AvatarMixer::handleAvatarIdentityPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode) {
    auto start = usecTimestampNow();
    auto nodeList = DependencyManager::get<NodeList>();
    getOrCreateClientData(senderNode);

    if (senderNode->getLinkedData()) {
        AvatarMixerClientData* nodeData = dynamic_cast<AvatarMixerClientData*>(senderNode->getLinkedData());
        if (nodeData != nullptr) {
            AvatarData& avatar = nodeData->getAvatar();

            // parse the identity packet and update the change timestamp if appropriate
            bool identityChanged = false;
            bool displayNameChanged = false;
            bool skeletonModelUrlChanged = false;
            avatar.processAvatarIdentity(message->getMessage(), identityChanged, displayNameChanged, skeletonModelUrlChanged);

            if (identityChanged) {
                QMutexLocker nodeDataLocker(&nodeData->getMutex());
                nodeData->flagIdentityChange();
                if (displayNameChanged) {
                    nodeData->setAvatarSessionDisplayNameMustChange(true);
                }
                if (skeletonModelUrlChanged && !_avatarWhitelist.isEmpty()) {
                    nodeData->setAvatarSkeletonModelUrlMustChange(true);
                }
            }
        }
    }
    auto end = usecTimestampNow();
    _handleAvatarIdentityPacketElapsedTime += (end - start);
}

void AvatarMixer::handleKillAvatarPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer node) {
    auto start = usecTimestampNow();
    DependencyManager::get<NodeList>()->processKillNode(*message);
    auto end = usecTimestampNow();
    _handleKillAvatarPacketElapsedTime += (end - start);

    optionallyReplicatePacket(*message, *node);
}

void AvatarMixer::handleNodeIgnoreRequestPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode) {
    auto start = usecTimestampNow();
    auto nodeList = DependencyManager::get<NodeList>();
    AvatarMixerClientData* nodeData = reinterpret_cast<AvatarMixerClientData*>(senderNode->getLinkedData());
    bool addToIgnore;
    message->readPrimitive(&addToIgnore);
    while (message->getBytesLeftToRead()) {
        // parse out the UUID being ignored from the packet
        QUuid ignoredUUID = QUuid::fromRfc4122(message->readWithoutCopy(NUM_BYTES_RFC4122_UUID));

        if (nodeList->nodeWithUUID(ignoredUUID)) {
            // Reset the lastBroadcastTime for the ignored avatar to 0
            // so the AvatarMixer knows it'll have to send identity data about the ignored avatar
            // to the ignorer if the ignorer unignores.
            nodeData->setLastBroadcastTime(ignoredUUID, 0);

            // Reset the lastBroadcastTime for the ignorer (FROM THE PERSPECTIVE OF THE IGNORED) to 0
            // so the AvatarMixer knows it'll have to send identity data about the ignorer
            // to the ignored if the ignorer unignores.
            auto ignoredNode = nodeList->nodeWithUUID(ignoredUUID);
            AvatarMixerClientData* ignoredNodeData = reinterpret_cast<AvatarMixerClientData*>(ignoredNode->getLinkedData());
            ignoredNodeData->setLastBroadcastTime(senderNode->getUUID(), 0);
        }

        if (addToIgnore) {
            senderNode->addIgnoredNode(ignoredUUID);
        } else {
            senderNode->removeIgnoredNode(ignoredUUID);
        }
    }
    auto end = usecTimestampNow();
    _handleNodeIgnoreRequestPacketElapsedTime += (end - start);
}

void AvatarMixer::handleRadiusIgnoreRequestPacket(QSharedPointer<ReceivedMessage> packet, SharedNodePointer sendingNode) {
    auto start = usecTimestampNow();
    sendingNode->parseIgnoreRadiusRequestMessage(packet);
    auto end = usecTimestampNow();
    _handleRadiusIgnoreRequestPacketElapsedTime += (end - start);
}

void AvatarMixer::sendStatsPacket() {
    auto start = usecTimestampNow();


    QJsonObject statsObject;

    statsObject["broadcast_loop_rate"] = _loopRate.rate();
    statsObject["threads"] = _slavePool.numThreads();
    statsObject["trailing_mix_ratio"] = _trailingMixRatio;
    statsObject["throttling_ratio"] = _throttlingRatio;

    // this things all occur on the frequency of the tight loop
    int tightLoopFrames = _numTightLoopFrames;
    int tenTimesPerFrame = tightLoopFrames * 10;
    #define TIGHT_LOOP_STAT(x) (x > tenTimesPerFrame) ? x / tightLoopFrames : ((float)x / (float)tightLoopFrames);
    #define TIGHT_LOOP_STAT_UINT64(x) (x > (quint64)tenTimesPerFrame) ? x / tightLoopFrames : ((float)x / (float)tightLoopFrames);

    statsObject["average_listeners_last_second"] = TIGHT_LOOP_STAT(_sumListeners);

    QJsonObject singleCoreTasks;
    singleCoreTasks["processEvents"] = TIGHT_LOOP_STAT_UINT64(_processEventsElapsedTime);
    singleCoreTasks["queueIncomingPacket"] = TIGHT_LOOP_STAT_UINT64(_queueIncomingPacketElapsedTime);

    QJsonObject incomingPacketStats;
    incomingPacketStats["handleAvatarIdentityPacket"] = TIGHT_LOOP_STAT_UINT64(_handleAvatarIdentityPacketElapsedTime);
    incomingPacketStats["handleKillAvatarPacket"] = TIGHT_LOOP_STAT_UINT64(_handleKillAvatarPacketElapsedTime);
    incomingPacketStats["handleNodeIgnoreRequestPacket"] = TIGHT_LOOP_STAT_UINT64(_handleNodeIgnoreRequestPacketElapsedTime);
    incomingPacketStats["handleRadiusIgnoreRequestPacket"] = TIGHT_LOOP_STAT_UINT64(_handleRadiusIgnoreRequestPacketElapsedTime);
    incomingPacketStats["handleRequestsDomainListDataPacket"] = TIGHT_LOOP_STAT_UINT64(_handleRequestsDomainListDataPacketElapsedTime);
    incomingPacketStats["handleViewFrustumPacket"] = TIGHT_LOOP_STAT_UINT64(_handleViewFrustumPacketElapsedTime);

    singleCoreTasks["incoming_packets"] = incomingPacketStats;
    singleCoreTasks["sendStats"] = (float)_sendStatsElapsedTime;

    statsObject["singleCoreTasks"] = singleCoreTasks;

    QJsonObject parallelTasks;

    QJsonObject processQueuedAvatarDataPacketsStats;
    processQueuedAvatarDataPacketsStats["1_total"] = TIGHT_LOOP_STAT_UINT64(_processQueuedAvatarDataPacketsElapsedTime);
    processQueuedAvatarDataPacketsStats["2_lockWait"] = TIGHT_LOOP_STAT_UINT64(_processQueuedAvatarDataPacketsLockWaitElapsedTime);
    parallelTasks["processQueuedAvatarDataPackets"] = processQueuedAvatarDataPacketsStats;

    QJsonObject broadcastAvatarDataStats;

    broadcastAvatarDataStats["1_total"] = TIGHT_LOOP_STAT_UINT64(_broadcastAvatarDataElapsedTime);
    broadcastAvatarDataStats["2_innner"] = TIGHT_LOOP_STAT_UINT64(_broadcastAvatarDataInner);
    broadcastAvatarDataStats["3_lockWait"] = TIGHT_LOOP_STAT_UINT64(_broadcastAvatarDataLockWait);
    broadcastAvatarDataStats["4_NodeTransform"] = TIGHT_LOOP_STAT_UINT64(_broadcastAvatarDataNodeTransform);
    broadcastAvatarDataStats["5_Functor"] = TIGHT_LOOP_STAT_UINT64(_broadcastAvatarDataNodeFunctor);

    parallelTasks["broadcastAvatarData"] = broadcastAvatarDataStats;

    QJsonObject displayNameManagementStats;
    displayNameManagementStats["1_total"] = TIGHT_LOOP_STAT_UINT64(_displayNameManagementElapsedTime);
    parallelTasks["displayNameManagement"] = displayNameManagementStats;

    statsObject["parallelTasks"] = parallelTasks;


    AvatarMixerSlaveStats aggregateStats;
    QJsonObject slavesObject;

    float secondsSinceLastStats = (float)(start - _lastStatsTime) / (float)USECS_PER_SECOND;
    // gather stats
    int slaveNumber = 1;
    _slavePool.each([&](AvatarMixerSlave& slave) {
        QJsonObject slaveObject;
        AvatarMixerSlaveStats stats;
        slave.harvestStats(stats);
        slaveObject["recevied_1_nodesProcessed"] = TIGHT_LOOP_STAT(stats.nodesProcessed);
        slaveObject["received_2_numPacketsReceived"] = TIGHT_LOOP_STAT(stats.packetsProcessed);

        slaveObject["sent_1_nodesBroadcastedTo"] = TIGHT_LOOP_STAT(stats.nodesBroadcastedTo);
        slaveObject["sent_2_numBytesSent"] = TIGHT_LOOP_STAT(stats.numBytesSent);
        slaveObject["sent_3_numPacketsSent"] = TIGHT_LOOP_STAT(stats.numPacketsSent);
        slaveObject["sent_4_numIdentityPackets"] = TIGHT_LOOP_STAT(stats.numIdentityPackets);

        float averageNodes = ((float)stats.nodesBroadcastedTo / (float)tightLoopFrames);
        float averageOutboundAvatarKbps = averageNodes ? ((stats.numBytesSent / secondsSinceLastStats) / BYTES_PER_KILOBIT) / averageNodes : 0.0f;
        slaveObject["sent_5_averageOutboundAvatarKbps"] = averageOutboundAvatarKbps;

        float averageOthersIncluded = averageNodes ? stats.numOthersIncluded / averageNodes : 0.0f;
        slaveObject["sent_6_averageOthersIncluded"] = TIGHT_LOOP_STAT(averageOthersIncluded);

        float averageOverBudgetAvatars = averageNodes ? stats.overBudgetAvatars / averageNodes : 0.0f;
        slaveObject["sent_7_averageOverBudgetAvatars"] = TIGHT_LOOP_STAT(averageOverBudgetAvatars);

        slaveObject["timing_1_processIncomingPackets"] = TIGHT_LOOP_STAT_UINT64(stats.processIncomingPacketsElapsedTime);
        slaveObject["timing_2_ignoreCalculation"] = TIGHT_LOOP_STAT_UINT64(stats.ignoreCalculationElapsedTime);
        slaveObject["timing_3_toByteArray"] = TIGHT_LOOP_STAT_UINT64(stats.toByteArrayElapsedTime);
        slaveObject["timing_4_avatarDataPacking"] = TIGHT_LOOP_STAT_UINT64(stats.avatarDataPackingElapsedTime);
        slaveObject["timing_5_packetSending"] = TIGHT_LOOP_STAT_UINT64(stats.packetSendingElapsedTime);
        slaveObject["timing_6_jobElapsedTime"] = TIGHT_LOOP_STAT_UINT64(stats.jobElapsedTime);

        slavesObject[QString::number(slaveNumber)] = slaveObject;
        slaveNumber++;

        aggregateStats += stats;
    });

    QJsonObject slavesAggregatObject;

    slavesAggregatObject["recevied_1_nodesProcessed"] = TIGHT_LOOP_STAT(aggregateStats.nodesProcessed);
    slavesAggregatObject["received_2_numPacketsReceived"] = TIGHT_LOOP_STAT(aggregateStats.packetsProcessed);

    slavesAggregatObject["sent_1_nodesBroadcastedTo"] = TIGHT_LOOP_STAT(aggregateStats.nodesBroadcastedTo);
    slavesAggregatObject["sent_2_numBytesSent"] = TIGHT_LOOP_STAT(aggregateStats.numBytesSent);
    slavesAggregatObject["sent_3_numPacketsSent"] = TIGHT_LOOP_STAT(aggregateStats.numPacketsSent);
    slavesAggregatObject["sent_4_numIdentityPackets"] = TIGHT_LOOP_STAT(aggregateStats.numIdentityPackets);

    float averageNodes = ((float)aggregateStats.nodesBroadcastedTo / (float)tightLoopFrames);
    float averageOutboundAvatarKbps = averageNodes ? ((aggregateStats.numBytesSent / secondsSinceLastStats) / BYTES_PER_KILOBIT) / averageNodes : 0.0f;
    slavesAggregatObject["sent_5_averageOutboundAvatarKbps"] = averageOutboundAvatarKbps;

    float averageOthersIncluded = averageNodes ? aggregateStats.numOthersIncluded / averageNodes : 0.0f;
    slavesAggregatObject["sent_6_averageOthersIncluded"] = TIGHT_LOOP_STAT(averageOthersIncluded);

    float averageOverBudgetAvatars = averageNodes ? aggregateStats.overBudgetAvatars / averageNodes : 0.0f;
    slavesAggregatObject["sent_7_averageOverBudgetAvatars"] = TIGHT_LOOP_STAT(averageOverBudgetAvatars);

    slavesAggregatObject["timing_1_processIncomingPackets"] = TIGHT_LOOP_STAT_UINT64(aggregateStats.processIncomingPacketsElapsedTime);
    slavesAggregatObject["timing_2_ignoreCalculation"] = TIGHT_LOOP_STAT_UINT64(aggregateStats.ignoreCalculationElapsedTime);
    slavesAggregatObject["timing_3_toByteArray"] = TIGHT_LOOP_STAT_UINT64(aggregateStats.toByteArrayElapsedTime);
    slavesAggregatObject["timing_4_avatarDataPacking"] = TIGHT_LOOP_STAT_UINT64(aggregateStats.avatarDataPackingElapsedTime);
    slavesAggregatObject["timing_5_packetSending"] = TIGHT_LOOP_STAT_UINT64(aggregateStats.packetSendingElapsedTime);
    slavesAggregatObject["timing_6_jobElapsedTime"] = TIGHT_LOOP_STAT_UINT64(aggregateStats.jobElapsedTime);

    statsObject["slaves_aggregate"] = slavesAggregatObject;
    statsObject["slaves_individual"] = slavesObject;

    _handleViewFrustumPacketElapsedTime = 0;
    _handleAvatarIdentityPacketElapsedTime = 0;
    _handleKillAvatarPacketElapsedTime = 0;
    _handleNodeIgnoreRequestPacketElapsedTime = 0;
    _handleRadiusIgnoreRequestPacketElapsedTime = 0;
    _handleRequestsDomainListDataPacketElapsedTime = 0;
    _processEventsElapsedTime = 0;
    _queueIncomingPacketElapsedTime = 0;
    _processQueuedAvatarDataPacketsElapsedTime = 0;
    _processQueuedAvatarDataPacketsLockWaitElapsedTime = 0;

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

    statsObject["z_avatars"] = avatarsObject;

    ThreadedAssignment::addPacketStatsAndSendStatsPacket(statsObject);

    _sumListeners = 0;
    _sumIdentityPackets = 0;
    _numTightLoopFrames = 0;

    _broadcastAvatarDataElapsedTime = 0;
    _broadcastAvatarDataInner = 0;
    _broadcastAvatarDataLockWait = 0;
    _broadcastAvatarDataNodeTransform = 0;
    _broadcastAvatarDataNodeFunctor = 0;

    _displayNameManagementElapsedTime = 0;
    _ignoreCalculationElapsedTime = 0;
    _avatarDataPackingElapsedTime = 0;
    _packetSendingElapsedTime = 0;


    auto end = usecTimestampNow();
    _sendStatsElapsedTime = (end - start);

    _lastStatsTime = start;

}

void AvatarMixer::run() {
    qCDebug(avatars) << "Waiting for connection to domain to request settings from domain-server.";

    // wait until we have the domain-server settings, otherwise we bail
    DomainHandler& domainHandler = DependencyManager::get<NodeList>()->getDomainHandler();
    connect(&domainHandler, &DomainHandler::settingsReceived, this, &AvatarMixer::domainSettingsRequestComplete);
    connect(&domainHandler, &DomainHandler::settingsReceiveFail, this, &AvatarMixer::domainSettingsRequestFailed);

    ThreadedAssignment::commonInit(AVATAR_MIXER_LOGGING_NAME, NodeType::AvatarMixer);
}

AvatarMixerClientData* AvatarMixer::getOrCreateClientData(SharedNodePointer node) {
    auto clientData = dynamic_cast<AvatarMixerClientData*>(node->getLinkedData());

    if (!clientData) {
        node->setLinkedData(std::unique_ptr<NodeData> { new AvatarMixerClientData(node->getUUID()) });
        clientData = dynamic_cast<AvatarMixerClientData*>(node->getLinkedData());
        auto& avatar = clientData->getAvatar();
        avatar.setDomainMinimumScale(_domainMinimumScale);
        avatar.setDomainMaximumScale(_domainMaximumScale);
    }

    return clientData;
}

void AvatarMixer::domainSettingsRequestComplete() {
    auto nodeList = DependencyManager::get<NodeList>();
    nodeList->addSetOfNodeTypesToNodeInterestSet({
        NodeType::Agent, NodeType::EntityScriptServer,
        NodeType::UpstreamAvatarMixer, NodeType::DownstreamAvatarMixer
    });

    // parse the settings to pull out the values we need
    parseDomainServerSettings(nodeList->getDomainHandler().getSettingsObject());

    // start our tight loop...
    start();
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
    QJsonObject avatarMixerGroupObject = domainSettings[AVATAR_MIXER_SETTINGS_KEY].toObject();


    const QString NODE_SEND_BANDWIDTH_KEY = "max_node_send_bandwidth";

    const float DEFAULT_NODE_SEND_BANDWIDTH = 5.0f;
    QJsonValue nodeBandwidthValue = avatarMixerGroupObject[NODE_SEND_BANDWIDTH_KEY];
    if (!nodeBandwidthValue.isDouble()) {
        qCDebug(avatars) << NODE_SEND_BANDWIDTH_KEY << "is not a double - will continue with default value";
    }

    _maxKbpsPerNode = nodeBandwidthValue.toDouble(DEFAULT_NODE_SEND_BANDWIDTH) * KILO_PER_MEGA;
    qCDebug(avatars) << "The maximum send bandwidth per node is" << _maxKbpsPerNode << "kbps.";

    const QString AUTO_THREADS = "auto_threads";
    bool autoThreads = avatarMixerGroupObject[AUTO_THREADS].toBool();
    if (!autoThreads) {
        bool ok;
        const QString NUM_THREADS = "num_threads";
        int numThreads = avatarMixerGroupObject[NUM_THREADS].toString().toInt(&ok);
        if (!ok) {
            qCWarning(avatars) << "Avatar mixer: Error reading thread count. Using 1 thread.";
            numThreads = 1;
        }
        qCDebug(avatars) << "Avatar mixer will use specified number of threads:" << numThreads;
        _slavePool.setNumThreads(numThreads);
    } else {
        qCDebug(avatars) << "Avatar mixer will automatically determine number of threads to use. Using:" << _slavePool.numThreads() << "threads.";
    }

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

    qCDebug(avatars) << "This domain requires a minimum avatar scale of" << _domainMinimumScale
                     << "and a maximum avatar scale of" << _domainMaximumScale;

    const QString AVATAR_WHITELIST_DEFAULT{ "" };
    static const QString AVATAR_WHITELIST_OPTION = "avatar_whitelist";
    _avatarWhitelist = domainSettings[AVATARS_SETTINGS_KEY].toObject()[AVATAR_WHITELIST_OPTION].toString(AVATAR_WHITELIST_DEFAULT).split(',', QString::KeepEmptyParts);

    static const QString REPLACEMENT_AVATAR_OPTION = "replacement_avatar";
    _replacementAvatar = domainSettings[AVATARS_SETTINGS_KEY].toObject()[REPLACEMENT_AVATAR_OPTION].toString(REPLACEMENT_AVATAR_DEFAULT);

    if ((_avatarWhitelist.count() == 1) && _avatarWhitelist[0].isEmpty()) {
        _avatarWhitelist.clear(); // KeepEmptyParts above will parse "," as ["", ""] (which is ok), but "" as [""] (which is not ok).
    }

    if (_avatarWhitelist.isEmpty()) {
        qCDebug(avatars) << "All avatars are allowed.";
    } else {
        qCDebug(avatars) << "Avatars other than" << _avatarWhitelist << "will be replaced by" << (_replacementAvatar.isEmpty() ? "default" : _replacementAvatar);
    }
}
