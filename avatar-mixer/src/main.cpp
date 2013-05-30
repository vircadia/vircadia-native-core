//
//  main.cpp
//  Avatar Mixer
//
//  Created by Leonardo Murillo on 03/25/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved
//
//  The avatar mixer receives head, hand and positional data from all connected
//  agents, and broadcasts that data back to them, every BROADCAST_INTERVAL ms.
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

#include <AgentList.h>
#include <SharedUtil.h>
#include <PacketHeaders.h>
#include <AgentTypes.h>
#include <StdDev.h>
#include <UDPSocket.h>

#include "AvatarData.h"

const int AVATAR_LISTEN_PORT = 55444;

unsigned char *addAgentToBroadcastPacket(unsigned char *currentPosition, Agent *agentToAdd) {
    currentPosition += packAgentId(currentPosition, agentToAdd->getAgentID());

    AvatarData *agentData = (AvatarData *)agentToAdd->getLinkedData();
    currentPosition += agentData->getBroadcastData(currentPosition);
    
    return currentPosition;
}

void attachAvatarDataToAgent(Agent* newAgent) {
    if (newAgent->getLinkedData() == NULL) {
        newAgent->setLinkedData(new AvatarData(newAgent));
    }
}

int main(int argc, const char* argv[]) {
    AgentList* agentList = AgentList::createInstance(AGENT_TYPE_AVATAR_MIXER, AVATAR_LISTEN_PORT);
    setvbuf(stdout, NULL, _IOLBF, 0);
    
    // Handle Local Domain testing with the --local command line
    const char* local = "--local";
    if (cmdOptionExists(argc, argv, local)) {
        printf("Local Domain MODE!\n");
        int ip = getLocalAddress();
        sprintf(DOMAIN_IP,"%d.%d.%d.%d", (ip & 0xFF), ((ip >> 8) & 0xFF),((ip >> 16) & 0xFF), ((ip >> 24) & 0xFF));
    }
    
    agentList->linkedDataCreateCallback = attachAvatarDataToAgent;
    
    agentList->startDomainServerCheckInThread();
    agentList->startSilentAgentRemovalThread();
    
    sockaddr *agentAddress = new sockaddr;
    unsigned char *packetData = new unsigned char[MAX_PACKET_SIZE];
    ssize_t receivedBytes = 0;
    
    unsigned char *broadcastPacket = new unsigned char[MAX_PACKET_SIZE];
    *broadcastPacket = PACKET_HEADER_BULK_AVATAR_DATA;
    
    unsigned char* currentBufferPosition = NULL;
    
    uint16_t agentID = 0;
    Agent* avatarAgent = NULL;
        
    while (true) {
        if (agentList->getAgentSocket()->receive(agentAddress, packetData, &receivedBytes)) {
            switch (packetData[0]) {
                case PACKET_HEADER_HEAD_DATA:
                    // grab the agent ID from the packet
                    unpackAgentId(packetData + 1, &agentID);
                    
                    // add or update the agent in our list
                    avatarAgent = agentList->addOrUpdateAgent(agentAddress, agentAddress, AGENT_TYPE_AVATAR, agentID);
                    
                    // parse positional data from an agent
                    agentList->updateAgentWithData(avatarAgent, packetData, receivedBytes);
                case PACKET_HEADER_INJECT_AUDIO:
                    currentBufferPosition = broadcastPacket + 1;
                    
                    // send back a packet with other active agent data to this agent
                    for (AgentList::iterator agent = agentList->begin(); agent != agentList->end(); agent++) {
                        if (agent->getLinkedData() && !socketMatch(agentAddress, agent->getActiveSocket())) {
                            currentBufferPosition = addAgentToBroadcastPacket(currentBufferPosition, &*agent);
                        }
                    }
                    
                    agentList->getAgentSocket()->send(agentAddress, broadcastPacket, currentBufferPosition - broadcastPacket);
                    
                    break;
                case PACKET_HEADER_DOMAIN:
                    // ignore the DS packet, for now agents are added only when they communicate directly with us
                    break;
                default:
                    // hand this off to the AgentList
                    agentList->processAgentData(agentAddress, packetData, receivedBytes);
                    break;
            }
        }
    }
    
    agentList->stopSilentAgentRemovalThread();
    agentList->stopDomainServerCheckInThread();
    
    return 0;
}
