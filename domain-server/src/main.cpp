//
//  main.cpp
//  Domain Server 
//
//  Created by Philip Rosedale on 11/20/12.
//  Copyright (c) 2012 High Fidelity, Inc. All rights reserved.
//
//  The Domain Server keeps a list of nodes that have connected to it, and echoes that list of
//  nodes out to nodes when they check in.
//
//  The connection is stateless... the domain server will set you inactive if it does not hear from
//  you in LOGOFF_CHECK_INTERVAL milliseconds, meaning your info will not be sent to other users.
//
//  Each packet from an node has as first character the type of server:
//
//  I - Interactive Node
//  M - Audio Mixer
//

#include <fcntl.h>
#include <map>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "NodeList.h"
#include "NodeTypes.h"
#include "Logstash.h"
#include "PacketHeaders.h"
#include "SharedUtil.h"

const int DOMAIN_LISTEN_PORT = 40102;
unsigned char packetData[MAX_PACKET_SIZE];

const int NODE_COUNT_STAT_INTERVAL_MSECS = 5000;

unsigned char* addNodeToBroadcastPacket(unsigned char* currentPosition, Node* nodeToAdd) {
    *currentPosition++ = nodeToAdd->getType();
    
    currentPosition += packNodeId(currentPosition, nodeToAdd->getNodeID());
    currentPosition += packSocket(currentPosition, nodeToAdd->getPublicSocket());
    currentPosition += packSocket(currentPosition, nodeToAdd->getLocalSocket());
    
    // return the new unsigned char * for broadcast packet
    return currentPosition;
}

