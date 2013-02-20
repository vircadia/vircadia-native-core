//
//  AgentList.cpp
//  hifi
//
//  Created by Stephen Birarda on 2/15/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include "AgentList.h"

AgentList::AgentList() : agentSocket(AGENT_SOCKET_LISTEN_PORT) {
    
}

UDPSocket * AgentList::getAgentSocket() {
    return &agentSocket;
}

int AgentList::updateList(char *packetData) {
    int readAgents = 0;
    int scannedItems = 0;
    
    char agentType;
    AgentSocket agentPublicSocket = AgentSocket();
    AgentSocket agentLocalSocket = AgentSocket();
    Agent newAgent;
    
    while((scannedItems = sscanf(packetData,
                                "%c %s %hd %s %hd,",
                                &agentType,
                                agentPublicSocket.address,
                                &agentPublicSocket.port,
                                agentLocalSocket.address,
                                &agentLocalSocket.port
                                ))) {
        
        std::vector<Agent>::iterator agent;
        
        for(agent = agents.begin(); agent != agents.end(); agent++) {
            if (agent->matches(&agentPublicSocket, &agentLocalSocket, agentType)) {
                // we already have this agent, stop checking
                break;
            }
        }
        
        if (agent == agents.end()) {
            // we didn't have this agent, so add them
            newAgent = Agent(&agentPublicSocket, &agentLocalSocket, agentType);
            std::cout << "Added new agent - PS: " << agentPublicSocket << " LS: " << agentLocalSocket << " AT: " << agentType << "\n";
            agents.push_back(newAgent);
        } else {
            // we had this agent already, don't do anything
        }
        
        readAgents++;
    }
    
    return readAgents;
}

void AgentList::pingAgents() {
    for(std::vector<Agent>::iterator agent = agents.begin(); agent != agents.end(); agent++) {
        char payload[] = "P";
        
        if (agent->activeSocket != NULL) {
            // we know which socket is good for this agent, send there
            agentSocket.send(agent->activeSocket, payload, 1);
        } else {
            // ping both of the sockets for the agent so we can figure out
            // which socket we can use
            agentSocket.send(agent->publicSocket, payload, 1);
            agentSocket.send(agent->localSocket, payload, 1);
        }
    }
}