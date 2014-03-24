//
//  AvatarMixer.cpp
//  hifi
//
//  Created by Stephen Birarda on 9/5/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//
//  Original avatar-mixer main created by Leonardo Murillo on 03/25/13.
//
//  The avatar mixer receives head, hand and positional data from all connected
//  nodes, and broadcasts that data back to them, every BROADCAST_INTERVAL ms.

#include <QtCore/QCoreApplication>
#include <QtCore/QElapsedTimer>
#include <QtCore/QJsonObject>
#include <QtCore/QTimer>

#include <Logging.h>
#include <NodeList.h>
#include <PacketHeaders.h>
#include <SharedUtil.h>
#include <UUID.h>

#include "AvatarMixerClientData.h"

#include "AvatarMixer.h"

const QString AVATAR_MIXER_LOGGING_NAME = "avatar-mixer";

const unsigned int AVATAR_DATA_SEND_INTERVAL_USECS = (1 / 60.0) * 1000 * 1000;

AvatarMixer::AvatarMixer(const QByteArray& packet) :
    ThreadedAssignment(packet),
    _trailingSleepRatio(1.0f),
    _performanceThrottlingRatio(0.0f),
    _sumListeners(0),
    _numStatFrames(0)
{
    // make sure we hear about node kills so we can tell the other nodes
    connect(NodeList::getInstance(), &NodeList::nodeKilled, this, &AvatarMixer::nodeKilled);
}

void attachAvatarDataToNode(Node* newNode) {
    if (!newNode->getLinkedData()) {
        newNode->setLinkedData(new AvatarMixerClientData());
    }
}

// NOTE: some additional optimizations to consider.
//    1) use the view frustum to cull those avatars that are out of view. Since avatar data doesn't need to be present
//       if the avatar is not in view or in the keyhole.
//    2) after culling for view frustum, sort order the avatars by distance, send the closest ones first.
//    3) if we need to rate limit the amount of data we send, we can use a distance weighted "semi-random" function to
//       determine which avatars are included in the packet stream
//    4) we should optimize the avatar data format to be more compact (100 bytes is pretty wasteful).
void AvatarMixer::broadcastAvatarData() {
    static QByteArray mixedAvatarByteArray;
    
    int numPacketHeaderBytes = populatePacketHeader(mixedAvatarByteArray, PacketTypeBulkAvatarData);
    
    NodeList* nodeList = NodeList::getInstance();
    
    foreach (const SharedNodePointer& node, nodeList->getNodeHash()) {
        if (node->getLinkedData() && node->getType() == NodeType::Agent && node->getActiveSocket()) {
            ++_sumListeners;
            
            // reset packet pointers for this node
            mixedAvatarByteArray.resize(numPacketHeaderBytes);
            
            AvatarMixerClientData* myData = reinterpret_cast<AvatarMixerClientData*>(node->getLinkedData());
            AvatarData& avatar = myData->getAvatar();
            glm::vec3 myPosition = avatar.getPosition();
            
            // this is an AGENT we have received head data from
            // send back a packet with other active node data to this node
            foreach (const SharedNodePointer& otherNode, nodeList->getNodeHash()) {
                if (otherNode->getLinkedData() && otherNode->getUUID() != node->getUUID()) {
                    
                    AvatarMixerClientData* otherNodeData = reinterpret_cast<AvatarMixerClientData*>(otherNode->getLinkedData());
                    AvatarData& otherAvatar = otherNodeData->getAvatar();
                    glm::vec3 otherPosition = otherAvatar.getPosition();
                    float distanceToAvatar = glm::length(myPosition - otherPosition);
                    //  The full rate distance is the distance at which EVERY update will be sent for this avatar
                    //  at a distance of twice the full rate distance, there will be a 50% chance of sending this avatar's update
                    const float FULL_RATE_DISTANCE = 2.f;
                    //  Decide whether to send this avatar's data based on it's distance from us
                    if ((distanceToAvatar == 0.f) || (randFloat() < FULL_RATE_DISTANCE / distanceToAvatar)
                            * (1 - _performanceThrottlingRatio)) {
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
                    }
                }
            }
            
            nodeList->writeDatagram(mixedAvatarByteArray, node);
        }
    }
}

