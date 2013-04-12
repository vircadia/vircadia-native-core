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

#include "AvatarAgentData.h"

const int AVATAR_LISTEN_PORT = 55444;
const unsigned short BROADCAST_INTERVAL_USECS = 20 * 1000 * 1000;

unsigned char *addAgentToBroadcastPacket(unsigned char *currentPosition, Agent *agentToAdd) {
    currentPosition += packSocket(currentPosition, agentToAdd->getPublicSocket());

    AvatarAgentData *agentData = (AvatarAgentData *)agentToAdd->getLinkedData();
    
    int bytesWritten = sprintf((char *)currentPosition,
                               PACKET_FORMAT,
                               agentData->getPitch(),
                               agentData->getYaw(),
                               agentData->getRoll(),
                               agentData->getHeadPositionX(),
                               agentData->getHeadPositionY(),
                               agentData->getHeadPositionZ(),
                               agentData->getLoudness(),
                               agentData->getAverageLoudness(),
                               agentData->getHandPositionX(),
                               agentData->getHandPositionY(),
                               agentData->getHandPositionZ());

    currentPosition += bytesWritten;
    return currentPosition;
}

void attachAvatarDataToAgent(Agent *newAgent) {
    if (newAgent->getLinkedData() == NULL) {
        newAgent->setLinkedData(new AvatarAgentData());
    }
}

int main(int argc, char* argv[])
{
    AgentList *agentList = AgentList::createInstance(AGENT_TYPE_AVATAR_MIXER, AVATAR_LISTEN_PORT);
    setvbuf(stdout, NULL, _IOLBF, 0);
    
    agentList->linkedDataCreateCallback = attachAvatarDataToAgent;
    
    agentList->startDomainServerCheckInThread();
    agentList->startSilentAgentRemovalThread();
    agentList->startPingUnknownAgentsThread();
    
    sockaddr *agentAddress = new sockaddr;
    char *packetData = new char[MAX_PACKET_SIZE];
    ssize_t receivedBytes = 0;
    
    unsigned char *broadcastPacket = new unsigned char[MAX_PACKET_SIZE];
    *broadcastPacket = PACKET_HEADER_BULK_AVATAR_DATA;
    
    unsigned char* currentBufferPosition = NULL;
    int agentIndex = 0;
        
    while (true) {
        if (agentList->getAgentSocket().receive(agentAddress, packetData, &receivedBytes)) {
            switch (packetData[0]) {
                case PACKET_HEADER_HEAD_DATA:
                    // this is positional data from an agent
                    agentList->updateAgentWithData(agentAddress, (void *)packetData, receivedBytes);
                    
                    currentBufferPosition = broadcastPacket + 1;
                    agentIndex = 0;
                    
                    // send back a packet with other active agent data to this agent
                    for (std::vector<Agent>::iterator avatarAgent = agentList->getAgents().begin();
                         avatarAgent != agentList->getAgents().end();
                         avatarAgent++) {
                        if (avatarAgent->getLinkedData() != NULL
                            && agentIndex != agentList->indexOfMatchingAgent(agentAddress)) {
                            currentBufferPosition = addAgentToBroadcastPacket(currentBufferPosition, &*avatarAgent);
                        }
                        
                        agentIndex++;
                    }
                    
                    agentList->getAgentSocket().send(agentAddress,
                                                    broadcastPacket,
                                                    currentBufferPosition - broadcastPacket);
                    
                    break;
                default:
                    // hand this off to the AgentList
                    agentList->processAgentData(agentAddress, (void *)packetData, receivedBytes);
                    break;
            }
        }
    }
    
    agentList->stopDomainServerCheckInThread();
    agentList->stopSilentAgentRemovalThread();
    agentList->stopPingUnknownAgentsThread();
    
    return 0;
}
