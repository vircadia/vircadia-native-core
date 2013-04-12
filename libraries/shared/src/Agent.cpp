//
//  Agent.cpp
//  hifi
//
//  Created by Stephen Birarda on 2/15/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <pthread.h>
#include "Agent.h"
#include "AgentTypes.h"
#include <cstring>
#include "UDPSocket.h"
#include "SharedUtil.h"

#ifdef _WIN32
#include "Syssocket.h"
#else
#include <arpa/inet.h>
#endif

Agent::Agent() {}

Agent::Agent(sockaddr *agentPublicSocket, sockaddr *agentLocalSocket, char agentType, uint16_t thisAgentId) {
    publicSocket = new sockaddr;
    memcpy(publicSocket, agentPublicSocket, sizeof(sockaddr));
    
    localSocket = new sockaddr;
    memcpy(localSocket, agentLocalSocket, sizeof(sockaddr));
    
    type = agentType;
    agentId = thisAgentId;
    
    firstRecvTimeUsecs = usecTimestampNow();
    lastRecvTimeUsecs = usecTimestampNow();
    
    activeSocket = NULL;
    linkedData = NULL;
    
    pthread_mutex_init(&deleteMutex, NULL);
}

Agent::Agent(const Agent &otherAgent) {
    publicSocket = new sockaddr;
    memcpy(publicSocket, otherAgent.publicSocket, sizeof(sockaddr));
    
    localSocket = new sockaddr;
    memcpy(localSocket, otherAgent.localSocket, sizeof(sockaddr));
    
    agentId = otherAgent.agentId;
    
    if (otherAgent.activeSocket == otherAgent.publicSocket) {
        activeSocket = publicSocket;
    } else if (otherAgent.activeSocket == otherAgent.localSocket) {
        activeSocket = localSocket;
    } else {
        activeSocket = NULL;
    }
    
    firstRecvTimeUsecs = otherAgent.firstRecvTimeUsecs;
    lastRecvTimeUsecs = otherAgent.lastRecvTimeUsecs;
    type = otherAgent.type;
    
    if (otherAgent.linkedData != NULL) {
        linkedData = otherAgent.linkedData->clone();
    } else {
        linkedData = NULL;
    }
    
    pthread_mutex_init(&deleteMutex, NULL);
}

Agent& Agent::operator=(Agent otherAgent) {
    swap(*this, otherAgent);
    return *this;
}

void Agent::swap(Agent &first, Agent &second) {
    using std::swap;
    swap(first.publicSocket, second.publicSocket);
    swap(first.localSocket, second.localSocket);
    swap(first.activeSocket, second.activeSocket);
    swap(first.type, second.type);
    swap(first.linkedData, second.linkedData);
    swap(first.agentId, second.agentId);
}

Agent::~Agent() {
    delete publicSocket;
    delete localSocket;
    delete linkedData;
}

char Agent::getType() const {
    return type;
}

// Names of Agent Types
const char* AGENT_TYPE_NAME_DOMAIN = "Domain";
const char* AGENT_TYPE_NAME_VOXEL = "Voxel Server";
const char* AGENT_TYPE_NAME_INTERFACE = "Client Interface";
const char* AGENT_TYPE_NAME_AUDIO_MIXER = "Audio Mixer";
const char* AGENT_TYPE_NAME_AVATAR_MIXER = "Avatar Mixer";
const char* AGENT_TYPE_NAME_UNKNOWN = "Unknown";

const char* Agent::getTypeName() const {
	const char* name = AGENT_TYPE_NAME_UNKNOWN;
	switch (this->type) {
		case AGENT_TYPE_DOMAIN:
			name = AGENT_TYPE_NAME_DOMAIN;
			break;
		case AGENT_TYPE_VOXEL:
			name = AGENT_TYPE_NAME_VOXEL;
			break;
		case AGENT_TYPE_INTERFACE:
			name = AGENT_TYPE_NAME_INTERFACE;
			break;
		case AGENT_TYPE_AUDIO_MIXER:
			name = AGENT_TYPE_NAME_AUDIO_MIXER;
			break;
        case AGENT_TYPE_AVATAR_MIXER:
            name = AGENT_TYPE_NAME_AVATAR_MIXER;
            break;
	}
	return name;
}

void Agent::setType(char newType) {
    type = newType;
}

uint16_t Agent::getAgentId() {
    return agentId;
}

void Agent::setAgentId(uint16_t thisAgentId) {
    agentId = thisAgentId;
}

double Agent::getFirstRecvTimeUsecs() {
    return firstRecvTimeUsecs;
}

void Agent::setFirstRecvTimeUsecs(double newTimeUsecs) {
    firstRecvTimeUsecs = newTimeUsecs;
}

double Agent::getLastRecvTimeUsecs() {
    return lastRecvTimeUsecs;
}

void Agent::setLastRecvTimeUsecs(double newTimeUsecs) {
    lastRecvTimeUsecs = newTimeUsecs;
}

sockaddr* Agent::getPublicSocket() {
    return publicSocket;
}

void Agent::setPublicSocket(sockaddr *newSocket) {
    publicSocket = newSocket;
}

sockaddr* Agent::getLocalSocket() {
    return localSocket;
}

void Agent::setLocalSocket(sockaddr *newSocket) {
    publicSocket = newSocket;
}

sockaddr* Agent::getActiveSocket() {
    return activeSocket;
}

void Agent::activateLocalSocket() {
    activeSocket = localSocket;
}

void Agent::activatePublicSocket() {
    activeSocket = publicSocket;
}

AgentData* Agent::getLinkedData() {
    return linkedData;
}

void Agent::setLinkedData(AgentData *newData) {
    linkedData = newData;
}


bool Agent::operator==(const Agent& otherAgent) {
    return matches(otherAgent.publicSocket, otherAgent.localSocket, otherAgent.type);
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
    
    os << "T: " << agent->getTypeName() << " (" << agent->type << ") PA: " << inet_ntoa(agentPublicSocket->sin_addr) <<
        ":" << ntohs(agentPublicSocket->sin_port) << " LA: " << inet_ntoa(agentLocalSocket->sin_addr) <<
        ":" << ntohs(agentLocalSocket->sin_port);
    return os;
}