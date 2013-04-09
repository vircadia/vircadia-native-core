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
const unsigned short BROADCAST_INTERVAL = 20;

AgentList agentList(AGENT_TYPE_AVATAR_MIXER, AVATAR_LISTEN_PORT);

unsigned char *addAgentToBroadcastPacket(unsigned char *currentPosition, Agent *agentToAdd) {
    unsigned char *packetData = new unsigned char();
    
    currentPosition += packSocket(currentPosition, agentToAdd->getActiveSocket());

    AvatarAgentData *agentData = (AvatarAgentData *)agentToAdd->getLinkedData();
    
    sprintf((char *)packetData, PACKET_FORMAT, agentData->getPitch(),
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

    memcpy(currentPosition, packetData, strlen((const char*)packetData));
    currentPosition += strlen((const char*)packetData);

    return currentPosition;
}

void *sendAvatarData(void *args) {
    timeval startTime;
    while (true) {
        gettimeofday(&startTime, NULL);
        
        unsigned char *currentBufferPosition;
        unsigned char *startPointer;
        unsigned char *broadcastPacket = new unsigned char[MAX_PACKET_SIZE];
        
        broadcastPacket = (unsigned char *)PACKET_HEADER_HEAD_DATA;
        currentBufferPosition = broadcastPacket + 1;
        startPointer = currentBufferPosition;
        
        
        for (std::vector<Agent>::iterator avatarAgent = agentList.getAgents().begin();
             avatarAgent != agentList.getAgents().end();
             avatarAgent++) {
            addAgentToBroadcastPacket(currentBufferPosition, &*avatarAgent);
        }
        
        for (std::vector<Agent>::iterator avatarAgent = agentList.getAgents().begin();
             avatarAgent != agentList.getAgents().end();
             avatarAgent++) {
            agentList.getAgentSocket().send(avatarAgent->getActiveSocket(), broadcastPacket, strlen((const char *)broadcastPacket));
        }
        
        double usecToSleep = usecTimestamp(&startTime) + (BROADCAST_INTERVAL * 10000000) - usecTimestampNow();
        usleep(usecToSleep);
    }
}

void attachAvatarDataToAgent(Agent *newAgent) {
    if (newAgent->getLinkedData() == NULL) {
        newAgent->setLinkedData(new AvatarAgentData());
    }
}

int main(int argc, char* argv[])
{
    setvbuf(stdout, NULL, _IOLBF, 0);
    
    pthread_t sendAvatarDataThread;
    pthread_create(&sendAvatarDataThread, NULL, sendAvatarData, NULL);
    
    agentList.linkedDataCreateCallback = attachAvatarDataToAgent;
    
    agentList.startDomainServerCheckInThread();
    agentList.startSilentAgentRemovalThread();
    
    sockaddr *agentAddress = new sockaddr;
    char *packetData = new char[MAX_PACKET_SIZE];
    ssize_t receivedBytes = 0;
        
    while (true) {
        if (agentList.getAgentSocket().receive(agentAddress, packetData, &receivedBytes)) {
            if (packetData[0] == PACKET_HEADER_HEAD_DATA) {
                
                if (agentList.addOrUpdateAgent(agentAddress, agentAddress, AGENT_TYPE_INTERFACE, agentList.getLastAgentId())) {
                    agentList.increaseAgentId();
                }
                
                agentList.updateAgentWithData(agentAddress, (void *)packetData, receivedBytes);
            }
        }
    }
    
    agentList.stopDomainServerCheckInThread();
    agentList.stopSilentAgentRemovalThread();
}
