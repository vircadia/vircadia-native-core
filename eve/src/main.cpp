//
//  main.cpp
//  eve
//
//  Created by Stephen Birarda on 4/22/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <AgentTypes.h>
#include <AgentList.h>
#include <PacketHeaders.h>

const int EVE_AGENT_LIST_PORT = 55441;

bool stopReceiveAgentDataThread;

void *receiveAgentData(void *args)
{
    sockaddr senderAddress;
    ssize_t bytesReceived;
    unsigned char incomingPacket[MAX_PACKET_SIZE];
    
    while (!::stopReceiveAgentDataThread) {
        if (AgentList::getInstance()->getAgentSocket().receive(&senderAddress, incomingPacket, &bytesReceived)) {
            
            // we're going to ignore any data that isn't the agent list from the domain server
            // ex: the avatar mixer will be sending us the position of other agents
            switch (incomingPacket[0]) {
                case PACKET_HEADER_DOMAIN:
                    AgentList::getInstance()->processAgentData(&senderAddress, incomingPacket, bytesReceived);
                    break;
            }
        }
    }
    
    pthread_exit(0);
    return NULL;
}

int main(int argc, char* argv[]) {
    // create an AgentList instance to handle communication with other agents
    AgentList *agentList = AgentList::createInstance(AGENT_TYPE_AVATAR, EVE_AGENT_LIST_PORT);
    
    // start telling the domain server that we are alive
    agentList->startDomainServerCheckInThread();
    
    // start the agent list thread that will kill off agents when they stop talking
    agentList->startSilentAgentRemovalThread();
    
    // start the ping thread that hole punches to create an active connection to other agents
    agentList->startPingUnknownAgentsThread();
    
    // stop the agent list's threads
    agentList->stopDomainServerCheckInThread();
    agentList->stopPingUnknownAgentsThread();
    agentList->stopSilentAgentRemovalThread();
}


