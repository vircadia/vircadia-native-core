//
//  AgentList.cpp
//  hifi
//
//  Created by Stephen Birarda on 2/15/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include "AgentList.h"
#include <arpa/inet.h>

AgentList::AgentList() : agentSocket(AGENT_SOCKET_LISTEN_PORT) {
    
}

AgentList::AgentList(int socketListenPort) : agentSocket(socketListenPort) {
    
}

UDPSocket * AgentList::getAgentSocket() {
    return &agentSocket;
}

void AgentList::processAgentData(sockaddr *senderAddress, unsigned char *packetData, size_t dataBytes) {
    switch (packetData[0]) {
        case 'D':
        {
            // list of agents from domain server
            updateList(packetData, dataBytes);
            break;
        }
        case 'P':
        {
            // ping from another agent
            char reply[] = "R";
            agentSocket.send(senderAddress, reply, 1);
            break;
        }
        case 'R':
        {
            // ping reply from another agent
            handlePingReply(senderAddress);
            break;
        }
    }
}

int AgentList::updateList(unsigned char *packetData, size_t dataBytes) {
    int readAgents = 0;

    char agentType;
    
    // assumes only IPv4 addresses
    sockaddr_in agentPublicSocket;
    agentPublicSocket.sin_family = AF_INET;
    sockaddr_in agentLocalSocket;
    agentLocalSocket.sin_family = AF_INET;
    
    unsigned char *readPtr = packetData + 1;
    unsigned char *startPtr = packetData;
    
    while((readPtr - startPtr) < dataBytes) {
        agentType = *readPtr++;
        readPtr += unpackSocket(readPtr, (sockaddr *)&agentPublicSocket);
        readPtr += unpackSocket(readPtr, (sockaddr *)&agentLocalSocket);
        
        addOrUpdateAgent((sockaddr *)&agentPublicSocket, (sockaddr *)&agentLocalSocket, agentType);
    }  

    return readAgents;
}

bool AgentList::addOrUpdateAgent(sockaddr *publicSocket, sockaddr *localSocket, char agentType) {
    std::vector<Agent>::iterator agent;
    
    for(agent = agents.begin(); agent != agents.end(); agent++) {
        if (agent->matches(publicSocket, localSocket, agentType)) {
            // we already have this agent, stop checking
            break;
        }
    }
    
    if (agent == agents.end()) {
        // we didn't have this agent, so add them
        Agent newAgent = Agent(publicSocket, localSocket, agentType);
        
        std::cout << "Added agent - " << &newAgent << "\n";

        agents.push_back(newAgent);
        
        return true;
    } else {
        // we had this agent already
        return false;
    }    
}

void AgentList::pingAgents() {
    for(std::vector<Agent>::iterator agent = agents.begin(); agent != agents.end(); agent++) {
        char payload[] = "P";
        
        if (agent->activeSocket != NULL) {
            // we know which socket is good for this agent, send there
            agentSocket.send((sockaddr *)agent->activeSocket, payload, 1);
        } else {
            // ping both of the sockets for the agent so we can figure out
            // which socket we can use
            agentSocket.send(agent->publicSocket, payload, 1);
            agentSocket.send(agent->localSocket, payload, 1);
        }
    }
}

void AgentList::handlePingReply(sockaddr *agentAddress) {
    for(std::vector<Agent>::iterator agent = agents.begin(); agent != agents.end(); agent++) {
        // check both the public and local addresses for each agent to see if we find a match
        // prioritize the private address so that we prune erroneous local matches
        sockaddr *matchedSocket = NULL;
        
        if (socketMatch(agent->publicSocket, agentAddress)) {
            matchedSocket = agent->publicSocket;
        } else if (socketMatch(agent->localSocket, agentAddress)) {
            matchedSocket = agent->localSocket;
        }
        
        if (matchedSocket != NULL) {
            // matched agent, stop checking
            // update the agent's ping
            break;
        }
    }
}