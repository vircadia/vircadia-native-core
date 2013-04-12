//
//  main.cpp
//  Domain Server 
//
//  Created by Philip Rosedale on 11/20/12.
//  Copyright (c) 2012 High Fidelity, Inc. All rights reserved.
//
//  The Domain Server keeps a list of agents that have connected to it, and echoes that list of
//  agents out to agents when they check in.
//
//  The connection is stateless... the domain server will set you inactive if it does not hear from
//  you in LOGOFF_CHECK_INTERVAL milliseconds, meaning your info will not be sent to other users.
//
//  Each packet from an agent has as first character the type of server:
//
//  I - Interactive Agent
//  M - Audio Mixer
//

#include <iostream>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <map>
#include "AgentList.h"
#include "AgentTypes.h"
#include <PacketHeaders.h>
#include "SharedUtil.h"

#ifdef _WIN32
#include "Syssocket.h"
#include "Systime.h"
#else
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif


const int DOMAIN_LISTEN_PORT = 40102;
unsigned char packetData[MAX_PACKET_SIZE];

const int LOGOFF_CHECK_INTERVAL = 5000;

#define DEBUG_TO_SELF 0

int lastActiveCount = 0;

unsigned char * addAgentToBroadcastPacket(unsigned char *currentPosition, Agent *agentToAdd) {
    *currentPosition++ = agentToAdd->getType();
    
    currentPosition += packAgentId(currentPosition, agentToAdd->getAgentId());
    currentPosition += packSocket(currentPosition, agentToAdd->getPublicSocket());
    currentPosition += packSocket(currentPosition, agentToAdd->getLocalSocket());
    
    // return the new unsigned char * for broadcast packet
    return currentPosition;
}

int main(int argc, const char * argv[])
{
    AgentList *agentList = AgentList::createInstance(AGENT_TYPE_DOMAIN, DOMAIN_LISTEN_PORT);
	// If user asks to run in "local" mode then we do NOT replace the IP
	// with the EC2 IP. Otherwise, we will replace the IP like we used to
	// this allows developers to run a local domain without recompiling the
	// domain server
	bool useLocal = cmdOptionExists(argc, argv, "--local");
	if (useLocal) {
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
    char agentType;
    
    unsigned char *broadcastPacket = new unsigned char[MAX_PACKET_SIZE];
    *broadcastPacket = PACKET_HEADER_DOMAIN;
    
    unsigned char *currentBufferPos;
    unsigned char *startPointer;
    int packetBytesWithoutLeadingChar;
    
    sockaddr_in agentPublicAddress, agentLocalAddress;
    agentLocalAddress.sin_family = AF_INET;
    
    in_addr_t serverLocalAddress = getLocalAddress();
    
    agentList->startSilentAgentRemovalThread();
    
    while (true) {
        if (agentList->getAgentSocket().receive((sockaddr *)&agentPublicAddress, packetData, &receivedBytes)) {
            std::map<char, Agent *> newestSoloAgents;
            
            agentType = packetData[0];
            unpackSocket(&packetData[1], (sockaddr *)&agentLocalAddress);
            
            // check the agent public address
            // if it matches our local address we're on the same box
            // so hardcode the EC2 public address for now
            if (agentPublicAddress.sin_addr.s_addr == serverLocalAddress) {
            	// If we're not running "local" then we do replace the IP
            	// with the EC2 IP. Otherwise, we use our normal public IP
            	if (!useLocal) {
	                agentPublicAddress.sin_addr.s_addr = 895283510; // local IP in this format...
	            }
            }
            
            if (agentList->addOrUpdateAgent((sockaddr *)&agentPublicAddress,
                                           (sockaddr *)&agentLocalAddress,
                                           agentType,
                                           agentList->getLastAgentId())) {
                
                agentList->increaseAgentId();
            
            }
            
            currentBufferPos = broadcastPacket + 1;
            startPointer = currentBufferPos;
            
            for(std::vector<Agent>::iterator agent = agentList->getAgents().begin();
                agent != agentList->getAgents().end();
                agent++) {
                
                if (DEBUG_TO_SELF ||
                    !agent->matches((sockaddr *)&agentPublicAddress, (sockaddr *)&agentLocalAddress, agentType)) {
                    if (strchr(SOLO_AGENT_TYPES_STRING, (int) agent->getType()) == NULL) {
                        // this is an agent of which there can be multiple, just add them to the packet
                        currentBufferPos = addAgentToBroadcastPacket(currentBufferPos, &(*agent));
                    } else {
                        // solo agent, we need to only send newest
                        if (newestSoloAgents[agent->getType()] == NULL ||
                            newestSoloAgents[agent->getType()]->getFirstRecvTimeUsecs() < agent->getFirstRecvTimeUsecs()) {
                            // we have to set the newer solo agent to add it to the broadcast later
                            newestSoloAgents[agent->getType()] = &(*agent);
                        }
                    }
                } else {
                    // this is the agent, just update last receive to now
                    agent->setLastRecvTimeUsecs(usecTimestampNow());
                }
            }
            
            for (std::map<char, Agent *>::iterator agentIterator = newestSoloAgents.begin();
                 agentIterator != newestSoloAgents.end();
                 agentIterator++) {
                // this is the newest alive solo agent, add them to the packet
                currentBufferPos = addAgentToBroadcastPacket(currentBufferPos, agentIterator->second);
            }
            
            if ((packetBytesWithoutLeadingChar = (currentBufferPos - startPointer))) {
                agentList->getAgentSocket().send((sockaddr *)&agentPublicAddress,
                                                broadcastPacket,
                                                packetBytesWithoutLeadingChar + 1);
            }
        }
    }

    return 0;
}