int main(int argc, const char * argv[])
{
    NodeList* nodeList = NodeList::createInstance(NODE_TYPE_DOMAIN, DOMAIN_LISTEN_PORT);
	// If user asks to run in "local" mode then we do NOT replace the IP
	// with the EC2 IP. Otherwise, we will replace the IP like we used to
	// this allows developers to run a local domain without recompiling the
	// domain server
	bool isLocalMode = cmdOptionExists(argc, argv, "--local");
	if (isLocalMode) {
		printf("NOTE: Running in Local Mode!\n");
	} else {
		printf("--------------------------------------------------\n");
		printf("NOTE: Running in EC2 Mode. \n");
		printf("If you're a developer testing a local system, you\n");
		printf("probably want to include --local on command line.\n");
		printf("--------------------------------------------------\n");
	}

    setvbuf(stdout, NULL, _IOLBF, 0);
    
    ssize_t receivedBytes = 0;
    char nodeType = '\0';
    
    unsigned char broadcastPacket[MAX_PACKET_SIZE];
    int numHeaderBytes = populateTypeAndVersion(broadcastPacket, PACKET_TYPE_DOMAIN);
    
    unsigned char* currentBufferPos;
    unsigned char* startPointer;
    
    sockaddr_in nodePublicAddress, nodeLocalAddress;
    nodeLocalAddress.sin_family = AF_INET;
    
    in_addr_t serverLocalAddress = getLocalAddress();
    
    nodeList->startSilentNodeRemovalThread();
    
    timeval lastStatSendTime = {};
    
    while (true) {
        if (nodeList->getNodeSocket()->receive((sockaddr *)&nodePublicAddress, packetData, &receivedBytes) &&
            (packetData[0] == PACKET_TYPE_DOMAIN_REPORT_FOR_DUTY || packetData[0] == PACKET_TYPE_DOMAIN_LIST_REQUEST) &&
            packetVersionMatch(packetData)) {
            // this is an RFD or domain list request packet, and there is a version match
            std::map<char, Node *> newestSoloNodes;
            
            int numBytesSenderHeader = numBytesForPacketHeader(packetData);
            
            nodeType = *(packetData + numBytesSenderHeader);
            int numBytesSocket = unpackSocket(packetData + numBytesSenderHeader + sizeof(NODE_TYPE),
                                              (sockaddr*) &nodeLocalAddress);
            
            sockaddr* destinationSocket = (sockaddr*) &nodePublicAddress;
            
            // check the node public address
            // if it matches our local address we're on the same box
            // so hardcode the EC2 public address for now
            if (nodePublicAddress.sin_addr.s_addr == serverLocalAddress) {
            	// If we're not running "local" then we do replace the IP
            	// with the EC2 IP. Otherwise, we use our normal public IP
            	if (!isLocalMode) {
	                nodePublicAddress.sin_addr.s_addr = 895283510; // local IP in this format...
                    destinationSocket = (sockaddr*) &nodeLocalAddress;
	            }
            }
            
            Node* newNode = nodeList->addOrUpdateNode((sockaddr*) &nodePublicAddress,
                                                      (sockaddr*) &nodeLocalAddress,
                                                      nodeType,
                                                      nodeList->getLastNodeID());
            
            if (newNode->getNodeID() == nodeList->getLastNodeID()) {
                nodeList->increaseNodeID();
            }
            
            currentBufferPos = broadcastPacket + numHeaderBytes;
            startPointer = currentBufferPos;
            
            unsigned char* nodeTypesOfInterest = packetData + numBytesSenderHeader + sizeof(NODE_TYPE)
                + numBytesSocket + sizeof(unsigned char);
            int numInterestTypes = *(nodeTypesOfInterest - 1);
            
            if (numInterestTypes > 0) {
                // if the node has sent no types of interest, assume they want nothing but their own ID back
                for (NodeList::iterator node = nodeList->begin(); node != nodeList->end(); node++) {
                    if (!node->matches((sockaddr*) &nodePublicAddress, (sockaddr*) &nodeLocalAddress, nodeType) &&
                            memchr(nodeTypesOfInterest, node->getType(), numInterestTypes)) {
                        // this is not the node themselves
                        // and this is an node of a type in the passed node types of interest
                        // or the node did not pass us any specific types they are interested in
                    
                        if (memchr(SOLO_NODE_TYPES, node->getType(), sizeof(SOLO_NODE_TYPES)) == NULL) {
                            // this is an node of which there can be multiple, just add them to the packet
                            // don't send avatar nodes to other avatars, that will come from avatar mixer
                            if (nodeType != NODE_TYPE_AGENT || node->getType() != NODE_TYPE_AGENT) {
                                currentBufferPos = addNodeToBroadcastPacket(currentBufferPos, &(*node));
                            }
                        
                        } else {
                            // solo node, we need to only send newest
                            if (newestSoloNodes[node->getType()] == NULL ||
                                newestSoloNodes[node->getType()]->getWakeMicrostamp() < node->getWakeMicrostamp()) {
                                // we have to set the newer solo node to add it to the broadcast later
                                newestSoloNodes[node->getType()] = &(*node);
                            }
                        }
                    }
                }
                
                for (std::map<char, Node *>::iterator soloNode = newestSoloNodes.begin();
                     soloNode != newestSoloNodes.end();
                     soloNode++) {
                    // this is the newest alive solo node, add them to the packet
                    currentBufferPos = addNodeToBroadcastPacket(currentBufferPos, soloNode->second);
                }
            }
                        
            // update last receive to now
            uint64_t timeNow = usecTimestampNow();
            newNode->setLastHeardMicrostamp(timeNow);
            
            if (packetData[0] == PACKET_TYPE_DOMAIN_REPORT_FOR_DUTY
                && memchr(SOLO_NODE_TYPES, nodeType, sizeof(SOLO_NODE_TYPES))) {
                newNode->setWakeMicrostamp(timeNow);
            }
            
            // add the node ID to the end of the pointer
            currentBufferPos += packNodeId(currentBufferPos, newNode->getNodeID());
            
            // send the constructed list back to this node
            nodeList->getNodeSocket()->send(destinationSocket,
                                            broadcastPacket,
                                            (currentBufferPos - startPointer) + numHeaderBytes);
        }
        
        if (Logstash::shouldSendStats()) {
            if (usecTimestampNow() - usecTimestamp(&lastStatSendTime) >= (NODE_COUNT_STAT_INTERVAL_MSECS * 1000)) {
                // time to send our count of nodes and servers to logstash
                const char NODE_COUNT_LOGSTASH_KEY[] = "ds-node-count";
                
                Logstash::stashValue(STAT_TYPE_TIMER, NODE_COUNT_LOGSTASH_KEY, nodeList->getNumAliveNodes());
                
                gettimeofday(&lastStatSendTime, NULL);
            }
        }
    }

    return 0;
}