void broadcastIdentityPacket() {
   
    NodeList* nodeList = NodeList::getInstance();
    
    QByteArray avatarIdentityPacket = byteArrayWithPopulatedHeader(PacketTypeAvatarIdentity);
    int numPacketHeaderBytes = avatarIdentityPacket.size();
    
    foreach (const SharedNodePointer& node, nodeList->getNodeHash()) {
        if (node->getLinkedData() && node->getType() == NodeType::Agent) {
            
            AvatarMixerClientData* nodeData = reinterpret_cast<AvatarMixerClientData*>(node->getLinkedData());
            AvatarData& avatar = nodeData->getAvatar();
            QByteArray individualData = avatar.identityByteArray();
            individualData.replace(0, NUM_BYTES_RFC4122_UUID, node->getUUID().toRfc4122());
            
            if (avatarIdentityPacket.size() + individualData.size() > MAX_PACKET_SIZE) {
                // we've hit MTU, send out the current packet before appending
                nodeList->broadcastToNodes(avatarIdentityPacket, NodeSet() << NodeType::Agent);
                avatarIdentityPacket.resize(numPacketHeaderBytes);
            }
            
            // append the individual data to the current the avatarIdentityPacket
            avatarIdentityPacket.append(individualData);
            
            // re-set the bool in AvatarMixerClientData so a change between key frames gets sent out
            nodeData->setHasSentIdentityBetweenKeyFrames(false);
        }
    }
    
    // send out the final packet
    if (avatarIdentityPacket.size() > numPacketHeaderBytes) {
        nodeList->broadcastToNodes(avatarIdentityPacket, NodeSet() << NodeType::Agent);
    }
}

void broadcastBillboardPacket(const SharedNodePointer& sendingNode) {
    AvatarMixerClientData* nodeData = static_cast<AvatarMixerClientData*>(sendingNode->getLinkedData());
    AvatarData& avatar = nodeData->getAvatar();
    QByteArray packet = byteArrayWithPopulatedHeader(PacketTypeAvatarBillboard);
    packet.append(sendingNode->getUUID().toRfc4122());
    packet.append(avatar.getBillboard());
    
    NodeList* nodeList = NodeList::getInstance();
    foreach (const SharedNodePointer& node, nodeList->getNodeHash()) {
        if (node->getType() == NodeType::Agent && node != sendingNode) {       
            nodeList->writeDatagram(packet, node);
        }
    }
}

