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

#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtCore/QJsonObject>
#include <QtCore/QTimer>
#include <QtCore/QThread>

#include <LogHandler.h>
#include <NodeList.h>
#include <PacketHeaders.h>
#include <SharedUtil.h>
#include <UUID.h>

#include "AvatarMixerClientData.h"

#include "AvatarMixer.h"

const QString AVATAR_MIXER_LOGGING_NAME = "avatar-mixer";

const unsigned int AVATAR_DATA_SEND_INTERVAL_MSECS = (1.0f / 60.0f) * 1000;

AvatarMixer::AvatarMixer(const QByteArray& packet) :
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
    connect(NodeList::getInstance(), &NodeList::nodeKilled, this, &AvatarMixer::nodeKilled);
}

AvatarMixer::~AvatarMixer() {
    _broadcastThread.quit();
    _broadcastThread.wait();
}

void attachAvatarDataToNode(Node* newNode) {
    if (!newNode->getLinkedData()) {
        newNode->setLinkedData(new AvatarMixerClientData());
    }
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
    
    _trailingSleepRatio = (PREVIOUS_FRAMES_RATIO * _trailingSleepRatio)
        + (idleTime * CURRENT_FRAME_RATIO / (float) AVATAR_DATA_SEND_INTERVAL_MSECS);
    
    float lastCutoffRatio = _performanceThrottlingRatio;
    bool hasRatioChanged = false;
    
    if (framesSinceCutoffEvent >= TRAILING_AVERAGE_FRAMES) {
        if (_trailingSleepRatio <= STRUGGLE_TRIGGER_SLEEP_PERCENTAGE_THRESHOLD) {
            // we're struggling - change our min required loudness to reduce some load
            _performanceThrottlingRatio = _performanceThrottlingRatio + (0.5f * (1.0f - _performanceThrottlingRatio));
            
            qDebug() << "Mixer is struggling, sleeping" << _trailingSleepRatio * 100 << "% of frame time. Old cutoff was"
            << lastCutoffRatio << "and is now" << _performanceThrottlingRatio;
            hasRatioChanged = true;
        } else if (_trailingSleepRatio >= BACK_OFF_TRIGGER_SLEEP_PERCENTAGE_THRESHOLD && _performanceThrottlingRatio != 0) {
            // we've recovered and can back off the required loudness
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
    
    static QByteArray mixedAvatarByteArray;
    
    int numPacketHeaderBytes = populatePacketHeader(mixedAvatarByteArray, PacketTypeBulkAvatarData);
    
    NodeList* nodeList = NodeList::getInstance();
    
    AvatarMixerClientData* nodeData = NULL;
    AvatarMixerClientData* otherNodeData = NULL;
    
    nodeList->eachNode([&](const SharedNodePointer& node) {
        if (node->getLinkedData() && node->getType() == NodeType::Agent && node->getActiveSocket()
            && (nodeData = reinterpret_cast<AvatarMixerClientData*>(node->getLinkedData()))->getMutex().tryLock()) {
            ++_sumListeners;
            
            // reset packet pointers for this node
            mixedAvatarByteArray.resize(numPacketHeaderBytes);
            
            AvatarData& avatar = nodeData->getAvatar();
            glm::vec3 myPosition = avatar.getPosition();
            
            // this is an AGENT we have received head data from
            // send back a packet with other active node data to this node
            nodeList->eachNode([&](const SharedNodePointer& otherNode) {
                if (otherNode->getLinkedData() && otherNode->getUUID() != node->getUUID()
                    && (otherNodeData = reinterpret_cast<AvatarMixerClientData*>(otherNode->getLinkedData()))->getMutex().tryLock()) {
                    
                    AvatarMixerClientData* otherNodeData = reinterpret_cast<AvatarMixerClientData*>(otherNode->getLinkedData());
                    AvatarData& otherAvatar = otherNodeData->getAvatar();
                    glm::vec3 otherPosition = otherAvatar.getPosition();
            
                    float distanceToAvatar = glm::length(myPosition - otherPosition);
                    //  The full rate distance is the distance at which EVERY update will be sent for this avatar
                    //  at a distance of twice the full rate distance, there will be a 50% chance of sending this avatar's update
                    const float FULL_RATE_DISTANCE = 2.f;
                    
                    //  Decide whether to send this avatar's data based on it's distance from us
                    if ((_performanceThrottlingRatio == 0 || randFloat() < (1.0f - _performanceThrottlingRatio))
                        && (distanceToAvatar == 0.f || randFloat() < FULL_RATE_DISTANCE / distanceToAvatar)) {
                        QByteArray avatarByteArray;
                        avatarByteArray.append(otherNode->getUUID().toRfc4122());
                        avatarByteArray.append(otherAvatar.toByteArray());
                        
                        if (avatarByteArray.size() + mixedAvatarByteArray.size() > MAX_PACKET_SIZE) {
                            nodeList->writeDatagram(mixedAvatarByteArray, node);
                            
                            // reset the packet
                            mixedAvatarByteArray.resize(numPacketHeaderBytes);
                        }
                        
                        // copy the avatar into the mixedAvatarByteArray packet
                        mixedAvatarByteArray.append(avatarByteArray);
                        
                        // if the receiving avatar has just connected make sure we send out the mesh and billboard
                        // for this avatar (assuming they exist)
                        bool forceSend = !nodeData->checkAndSetHasReceivedFirstPackets();
                        
                        // we will also force a send of billboard or identity packet
                        // if either has changed in the last frame
                        
                        if (otherNodeData->getBillboardChangeTimestamp() > 0
                            && (forceSend
                                || otherNodeData->getBillboardChangeTimestamp() > _lastFrameTimestamp
                                || randFloat() < BILLBOARD_AND_IDENTITY_SEND_PROBABILITY)) {
                            QByteArray billboardPacket = byteArrayWithPopulatedHeader(PacketTypeAvatarBillboard);
                            billboardPacket.append(otherNode->getUUID().toRfc4122());
                            billboardPacket.append(otherNodeData->getAvatar().getBillboard());
                            nodeList->writeDatagram(billboardPacket, node);
                            
                            ++_sumBillboardPackets;
                        }
                        
                        if (otherNodeData->getIdentityChangeTimestamp() > 0
                            && (forceSend
                                || otherNodeData->getIdentityChangeTimestamp() > _lastFrameTimestamp
                                || randFloat() < BILLBOARD_AND_IDENTITY_SEND_PROBABILITY)) {
                                
                            QByteArray identityPacket = byteArrayWithPopulatedHeader(PacketTypeAvatarIdentity);
                            
                            QByteArray individualData = otherNodeData->getAvatar().identityByteArray();
                            individualData.replace(0, NUM_BYTES_RFC4122_UUID, otherNode->getUUID().toRfc4122());
                            identityPacket.append(individualData);
                            
                            nodeList->writeDatagram(identityPacket, node);
                                
                            ++_sumIdentityPackets;
                        }
                    }
                
                    otherNodeData->getMutex().unlock();
                }
            });
            
            nodeList->writeDatagram(mixedAvatarByteArray, node);
            
            nodeData->getMutex().unlock();
        }
    });
    
    _lastFrameTimestamp = QDateTime::currentMSecsSinceEpoch();
}

void AvatarMixer::nodeKilled(SharedNodePointer killedNode) {
    if (killedNode->getType() == NodeType::Agent
        && killedNode->getLinkedData()) {
        // this was an avatar we were sending to other people
        // send a kill packet for it to our other nodes
        QByteArray killPacket = byteArrayWithPopulatedHeader(PacketTypeKillAvatar);
        killPacket += killedNode->getUUID().toRfc4122();
        
        NodeList::getInstance()->broadcastToNodes(killPacket,
                                                  NodeSet() << NodeType::Agent);
    }
}

void AvatarMixer::readPendingDatagrams() {
    QByteArray receivedPacket;
    HifiSockAddr senderSockAddr;
    
    NodeList* nodeList = NodeList::getInstance();
    
    while (readAvailableDatagram(receivedPacket, senderSockAddr)) {
        if (nodeList->packetVersionAndHashMatch(receivedPacket)) {
            switch (packetTypeForPacket(receivedPacket)) {
                case PacketTypeAvatarData: {
                    nodeList->findNodeAndUpdateWithDataFromPacket(receivedPacket);
                    break;
                }
                case PacketTypeAvatarIdentity: {
                    
                    // check if we have a matching node in our list
                    SharedNodePointer avatarNode = nodeList->sendingNodeForPacket(receivedPacket);
                    
                    if (avatarNode && avatarNode->getLinkedData()) {
                        AvatarMixerClientData* nodeData = reinterpret_cast<AvatarMixerClientData*>(avatarNode->getLinkedData());
                        AvatarData& avatar = nodeData->getAvatar();
                        
                        // parse the identity packet and update the change timestamp if appropriate
                        if (avatar.hasIdentityChangedAfterParsing(receivedPacket)) {
                            QMutexLocker nodeDataLocker(&nodeData->getMutex());
                            nodeData->setIdentityChangeTimestamp(QDateTime::currentMSecsSinceEpoch());
                        }
                    }
                    break;
                }
                case PacketTypeAvatarBillboard: {
                    
                    // check if we have a matching node in our list
                    SharedNodePointer avatarNode = nodeList->sendingNodeForPacket(receivedPacket);
                    
                    if (avatarNode && avatarNode->getLinkedData()) {
                        AvatarMixerClientData* nodeData = static_cast<AvatarMixerClientData*>(avatarNode->getLinkedData());
                        AvatarData& avatar = nodeData->getAvatar();
                        
                        // parse the billboard packet and update the change timestamp if appropriate
                        if (avatar.hasBillboardChangedAfterParsing(receivedPacket)) {
                            QMutexLocker nodeDataLocker(&nodeData->getMutex());
                            nodeData->setBillboardChangeTimestamp(QDateTime::currentMSecsSinceEpoch());
                        }
                        
                    }
                    break;
                }
                case PacketTypeKillAvatar: {
                    nodeList->processKillNode(receivedPacket);
                    break;
                }
                default:
                    // hand this off to the NodeList
                    nodeList->processNodeData(senderSockAddr, receivedPacket);
                    break;
            }
        }
    }
}

void AvatarMixer::sendStatsPacket() {
    QJsonObject statsObject;
    statsObject["average_listeners_last_second"] = (float) _sumListeners / (float) _numStatFrames;
    
    statsObject["average_billboard_packets_per_frame"] = (float) _sumBillboardPackets / (float) _numStatFrames;
    statsObject["average_identity_packets_per_frame"] = (float) _sumIdentityPackets / (float) _numStatFrames;
    
    statsObject["trailing_sleep_percentage"] = _trailingSleepRatio * 100;
    statsObject["performance_throttling_ratio"] = _performanceThrottlingRatio;
    
    ThreadedAssignment::addPacketStatsAndSendStatsPacket(statsObject);
    
    _sumListeners = 0;
    _sumBillboardPackets = 0;
    _sumIdentityPackets = 0;
    _numStatFrames = 0;
}

void AvatarMixer::run() {
    ThreadedAssignment::commonInit(AVATAR_MIXER_LOGGING_NAME, NodeType::AvatarMixer);
    
    NodeList* nodeList = NodeList::getInstance();
    nodeList->addNodeTypeToInterestSet(NodeType::Agent);
    
    nodeList->linkedDataCreateCallback = attachAvatarDataToNode;
    
    // setup the timer that will be fired on the broadcast thread
    QTimer* broadcastTimer = new QTimer();
    broadcastTimer->setInterval(AVATAR_DATA_SEND_INTERVAL_MSECS);
    broadcastTimer->moveToThread(&_broadcastThread);
    
    // connect appropriate signals and slots
    connect(broadcastTimer, &QTimer::timeout, this, &AvatarMixer::broadcastAvatarData, Qt::DirectConnection);
    connect(&_broadcastThread, SIGNAL(started()), broadcastTimer, SLOT(start()));
    
    // start the broadcastThread
    _broadcastThread.start();
}
