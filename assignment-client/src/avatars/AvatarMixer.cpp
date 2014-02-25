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
    ThreadedAssignment(packet)
{
    // make sure we hear about node kills so we can tell the other nodes
    connect(NodeList::getInstance(), &NodeList::nodeKilled, this, &AvatarMixer::nodeKilled);
}

void attachAvatarDataToNode(Node* newNode) {
    if (newNode->getLinkedData() == NULL) {
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
void broadcastAvatarData() {
    static QByteArray mixedAvatarByteArray;
    
    int numPacketHeaderBytes = populatePacketHeader(mixedAvatarByteArray, PacketTypeBulkAvatarData);
    
    NodeList* nodeList = NodeList::getInstance();
    
    foreach (const SharedNodePointer& node, nodeList->getNodeHash()) {
        if (node->getLinkedData() && node->getType() == NodeType::Agent && node->getActiveSocket()) {
            
            // reset packet pointers for this node
            mixedAvatarByteArray.resize(numPacketHeaderBytes);
            
            AvatarMixerClientData* myData = reinterpret_cast<AvatarMixerClientData*>(node->getLinkedData());
            glm::vec3 myPosition = myData->getPosition();
            
            // this is an AGENT we have received head data from
            // send back a packet with other active node data to this node
            foreach (const SharedNodePointer& otherNode, nodeList->getNodeHash()) {
                if (otherNode->getLinkedData() && otherNode->getUUID() != node->getUUID()) {
                    
                    AvatarMixerClientData* otherNodeData = reinterpret_cast<AvatarMixerClientData*>(otherNode->getLinkedData());
                    glm::vec3 otherPosition = otherNodeData->getPosition();
                    float distanceToAvatar = glm::length(myPosition - otherPosition);
                    //  The full rate distance is the distance at which EVERY update will be sent for this avatar
                    //  at a distance of twice the full rate distance, there will be a 50% chance of sending this avatar's update
                    const float FULL_RATE_DISTANCE = 2.f;
                    //  Decide whether to send this avatar's data based on it's distance from us
                    if ((distanceToAvatar == 0.f) || (randFloat() < FULL_RATE_DISTANCE / distanceToAvatar)) {
                        QByteArray avatarByteArray;
                        avatarByteArray.append(otherNode->getUUID().toRfc4122());
                        avatarByteArray.append(otherNodeData->toByteArray());
                        
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
            QByteArray individualData = nodeData->identityByteArray();
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
    QByteArray packet = byteArrayWithPopulatedHeader(PacketTypeAvatarBillboard);
    packet.append(sendingNode->getUUID().toRfc4122());
    packet.append(nodeData->getBillboard());
    
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
                        if (nodeData->hasIdentityChangedAfterParsing(receivedPacket)
                            && !nodeData->hasSentIdentityBetweenKeyFrames()) {
                            // this avatar changed their identity in some way and we haven't sent a packet in this keyframe
                            QByteArray identityPacket = byteArrayWithPopulatedHeader(PacketTypeAvatarIdentity);
                            
                            QByteArray individualByteArray = nodeData->identityByteArray();
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
                        if (nodeData->hasBillboardChangedAfterParsing(receivedPacket)
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

const qint64 AVATAR_IDENTITY_KEYFRAME_MSECS = 5000;
const qint64 AVATAR_BILLBOARD_KEYFRAME_MSECS = 5000;

void AvatarMixer::run() {
    commonInit(AVATAR_MIXER_LOGGING_NAME, NodeType::AvatarMixer);
    
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
    
    while (!_isFinished) {
        
        QCoreApplication::processEvents();
        
        if (_isFinished) {
            break;
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
        
        int usecToSleep = usecTimestamp(&startTime) + (++nextFrame * AVATAR_DATA_SEND_INTERVAL_USECS) - usecTimestampNow();
        
        if (usecToSleep > 0) {
            usleep(usecToSleep);
        } else {
            qDebug() << "AvatarMixer loop took too" << -usecToSleep << "of extra time. Won't sleep.";
        }
    }
}
