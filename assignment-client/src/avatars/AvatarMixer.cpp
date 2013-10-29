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

#include <Logging.h>
#include <NodeList.h>
#include <PacketHeaders.h>
#include <SharedUtil.h>
#include <UUID.h>

#include "AvatarData.h"

#include "AvatarMixer.h"

const char AVATAR_MIXER_LOGGING_NAME[] = "avatar-mixer";

unsigned char* addNodeToBroadcastPacket(unsigned char *currentPosition, Node *nodeToAdd) {
    QByteArray rfcUUID = nodeToAdd->getUUID().toRfc4122();
    memcpy(currentPosition, rfcUUID.constData(), rfcUUID.size());
    currentPosition += rfcUUID.size();
    
    AvatarData *nodeData = (AvatarData *)nodeToAdd->getLinkedData();
    currentPosition += nodeData->getBroadcastData(currentPosition);
    
    return currentPosition;
}

void attachAvatarDataToNode(Node* newNode) {
    if (newNode->getLinkedData() == NULL) {
        newNode->setLinkedData(new AvatarData(newNode));
    }
}

// NOTE: some additional optimizations to consider.
//    1) use the view frustum to cull those avatars that are out of view. Since avatar data doesn't need to be present
//       if the avatar is not in view or in the keyhole.
//    2) after culling for view frustum, sort order the avatars by distance, send the closest ones first.
//    3) if we need to rate limit the amount of data we send, we can use a distance weighted "semi-random" function to
//       determine which avatars are included in the packet stream
//    4) we should optimize the avatar data format to be more compact (100 bytes is pretty wasteful).
void broadcastAvatarData(NodeList* nodeList, const QUuid& receiverUUID, sockaddr* receiverAddress) {
    static unsigned char broadcastPacketBuffer[MAX_PACKET_SIZE];
    static unsigned char avatarDataBuffer[MAX_PACKET_SIZE];
    unsigned char* broadcastPacket = (unsigned char*)&broadcastPacketBuffer[0];
    int numHeaderBytes = populateTypeAndVersion(broadcastPacket, PACKET_TYPE_BULK_AVATAR_DATA);
    unsigned char* currentBufferPosition = broadcastPacket + numHeaderBytes;
    int packetLength = currentBufferPosition - broadcastPacket;
    int packetsSent = 0;
    
    // send back a packet with other active node data to this node
    for (NodeList::iterator node = nodeList->begin(); node != nodeList->end(); node++) {
        if (node->getLinkedData() && node->getUUID() != receiverUUID) {
            unsigned char* avatarDataEndpoint = addNodeToBroadcastPacket((unsigned char*)&avatarDataBuffer[0], &*node);
            int avatarDataLength = avatarDataEndpoint - (unsigned char*)&avatarDataBuffer;
            
            if (avatarDataLength + packetLength <= MAX_PACKET_SIZE) {
                memcpy(currentBufferPosition, &avatarDataBuffer[0], avatarDataLength);
                packetLength += avatarDataLength;
                currentBufferPosition += avatarDataLength;
            } else {
                packetsSent++;
                //printf("packetsSent=%d packetLength=%d\n", packetsSent, packetLength);
                nodeList->getNodeSocket()->send(receiverAddress, broadcastPacket, currentBufferPosition - broadcastPacket);
                
                // reset the packet
                currentBufferPosition = broadcastPacket + numHeaderBytes;
                packetLength = currentBufferPosition - broadcastPacket;
                
                // copy the avatar that didn't fit into the next packet
                memcpy(currentBufferPosition, &avatarDataBuffer[0], avatarDataLength);
                packetLength += avatarDataLength;
                currentBufferPosition += avatarDataLength;
            }
        }
    }
    packetsSent++;
    //printf("packetsSent=%d packetLength=%d\n", packetsSent, packetLength);
    nodeList->getNodeSocket()->send(receiverAddress, broadcastPacket, currentBufferPosition - broadcastPacket);
}

AvatarMixer::AvatarMixer(const unsigned char* dataBuffer, int numBytes) : Assignment(dataBuffer, numBytes) {
    
}

void AvatarMixer::run() {
    // change the logging target name while AvatarMixer is running
    Logging::setTargetName(AVATAR_MIXER_LOGGING_NAME);
    
    NodeList* nodeList = NodeList::getInstance();
    nodeList->setOwnerType(NODE_TYPE_AVATAR_MIXER);
    
    nodeList->setNodeTypesOfInterest(&NODE_TYPE_AGENT, 1);
    
    nodeList->linkedDataCreateCallback = attachAvatarDataToNode;
    
    nodeList->startSilentNodeRemovalThread();
    
    sockaddr nodeAddress = {};
    ssize_t receivedBytes = 0;
    
    unsigned char packetData[MAX_PACKET_SIZE];
    
    QUuid nodeUUID;
    Node* avatarNode = NULL;
    
    timeval lastDomainServerCheckIn = {};
    
    while (true) {
        
        if (NodeList::getInstance()->getNumNoReplyDomainCheckIns() == MAX_SILENT_DOMAIN_SERVER_CHECK_INS) {
            break;
        }
        
        // send a check in packet to the domain server if DOMAIN_SERVER_CHECK_IN_USECS has elapsed
        if (usecTimestampNow() - usecTimestamp(&lastDomainServerCheckIn) >= DOMAIN_SERVER_CHECK_IN_USECS) {
            gettimeofday(&lastDomainServerCheckIn, NULL);
            NodeList::getInstance()->sendDomainServerCheckIn();
        }
        
        nodeList->possiblyPingInactiveNodes();
        
        if (nodeList->getNodeSocket()->receive(&nodeAddress, packetData, &receivedBytes) &&
            packetVersionMatch(packetData)) {
            switch (packetData[0]) {
                case PACKET_TYPE_HEAD_DATA:
                    nodeUUID = QUuid::fromRfc4122(QByteArray((char*) packetData + numBytesForPacketHeader(packetData),
                                                                   NUM_BYTES_RFC4122_UUID));
                    
                    // add or update the node in our list
                    avatarNode = nodeList->nodeWithUUID(nodeUUID);
                    
                    if (avatarNode) {
                        // parse positional data from an node
                        nodeList->updateNodeWithData(avatarNode, &nodeAddress, packetData, receivedBytes);
                    } else {
                        break;
                    }
                case PACKET_TYPE_INJECT_AUDIO:
                    broadcastAvatarData(nodeList, nodeUUID, &nodeAddress);
                    break;
                case PACKET_TYPE_AVATAR_URLS:
                case PACKET_TYPE_AVATAR_FACE_VIDEO:
                    nodeUUID = QUuid::fromRfc4122(QByteArray((char*) packetData + numBytesForPacketHeader(packetData),
                                                             NUM_BYTES_RFC4122_UUID));
                    // let everyone else know about the update
                    for (NodeList::iterator node = nodeList->begin(); node != nodeList->end(); node++) {
                        if (node->getActiveSocket() && node->getUUID() != nodeUUID) {
                            nodeList->getNodeSocket()->send(node->getActiveSocket(), packetData, receivedBytes);
                        }
                    }
                    break;
                default:
                    // hand this off to the NodeList
                    nodeList->processNodeData(&nodeAddress, packetData, receivedBytes);
                    break;
            }
        }
    }
    
    nodeList->stopSilentNodeRemovalThread();
}
