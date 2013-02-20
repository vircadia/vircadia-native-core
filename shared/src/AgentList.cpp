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
        
        std::vector<Agent>::iterator it;
        
        for(it = agents.begin(); it != agents.end(); ++it) {
            if (it->matches(&agentPublicSocket, &agentLocalSocket, agentType)) {
                // we already have this agent, stop checking
                break;
            }
        }
        
        if (it == agents.end()) {
            // we didn't have this agent, so add them
            newAgent = Agent(&agentPublicSocket, &agentLocalSocket, agentType);
            agents.push_back(newAgent);
        } else {
            // we had this agent already, don't do anything
        }
        
        readAgents++;
    }
    
    return readAgents;
}