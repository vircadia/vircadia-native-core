//
//  Agent.cpp
//  hifi
//
//  Created by Stephen Birarda on 2/15/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include "stdio.h"

#include <pthread.h>
#include "Agent.h"
#include "AgentTypes.h"
#include <cstring>
#include "Log.h"
#include "UDPSocket.h"
#include "SharedUtil.h"

#ifdef _WIN32
#include "Syssocket.h"
#else
#include <arpa/inet.h>
#endif

int unpackAgentId(unsigned char* packedData, uint16_t* agentId) {
    memcpy(agentId, packedData, sizeof(uint16_t));
    return sizeof(uint16_t);
}

int packAgentId(unsigned char* packStore, uint16_t agentId) {
    memcpy(packStore, &agentId, sizeof(uint16_t));
    return sizeof(uint16_t);
}

Agent::Agent(sockaddr* publicSocket, sockaddr* localSocket, char type, uint16_t agentID) :
    _type(type),
    _agentID(agentID),
    _wakeMicrostamp(usecTimestampNow()),
    _lastHeardMicrostamp(usecTimestampNow()),
    _activeSocket(NULL),
    _bytesReceivedMovingAverage(NULL),
    _linkedData(NULL),
    _isAlive(true)
{
    if (publicSocket) {
        _publicSocket = new sockaddr(*publicSocket);
    } else {
        _publicSocket = NULL;
    }
    
    if (localSocket) {
        _localSocket = new sockaddr(*localSocket);
    } else {
        _localSocket = NULL;
    }
}

Agent::~Agent() {
    delete _publicSocket;
    delete _localSocket;
    delete _linkedData;
    delete _bytesReceivedMovingAverage;
}

// Names of Agent Types
const char* AGENT_TYPE_NAME_DOMAIN = "Domain";
const char* AGENT_TYPE_NAME_VOXEL_SERVER = "Voxel Server";
const char* AGENT_TYPE_NAME_INTERFACE = "Client Interface";
const char* AGENT_TYPE_NAME_AUDIO_MIXER = "Audio Mixer";
const char* AGENT_TYPE_NAME_AVATAR_MIXER = "Avatar Mixer";
const char* AGENT_TYPE_NAME_AUDIO_INJECTOR = "Audio Injector";
const char* AGENT_TYPE_NAME_ANIMATION_SERVER = "Animation Server";
const char* AGENT_TYPE_NAME_UNKNOWN = "Unknown";

const char* Agent::getTypeName() const {
	switch (this->_type) {
		case AGENT_TYPE_DOMAIN:
			return AGENT_TYPE_NAME_DOMAIN;
		case AGENT_TYPE_VOXEL_SERVER:
			return AGENT_TYPE_NAME_VOXEL_SERVER;
		case AGENT_TYPE_AVATAR:
			return AGENT_TYPE_NAME_INTERFACE;
		case AGENT_TYPE_AUDIO_MIXER:
			return AGENT_TYPE_NAME_AUDIO_MIXER;
        case AGENT_TYPE_AVATAR_MIXER:
            return AGENT_TYPE_NAME_AVATAR_MIXER;
        case AGENT_TYPE_AUDIO_INJECTOR:
            return AGENT_TYPE_NAME_AUDIO_INJECTOR;
        case AGENT_TYPE_ANIMATION_SERVER:
            return AGENT_TYPE_NAME_ANIMATION_SERVER;
        default:
            return AGENT_TYPE_NAME_UNKNOWN;
	}
}

void Agent::activateLocalSocket() {
    _activeSocket = _localSocket;
}

void Agent::activatePublicSocket() {
    _activeSocket = _publicSocket;
}

bool Agent::operator==(const Agent& otherAgent) {
    return matches(otherAgent._publicSocket, otherAgent._localSocket, otherAgent._type);
}

bool Agent::matches(sockaddr* otherPublicSocket, sockaddr* otherLocalSocket, char otherAgentType) {
    // checks if two agent objects are the same agent (same type + local + public address)
    return _type == otherAgentType
        && socketMatch(_publicSocket, otherPublicSocket)
        && socketMatch(_localSocket, otherLocalSocket);
}

void Agent::recordBytesReceived(int bytesReceived) {
    if (_bytesReceivedMovingAverage == NULL) {
        _bytesReceivedMovingAverage = new SimpleMovingAverage(100);
    }
    
    _bytesReceivedMovingAverage->updateAverage((float) bytesReceived);
}

float Agent::getAveragePacketsPerSecond() {
    if (_bytesReceivedMovingAverage) {
        return (1 / _bytesReceivedMovingAverage->getEventDeltaAverage());
    } else {
        return 0;
    }
}

float Agent::getAverageKilobitsPerSecond() {
    if (_bytesReceivedMovingAverage) {
        return (_bytesReceivedMovingAverage->getAverageSampleValuePerSecond() * (8.0f / 1000));
    } else {
        return 0;
    }
}

void Agent::printLog(Agent const& agent) {
    
    char publicAddressBuffer[16] = {'\0'};
    unsigned short publicAddressPort = loadBufferWithSocketInfo(publicAddressBuffer, agent._publicSocket);
    
    //char localAddressBuffer[16] = {'\0'};
    //unsigned short localAddressPort = loadBufferWithSocketInfo(localAddressBuffer, agent.localSocket);
    
    ::printLog("# %d %s (%c) @ %s:%d\n",
               agent._agentID,
               agent.getTypeName(),
               agent._type,
               publicAddressBuffer,
               publicAddressPort);
}
