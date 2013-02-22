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
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/time.h>
#include "AgentList.h"

const int DOMAIN_LISTEN_PORT = 40102;

const int MAX_PACKET_SIZE = 1500;
unsigned char packetData[MAX_PACKET_SIZE];

const int LOGOFF_CHECK_INTERVAL = 5000;

#define DEBUG_TO_SELF 0

int lastActiveCount = 0;
AgentList agentList(DOMAIN_LISTEN_PORT);

int listForBroadcast(unsigned char *listBuffer, sockaddr *agentPublicAddress, sockaddr *agentLocalAddress, char agentType) {
    unsigned char *currentBufferPos = listBuffer + 1;
    unsigned char *startPointer = currentBufferPos;
    
    for(std::vector<Agent>::iterator agent = agentList.agents.begin(); agent != agentList.agents.end(); agent++) {
        
        if (DEBUG_TO_SELF || !agent->matches(agentPublicAddress, agentLocalAddress, agentType)) {
            *currentBufferPos++ = agent->type;
            
            currentBufferPos += packSocket(currentBufferPos, agent->publicSocket);
            currentBufferPos += packSocket(currentBufferPos, agent->localSocket);
        }
    }
    
    return 1 + (currentBufferPos - startPointer); // 1 is added for the leading 'D'
}

int main(int argc, const char * argv[])
{
    ssize_t receivedBytes = 0;
    char agentType;
    unsigned char *broadcastPacket = new unsigned char[MAX_PACKET_SIZE];
    *broadcastPacket = 'D';
    
    sockaddr_in agentPublicAddress, agentLocalAddress;
    agentLocalAddress.sin_family = AF_INET;
    
    agentList.startSilentAgentRemovalThread();
    
    while (true) {
        if (agentList.getAgentSocket()->receive((sockaddr *)&agentPublicAddress, packetData, &receivedBytes)) {
            agentType = packetData[0];
            unpackSocket(&packetData[1], (sockaddr *)&agentLocalAddress);
            
            agentList.addOrUpdateAgent((sockaddr *)&agentPublicAddress, (sockaddr *)&agentLocalAddress, agentType);
            
            int listBytes = listForBroadcast(broadcastPacket, (sockaddr *)&agentPublicAddress, (sockaddr *)&agentLocalAddress, agentType);
            
            if (listBytes > 0) {
                agentList.getAgentSocket()->send((sockaddr *)&agentPublicAddress, broadcastPacket, listBytes);
            }
        }
    }

    return 0;
}

