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
#include "shared_Log.h"
#include "UDPSocket.h"
#include "SharedUtil.h"

#ifdef _WIN32
#include "Syssocket.h"
#else
#include <arpa/inet.h>
#endif

using shared_lib::printLog;

int unpackAgentId(unsigned char *packedData, uint16_t *agentId) {
    memcpy(agentId, packedData, sizeof(uint16_t));
    return sizeof(uint16_t);
}

int packAgentId(unsigned char *packStore, uint16_t agentId) {
    memcpy(packStore, &agentId, sizeof(uint16_t));
    return sizeof(uint16_t);
}

Agent::Agent(sockaddr *agentPublicSocket, sockaddr *agentLocalSocket, char agentType, uint16_t thisAgentId) :
    _isAlive(true)
{
    if (agentPublicSocket != NULL) {
        publicSocket = new sockaddr;
        memcpy(publicSocket, agentPublicSocket, sizeof(sockaddr));
    } else {
        publicSocket = NULL;
    }
    
    if (agentLocalSocket != NULL) {
        localSocket = new sockaddr;
        memcpy(localSocket, agentLocalSocket, sizeof(sockaddr));
    } else {
        localSocket = NULL;
    }
    
    type = agentType;
    agentId = thisAgentId;
    
    _wakeMicrostamp = usecTimestampNow();
    _lastHeardMicrostamp = usecTimestampNow();
    
    activeSocket = NULL;
    linkedData = NULL;
    _bytesReceivedMovingAverage = NULL;
}

Agent::Agent(const Agent &otherAgent) {
    _isAlive = otherAgent._isAlive;
    
    if (otherAgent.publicSocket != NULL) {
        publicSocket = new sockaddr;
        memcpy(publicSocket, otherAgent.publicSocket, sizeof(sockaddr));
    } else {
        publicSocket = NULL;
    }
    
    if (otherAgent.localSocket != NULL) {
        localSocket = new sockaddr;
        memcpy(localSocket, otherAgent.localSocket, sizeof(sockaddr));
    } else {
        localSocket = NULL;
    }
    
    agentId = otherAgent.agentId;
    
    if (otherAgent.activeSocket == otherAgent.publicSocket) {
        activeSocket = publicSocket;
    } else if (otherAgent.activeSocket == otherAgent.localSocket) {
        activeSocket = localSocket;
    } else {
        activeSocket = NULL;
    }
    
    _wakeMicrostamp = otherAgent._wakeMicrostamp;
    _lastHeardMicrostamp = otherAgent._lastHeardMicrostamp;
    type = otherAgent.type;
    
    if (otherAgent.linkedData != NULL) {
        linkedData = otherAgent.linkedData->clone();
    } else {
        linkedData = NULL;
    }
    
    if (otherAgent._bytesReceivedMovingAverage != NULL) {
        _bytesReceivedMovingAverage = new SimpleMovingAverage(100);
        memcpy(_bytesReceivedMovingAverage, otherAgent._bytesReceivedMovingAverage, sizeof(SimpleMovingAverage));
    } else {
        _bytesReceivedMovingAverage = NULL;
    }
}

Agent& Agent::operator=(Agent otherAgent) {
    swap(*this, otherAgent);
    return *this;
}

void Agent::swap(Agent &first, Agent &second) {
    using std::swap;
    
    swap(first._isAlive, second._isAlive);
    swap(first.publicSocket, second.publicSocket);
    swap(first.localSocket, second.localSocket);
    swap(first.activeSocket, second.activeSocket);
    swap(first.type, second.type);
    swap(first.linkedData, second.linkedData);
    swap(first.agentId, second.agentId);
    swap(first._wakeMicrostamp, second._wakeMicrostamp);
    swap(first._lastHeardMicrostamp, second._lastHeardMicrostamp);
    swap(first._bytesReceivedMovingAverage, second._bytesReceivedMovingAverage);
}

Agent::~Agent() {
    delete publicSocket;
    delete localSocket;
    delete linkedData;
    delete _bytesReceivedMovingAverage;
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
		case AGENT_TYPE_AVATAR:
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

void Agent::recordBytesReceived(int bytesReceived) {
    if (_bytesReceivedMovingAverage == NULL) {
        _bytesReceivedMovingAverage = new SimpleMovingAverage(100);
    }
    
    _bytesReceivedMovingAverage->updateAverage((float) bytesReceived);
}

float Agent::getAveragePacketsPerSecond() {
    if (_bytesReceivedMovingAverage != NULL) {
        return (1 / _bytesReceivedMovingAverage->getEventDeltaAverage());
    } else {
        return 0;
    }
}

float Agent::getAverageKilobitsPerSecond() {
    if (_bytesReceivedMovingAverage != NULL) {
        return (_bytesReceivedMovingAverage->getAverageSampleValuePerSecond() * (8.0f / 1000));
    } else {
        return 0;
    }
}

void Agent::printLog(Agent const& agent) {
    
    char publicAddressBuffer[16] = {'\0'};
    unsigned short publicAddressPort = loadBufferWithSocketInfo(publicAddressBuffer, agent.publicSocket);
    
    //char localAddressBuffer[16] = {'\0'};
    //unsigned short localAddressPort = loadBufferWithSocketInfo(localAddressBuffer, agent.localSocket);
    
    ::printLog("# %d %s (%c) @ %s:%d\n",
               agent.agentId,
               agent.getTypeName(),
               agent.type,
               publicAddressBuffer,
               publicAddressPort);
}