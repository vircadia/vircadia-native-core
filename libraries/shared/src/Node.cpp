//
//  Node.cpp
//  hifi
//
//  Created by Stephen Birarda on 2/15/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <cstring>
#include <pthread.h>
#include <stdio.h>

#ifdef _WIN32
#include "Syssocket.h"
#else
#include <arpa/inet.h>
#endif

#include "Node.h"
#include "NodeTypes.h"
#include "SharedUtil.h"

#include <QtCore/QDebug>

Node::Node(const QUuid& uuid, char type, const HifiSockAddr& publicSocket, const HifiSockAddr& localSocket) :
    _type(type),
    _uuid(uuid),
    _wakeMicrostamp(usecTimestampNow()),
    _lastHeardMicrostamp(usecTimestampNow()),
    _publicSocket(publicSocket),
    _localSocket(localSocket),
    _activeSocket(NULL),
    _bytesReceivedMovingAverage(NULL),
    _linkedData(NULL),
    _isAlive(true)
{
    pthread_mutex_init(&_mutex, 0);
}

Node::~Node() {
    if (_linkedData) {
        _linkedData->deleteOrDeleteLater();
    }
    
    delete _bytesReceivedMovingAverage;
    
    pthread_mutex_destroy(&_mutex);
}

// Names of Node Types
const char* NODE_TYPE_NAME_DOMAIN = "Domain";
const char* NODE_TYPE_NAME_VOXEL_SERVER = "Voxel Server";
const char* NODE_TYPE_NAME_PARTICLE_SERVER = "Particle Server";
const char* NODE_TYPE_NAME_AGENT = "Agent";
const char* NODE_TYPE_NAME_AUDIO_MIXER = "Audio Mixer";
const char* NODE_TYPE_NAME_AVATAR_MIXER = "Avatar Mixer";
const char* NODE_TYPE_NAME_AUDIO_INJECTOR = "Audio Injector";
const char* NODE_TYPE_NAME_ANIMATION_SERVER = "Animation Server";
const char* NODE_TYPE_NAME_UNASSIGNED = "Unassigned";
const char* NODE_TYPE_NAME_UNKNOWN = "Unknown";

const char* Node::getTypeName() const {
	switch (this->_type) {
		case NODE_TYPE_DOMAIN:
			return NODE_TYPE_NAME_DOMAIN;
		case NODE_TYPE_VOXEL_SERVER:
			return NODE_TYPE_NAME_VOXEL_SERVER;
		case NODE_TYPE_PARTICLE_SERVER:
		    return NODE_TYPE_NAME_PARTICLE_SERVER;
		case NODE_TYPE_AGENT:
			return NODE_TYPE_NAME_AGENT;
		case NODE_TYPE_AUDIO_MIXER:
			return NODE_TYPE_NAME_AUDIO_MIXER;
        case NODE_TYPE_AVATAR_MIXER:
            return NODE_TYPE_NAME_AVATAR_MIXER;
        case NODE_TYPE_AUDIO_INJECTOR:
            return NODE_TYPE_NAME_AUDIO_INJECTOR;
        case NODE_TYPE_ANIMATION_SERVER:
            return NODE_TYPE_NAME_ANIMATION_SERVER;
        case NODE_TYPE_UNASSIGNED:
            return NODE_TYPE_NAME_UNASSIGNED;
        default:
            return NODE_TYPE_NAME_UNKNOWN;
	}
}

void Node::setPublicSocket(const HifiSockAddr& publicSocket) {
    if (_activeSocket == &_publicSocket) {
        // if the active socket was the public socket then reset it to NULL
        _activeSocket = NULL;
    }
    
    _publicSocket = publicSocket;
}

void Node::setLocalSocket(const HifiSockAddr& localSocket) {
    if (_activeSocket == &_localSocket) {
        // if the active socket was the local socket then reset it to NULL
        _activeSocket = NULL;
    }
    
    _localSocket = localSocket;
}

void Node::activateLocalSocket() {
    qDebug() << "Activating local socket for node" << *this << "\n";
    _activeSocket = &_localSocket;
}

void Node::activatePublicSocket() {
    qDebug() << "Activating public socket for node" << *this << "\n";
    _activeSocket = &_publicSocket;
}

void Node::recordBytesReceived(int bytesReceived) {
    if (_bytesReceivedMovingAverage == NULL) {
        _bytesReceivedMovingAverage = new SimpleMovingAverage(100);
    }
    
    _bytesReceivedMovingAverage->updateAverage((float) bytesReceived);
}

float Node::getAveragePacketsPerSecond() {
    if (_bytesReceivedMovingAverage) {
        return (1 / _bytesReceivedMovingAverage->getEventDeltaAverage());
    } else {
        return 0;
    }
}

float Node::getAverageKilobitsPerSecond() {
    if (_bytesReceivedMovingAverage) {
        return (_bytesReceivedMovingAverage->getAverageSampleValuePerSecond() * (8.0f / 1000));
    } else {
        return 0;
    }
}

QDebug operator<<(QDebug debug, const Node &node) {
    debug.nospace() << node.getTypeName() << " (" << node.getType() << ")";
    debug << " " << node.getUUID().toString().toLocal8Bit().constData() << " ";
    debug.nospace() << node.getPublicSocket() << "/" << node.getLocalSocket();
    return debug.nospace();
}
