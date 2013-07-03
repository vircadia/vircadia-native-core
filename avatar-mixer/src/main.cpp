//
//  main.cpp
//  Avatar Mixer
//
//  Created by Leonardo Murillo on 03/25/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved
//
//  The avatar mixer receives head, hand and positional data from all connected
//  nodes, and broadcasts that data back to them, every BROADCAST_INTERVAL ms.
//
//

#include <iostream>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>
#include <fstream>
#include <limits>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <NodeList.h>
#include <SharedUtil.h>
#include <PacketHeaders.h>
#include <NodeTypes.h>
#include <StdDev.h>
#include <UDPSocket.h>

#include "AvatarData.h"

const int AVATAR_LISTEN_PORT = 55444;

unsigned char *addNodeToBroadcastPacket(unsigned char *currentPosition, Node *nodeToAdd) {
    currentPosition += packNodeId(currentPosition, nodeToAdd->getNodeID());

    AvatarData *nodeData = (AvatarData *)nodeToAdd->getLinkedData();
    currentPosition += nodeData->getBroadcastData(currentPosition);
    
    return currentPosition;
}

void attachAvatarDataToNode(Node* newNode) {
    if (newNode->getLinkedData() == NULL) {
        newNode->setLinkedData(new AvatarData(newNode));
    }
}

int main(int argc, const char* argv[]) {

    NodeList* nodeList = NodeList::createInstance(NODE_TYPE_AVATAR_MIXER, AVATAR_LISTEN_PORT);
    setvbuf(stdout, NULL, _IOLBF, 0);
    
    // Handle Local Domain testing with the --local command line
    const char* local = "--local";
    if (cmdOptionExists(argc, argv, local)) {
        printf("Local Domain MODE!\n");
        int ip = getLocalAddress();
        sprintf(DOMAIN_IP,"%d.%d.%d.%d", (ip & 0xFF), ((ip >> 8) & 0xFF),((ip >> 16) & 0xFF), ((ip >> 24) & 0xFF));
    }
    
    nodeList->linkedDataCreateCallback = attachAvatarDataToNode;
    
    nodeList->startSilentNodeRemovalThread();
    
    sockaddr *nodeAddress = new sockaddr;
    unsigned char *packetData = new unsigned char[MAX_PACKET_SIZE];
    ssize_t receivedBytes = 0;
    
    unsigned char *broadcastPacket = new unsigned char[MAX_PACKET_SIZE];
    *broadcastPacket = PACKET_HEADER_BULK_AVATAR_DATA;
    
    unsigned char* currentBufferPosition = NULL;
    
    uint16_t nodeID = 0;
    Node* avatarNode = NULL;
    
    timeval lastDomainServerCheckIn = {};
    // we only need to hear back about avatar nodes from the DS
    NodeList::getInstance()->setNodeTypesOfInterest(&NODE_TYPE_AGENT, 1);
    
    while (true) {
        
        // send a check in packet to the domain server if DOMAIN_SERVER_CHECK_IN_USECS has elapsed
        if (usecTimestampNow() - usecTimestamp(&lastDomainServerCheckIn) >= DOMAIN_SERVER_CHECK_IN_USECS) {
            gettimeofday(&lastDomainServerCheckIn, NULL);
            NodeList::getInstance()->sendDomainServerCheckIn();
        }
        
        if (nodeList->getNodeSocket()->receive(nodeAddress, packetData, &receivedBytes)) {
            switch (packetData[0]) {
                case PACKET_HEADER_HEAD_DATA:
                    // grab the node ID from the packet
                    unpackNodeId(packetData + 1, &nodeID);
                    
                    // add or update the node in our list
                    avatarNode = nodeList->addOrUpdateNode(nodeAddress, nodeAddress, NODE_TYPE_AGENT, nodeID);
                    
                    // parse positional data from an node
                    nodeList->updateNodeWithData(avatarNode, packetData, receivedBytes);
                case PACKET_HEADER_INJECT_AUDIO:
                    currentBufferPosition = broadcastPacket + 1;
                    
                    // send back a packet with other active node data to this node
                    for (NodeList::iterator node = nodeList->begin(); node != nodeList->end(); node++) {
                        if (node->getLinkedData() && !socketMatch(nodeAddress, node->getActiveSocket())) {
                            currentBufferPosition = addNodeToBroadcastPacket(currentBufferPosition, &*node);
                        }
                    }
                    
                    nodeList->getNodeSocket()->send(nodeAddress, broadcastPacket, currentBufferPosition - broadcastPacket);
                    
                    break;
                case PACKET_HEADER_AVATAR_VOXEL_URL:
                    // grab the node ID from the packet
                    unpackNodeId(packetData + 1, &nodeID);
                    
                    // let everyone else know about the update
                    for (NodeList::iterator node = nodeList->begin(); node != nodeList->end(); node++) {
                        if (node->getActiveSocket() && node->getNodeID() != nodeID) {
                            nodeList->getNodeSocket()->send(node->getActiveSocket(), packetData, receivedBytes);
                        }
                    }
                    break;
                case PACKET_HEADER_DOMAIN:
                    // ignore the DS packet, for now nodes are added only when they communicate directly with us
                    break;
                default:
                    // hand this off to the NodeList
                    nodeList->processNodeData(nodeAddress, packetData, receivedBytes);
                    break;
            }
        }
    }
    
    nodeList->stopSilentNodeRemovalThread();
    
    return 0;
}
