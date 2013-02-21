//
//  Agent.cpp
//  hifi
//
//  Created by Stephen Birarda on 2/15/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include "Agent.h"
#include <arpa/inet.h>
#include "UDPSocket.h"

Agent::Agent() {
    
}

Agent::Agent(sockaddr *agentPublicSocket, sockaddr *agentLocalSocket, char agentType) {
    publicSocket = new sockaddr;
    memcpy(publicSocket, agentPublicSocket, sizeof(sockaddr));
    
    localSocket = new sockaddr;
    memcpy(localSocket, agentLocalSocket, sizeof(sockaddr));
    
    type = agentType;
    
    activeSocket = NULL;
    linkedData = NULL;
}

Agent::Agent(const Agent &otherAgent) {
    publicSocket = new sockaddr;
    memcpy(publicSocket, otherAgent.publicSocket, sizeof(sockaddr));
    
    localSocket = new sockaddr;
    memcpy(localSocket, otherAgent.localSocket, sizeof(sockaddr));
    
    if (otherAgent.activeSocket == otherAgent.publicSocket) {
        activeSocket = publicSocket;
    } else if (otherAgent.activeSocket == otherAgent.localSocket) {
        activeSocket = localSocket;
    } else {
        activeSocket = NULL;
    }
    
    type = otherAgent.type;
    
    // linked data is transient, gets re-assigned on next packet receive
    linkedData = NULL;
}

Agent& Agent::operator=(Agent otherAgent) {
    swap(*this, otherAgent);
    return *this;
}

bool Agent::operator==(const Agent& otherAgent) {
    return matches(otherAgent.publicSocket, otherAgent.localSocket, otherAgent.type);
}

void Agent::swap(Agent &first, Agent &second) {
    using std::swap;
    swap(first.publicSocket, second.publicSocket);
    swap(first.localSocket, second.localSocket);
    swap(first.activeSocket, second.activeSocket);
    swap(first.type, second.type);
    swap(first.linkedData, second.linkedData);
}

Agent::~Agent() {
    delete publicSocket;
    delete localSocket;
    delete linkedData;
}

bool Agent::matches(sockaddr *otherPublicSocket, sockaddr *otherLocalSocket, char otherAgentType) {
    // checks if two agent objects are the same agent (same type + local + public address)
    return type == otherAgentType
        && socketMatch(publicSocket, otherPublicSocket)
        && socketMatch(localSocket, otherLocalSocket);
}

std::ostream& operator<<(std::ostream& os, const Agent* agent) {
    sockaddr_in *agentPublicSocket = (sockaddr_in *)agent->publicSocket;
    sockaddr_in *agentLocalSocket = (sockaddr_in *)agent->localSocket;
    
    os << "T: " << agent->type << " PA: " << inet_ntoa(agentPublicSocket->sin_addr) <<
        ":" << ntohs(agentPublicSocket->sin_port) << " LA: " << inet_ntoa(agentLocalSocket->sin_addr) <<
        ":" << ntohs(agentLocalSocket->sin_port);
    return os;
}