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
#include <QtCore/QTimer>

#include <Logging.h>
#include <NodeList.h>
#include <PacketHeaders.h>
#include <SharedUtil.h>
#include <UUID.h>

#include "AvatarData.h"

#include "AvatarMixer.h"

const char AVATAR_MIXER_LOGGING_NAME[] = "avatar-mixer";

const unsigned int AVATAR_DATA_SEND_INTERVAL_USECS = (1 / 60.0) * 1000 * 1000;

AvatarMixer::AvatarMixer(const QByteArray& packet) :
    ThreadedAssignment(packet)
{
    // make sure we hear about node kills so we can tell the other nodes
    connect(NodeList::getInstance(), &NodeList::nodeKilled, this, &AvatarMixer::nodeKilled);
}

void attachAvatarDataToNode(Node* newNode) {
    if (newNode->getLinkedData() == NULL) {
        newNode->setLinkedData(new AvatarData());
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
    
    int packetsSent = 0;
    
    NodeList* nodeList = NodeList::getInstance();
    
    foreach (const SharedNodePointer& node, nodeList->getNodeHash()) {
        if (node->getLinkedData() && node->getType() == NODE_TYPE_AGENT && node->getActiveSocket()) {
            
            // reset packet pointers for this node
            mixedAvatarByteArray.resize(numPacketHeaderBytes);
            
            // this is an AGENT we have received head data from
            // send back a packet with other active node data to this node
            foreach (const SharedNodePointer& otherNode, nodeList->getNodeHash()) {
                if (otherNode->getLinkedData() && otherNode->getUUID() != node->getUUID()) {
                    
                    QByteArray avatarByteArray;
                    avatarByteArray.append(otherNode->getUUID().toRfc4122());
                    
                    
                    AvatarData *nodeData = (AvatarData *)otherNode->getLinkedData();
                    avatarByteArray.append(nodeData->toByteArray());
                    
                    if (avatarByteArray.size() + mixedAvatarByteArray.size() > MAX_PACKET_SIZE) {
                        packetsSent++;
                        //printf("packetsSent=%d packetLength=%d\n", packetsSent, packetLength);
                        nodeList->getNodeSocket().writeDatagram(mixedAvatarByteArray,
                                                                node->getActiveSocket()->getAddress(),
                                                                node->getActiveSocket()->getPort());
                        
                        // reset the packet
                        mixedAvatarByteArray.resize(numPacketHeaderBytes);
                    }
                    
                    // copy the avatar into the mixedAvatarByteArray packet
                    mixedAvatarByteArray.append(avatarByteArray);
                }
            }
            
            packetsSent++;
            //printf("packetsSent=%d packetLength=%d\n", packetsSent, packetLength);
            nodeList->getNodeSocket().writeDatagram(mixedAvatarByteArray,
                                                    node->getActiveSocket()->getAddress(),
                                                    node->getActiveSocket()->getPort());
        }
    }
}

void AvatarMixer::nodeKilled(SharedNodePointer killedNode) {
    if (killedNode->getType() == NODE_TYPE_AGENT
        && killedNode->getLinkedData()) {
        // this was an avatar we were sending to other people
        // send a kill packet for it to our other nodes
        QByteArray killPacket = byteArrayWithPopluatedHeader(PacketTypeKillAvatar);
        killPacket += killedNode->getUUID().toRfc4122();
        
        NodeList::getInstance()->broadcastToNodes(killPacket,
                                                  QSet<NODE_TYPE>() << NODE_TYPE_AGENT);
    }
}

void AvatarMixer::processDatagram(const QByteArray& dataByteArray, const HifiSockAddr& senderSockAddr) {
    
    NodeList* nodeList = NodeList::getInstance();
    
    switch (packetTypeForPacket(dataByteArray)) {
        case PacketTypeAvatarData: {
            QUuid nodeUUID;
            deconstructPacketHeader(dataByteArray, nodeUUID);
            
            // add or update the node in our list
            SharedNodePointer avatarNode = nodeList->nodeWithUUID(nodeUUID);
            
            if (avatarNode) {
                // parse positional data from an node
                nodeList->updateNodeWithData(avatarNode.data(), senderSockAddr, dataByteArray);
                
            }
            break;
        }
        case PacketTypeKillAvatar: {
            nodeList->processKillNode(dataByteArray);
            break;
        }
        default:
            // hand this off to the NodeList
            nodeList->processNodeData(senderSockAddr, dataByteArray);
            break;
    }
    
}

void AvatarMixer::run() {
    commonInit(AVATAR_MIXER_LOGGING_NAME, NODE_TYPE_AVATAR_MIXER);
    
    NodeList* nodeList = NodeList::getInstance();
    nodeList->addNodeTypeToInterestSet(NODE_TYPE_AGENT);
    
    nodeList->linkedDataCreateCallback = attachAvatarDataToNode;
    
    int nextFrame = 0;
    timeval startTime;
    
    gettimeofday(&startTime, NULL);
    
    while (!_isFinished) {
        
        QCoreApplication::processEvents();
        
        if (_isFinished) {
            break;
        }
        
        broadcastAvatarData();
        
        int usecToSleep = usecTimestamp(&startTime) + (++nextFrame * AVATAR_DATA_SEND_INTERVAL_USECS) - usecTimestampNow();
        
        if (usecToSleep > 0) {
            usleep(usecToSleep);
        } else {
            qDebug() << "AvatarMixer loop took too" << -usecToSleep << "of extra time. Won't sleep.";
        }
    }
}