void broadcastBillboardPackets() {
    foreach (const SharedNodePointer& node, NodeList::getInstance()->getNodeHash()) {
        if (node->getLinkedData() && node->getType() == NodeType::Agent) {       
            AvatarMixerClientData* nodeData = static_cast<AvatarMixerClientData*>(node->getLinkedData());
            broadcastBillboardPacket(node);
            nodeData->setHasSentBillboardBetweenKeyFrames(false);
        }
    }
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
                        if (avatar.hasIdentityChangedAfterParsing(receivedPacket)
                            && !nodeData->hasSentIdentityBetweenKeyFrames()) {
                            // this avatar changed their identity in some way and we haven't sent a packet in this keyframe
                            QByteArray identityPacket = byteArrayWithPopulatedHeader(PacketTypeAvatarIdentity);
                            
                            QByteArray individualByteArray = avatar.identityByteArray();
                            individualByteArray.replace(0, NUM_BYTES_RFC4122_UUID, avatarNode->getUUID().toRfc4122());
                            
                            identityPacket.append(individualByteArray);
                            
                            nodeData->setHasSentIdentityBetweenKeyFrames(true);
                            nodeList->broadcastToNodes(identityPacket, NodeSet() << NodeType::Agent);
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
                        if (avatar.hasBillboardChangedAfterParsing(receivedPacket)
                                && !nodeData->hasSentBillboardBetweenKeyFrames()) {
                            // this avatar changed their billboard and we haven't sent a packet in this keyframe
                            broadcastBillboardPacket(avatarNode);
                            nodeData->setHasSentBillboardBetweenKeyFrames(true);
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
    
    statsObject["trailing_sleep_percentage"] = _trailingSleepRatio * 100;
    statsObject["performance_throttling_ratio"] = _performanceThrottlingRatio;
    
    ThreadedAssignment::addPacketStatsAndSendStatsPacket(statsObject);
    
    _sumListeners = 0;
    _numStatFrames = 0;
}

const qint64 AVATAR_IDENTITY_KEYFRAME_MSECS = 5000;
const qint64 AVATAR_BILLBOARD_KEYFRAME_MSECS = 5000;

void AvatarMixer::run() {
    ThreadedAssignment::commonInit(AVATAR_MIXER_LOGGING_NAME, NodeType::AvatarMixer);
    
    NodeList* nodeList = NodeList::getInstance();
    nodeList->addNodeTypeToInterestSet(NodeType::Agent);
    
    nodeList->linkedDataCreateCallback = attachAvatarDataToNode;
    
    int nextFrame = 0;
    timeval startTime;
    
    gettimeofday(&startTime, NULL);
    
    QElapsedTimer identityTimer;
    identityTimer.start();
    
    QElapsedTimer billboardTimer;
    billboardTimer.start();
    
    int usecToSleep = AVATAR_DATA_SEND_INTERVAL_USECS;
    
    const int TRAILING_AVERAGE_FRAMES = 100;
    int framesSinceCutoffEvent = TRAILING_AVERAGE_FRAMES;
    
    while (!_isFinished) {
        
        ++_numStatFrames;
        
        const float STRUGGLE_TRIGGER_SLEEP_PERCENTAGE_THRESHOLD = 0.10f;
        const float BACK_OFF_TRIGGER_SLEEP_PERCENTAGE_THRESHOLD = 0.20f;
        
        const float RATIO_BACK_OFF = 0.02f;
        
        const float CURRENT_FRAME_RATIO = 1.0f / TRAILING_AVERAGE_FRAMES;
        const float PREVIOUS_FRAMES_RATIO = 1.0f - CURRENT_FRAME_RATIO;
        
        if (usecToSleep < 0) {
            usecToSleep = 0;
        }
        
        _trailingSleepRatio = (PREVIOUS_FRAMES_RATIO * _trailingSleepRatio)
            + (usecToSleep * CURRENT_FRAME_RATIO / (float) AVATAR_DATA_SEND_INTERVAL_USECS);
        
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
        
        broadcastAvatarData();
        
        if (identityTimer.elapsed() >= AVATAR_IDENTITY_KEYFRAME_MSECS) {
            // it's time to broadcast the keyframe identity packets
            broadcastIdentityPacket();
            
            // restart the timer so we do it again in AVATAR_IDENTITY_KEYFRAME_MSECS
            identityTimer.restart();
        }
 
        if (billboardTimer.elapsed() >= AVATAR_BILLBOARD_KEYFRAME_MSECS) {
            broadcastBillboardPackets();
            billboardTimer.restart();
        }
        
        QCoreApplication::processEvents();
        
        if (_isFinished) {
            break;
        }
        
        usecToSleep = usecTimestamp(&startTime) + (++nextFrame * AVATAR_DATA_SEND_INTERVAL_USECS) - usecTimestampNow();
        
        if (usecToSleep > 0) {
            usleep(usecToSleep);
        } else {
            qDebug() << "AvatarMixer loop took too" << -usecToSleep << "of extra time. Won't sleep.";
        }
    }
}
