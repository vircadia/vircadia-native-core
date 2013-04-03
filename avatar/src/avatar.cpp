//
//  avatar.cpp
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
#include <AgentList.h>
#include <SharedUtil.h>
#include <PacketCodes.h>
#include <StdDev.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "avatar.h"

AgentList agentList(PKT_AVATAR_MIXER, AVATAR_LISTEN_PORT);
const char *packetFormat = "%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f";

unsigned char *addAgentToBroadcastPacket(unsigned char *currentPosition, Agent *agentToAdd) {
    Head *agentHead = (Head *)agentToAdd->getLinkedData();
    Hand *agentHand = (Hand *)agentToAdd->getLinkedData();
    unsigned char *packetData;
    glm::vec3 headPosition = agentHead->getPos();
    glm::vec3 handPosition = agentHand->getPos();
    
    *currentPosition += packAgentId(currentPosition, agentToAdd->getAgentId());
    currentPosition += packSocket(currentPosition, agentToAdd->getActiveSocket());
    
    sprintf(packetData, packetFormat, agentHead->getPitch(),
                                      agentHead->getYaw(),
                                      agentHead->getRoll(),
                                      headPosition.x,
                                      headPosition.y,
                                      headPosition.z,
                                      agentHead->getLoudness(),
                                      agentHead->getAverageLoudness(),
                                      handPosition.x,
                                      handPosition.y,
                                      handPosition.z)
    
    memcpy(currentPosition, packetData, strlen(packetData));
    currentPosition += strlen(packetData);
    
    // return the new unsigned char * for broadcast packet
    return currentPosition;
}

void *sendAvatarData(void *args)
{
    timeval startTime;
    while (true) {
        gettimeofday(&startTime, NULL);
        
        unsigned char *currentBufferPosition;
        unsigned char *startPointer;
        unsigned char *broadcastPacket = new unsigned char[MAX_PACKET_SIZE];
        
        *broadcastPacket = PKT_AGENT_DATA;
        currentBufferPosition = broadcastPacket + 1;
        startPointer = currentBufferPosition;
        
        // Construct packet with data for all agents
        for (std::vector<Agent>::iterator agent = agentList.getAgents().begin(); agent != agentList.getAgents().end(); agent++) {
            if (agent->getLinkedData() != NULL) {
                addAgentToBroadcastPacket(currentBufferPosition, agent);
            }
        }
        
        // Stream the constructed packet to all agents
        for (std::vector<Agent>::iterator agent = agentList.getAgents().begin(); agent != agentList.getAgents().end(); agent++) {
            agentList.getAgentSocket().send(agent->getActiveSocket(), broadcastPacket, strlen(broadcastPacket));
        }
        
        double usecToSleep = usecTimestamp(&startTime) + (BROADCAST_INTERVAL * 10000000) - usecTimestampNow();
        usleep(usecToSleep);
    }
}

int main(int argc, char* argv[])
{
    setvbuf(stdout, NULL, _IOLBF, 0);
    
    agentList.startSilentAgentRemovalThread();
    agentList.startDomainServerCheckInThread();
    
    pthread_t sendAvatarDataThread;
    pthread_create(&sendAvatarDataThread, NULL, sendAvatarData, NULL);
    
    sockaddr *agentAddress = new sockaddr;
    unsigned char *packetData = new unsigned char[MAX_PACKET_SIZE];
    ssize_t receivedBytes = 0;
    
    while (true) {
        if(agentList.getAgentSocket().receive(agentAddress, packetData, &receivedBytes)) {
            if (packetData[0] == 'H') {
                if (agentList.addOrUpdateAgent(agentAddress, agentAddress, packetData[0], agentList.getLastAgentId())) {
                    agentList.increaseAgentId();
                }
                agentList.updateAgentWithData(agentAddress, packetData, receivedBytes);
            }
        }
    }
}
