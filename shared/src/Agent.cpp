//
//  Agent.cpp
//  hifi
//
//  Created by Stephen Birarda on 2/15/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include "Agent.h"

Agent::Agent() {
    
}

Agent::Agent(AgentSocket *agentPublicSocket, AgentSocket *agentLocalSocket, char agentType) {
    publicSocket = new AgentSocket(*agentPublicSocket);
    localSocket = new AgentSocket(*agentLocalSocket);
    activeSocket = NULL;
    type = agentType;
    
    linkedData = 0;
}

Agent::Agent(const Agent &otherAgent) {
    publicSocket = new AgentSocket(*otherAgent.publicSocket);
    localSocket = new AgentSocket(*otherAgent.localSocket);
    
    if (otherAgent.activeSocket == otherAgent.publicSocket) {
        activeSocket = publicSocket;
    } else if (otherAgent.activeSocket == otherAgent.localSocket) {
        activeSocket = localSocket;
    } else {
        activeSocket = NULL;
    }
    
    type = otherAgent.type;
    
    // copy over linkedData
}

Agent& Agent::operator=(const Agent &otherAgent) {
    if (this != &otherAgent) {
        // deallocate old memory
        delete publicSocket;
        delete localSocket;
        delete linkedData;
        
        publicSocket = new AgentSocket(*otherAgent.publicSocket);
        localSocket = new AgentSocket(*otherAgent.localSocket);
        
        if (otherAgent.activeSocket == otherAgent.publicSocket) {
            activeSocket = publicSocket;
        } else if (otherAgent.activeSocket == otherAgent.localSocket) {
            activeSocket = localSocket;
        } else {
            activeSocket = NULL;
        }
        
        type = otherAgent.type;
    }
    
    return *this;
}

Agent::~Agent() {
    delete publicSocket;
    delete localSocket;
    delete linkedData;
}

bool Agent::matches(AgentSocket *otherPublicSocket, AgentSocket *otherLocalSocket, char otherAgentType) {
    return publicSocket->port == otherPublicSocket->port
        && localSocket->port == otherLocalSocket->port
        && type == otherAgentType
        && strcmp(publicSocket->address, otherPublicSocket->address) == 0
        && strcmp(localSocket->address, otherLocalSocket->address) == 0;    
}