//
//  Node.cpp
//  libraries/networking/src
//
//  Created by Stephen Birarda on 2/15/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <cstring>
#include <stdio.h>

#include <UUID.h>

#include "Node.h"
#include "SharedUtil.h"

#include <QtCore/QDataStream>
#include <QtCore/QDebug>

const QString UNKNOWN_NodeType_t_NAME = "Unknown";

namespace NodeType {
    QHash<NodeType_t, QString> TypeNameHash;
}

void NodeType::init() {
    TypeNameHash.insert(NodeType::DomainServer, "Domain Server");
    TypeNameHash.insert(NodeType::VoxelServer, "Voxel Server");
    TypeNameHash.insert(NodeType::EntityServer, "Entity Server");
    TypeNameHash.insert(NodeType::MetavoxelServer, "Metavoxel Server");
    TypeNameHash.insert(NodeType::Agent, "Agent");
    TypeNameHash.insert(NodeType::AudioMixer, "Audio Mixer");
    TypeNameHash.insert(NodeType::AvatarMixer, "Avatar Mixer");
    TypeNameHash.insert(NodeType::AnimationServer, "Animation Server");
    TypeNameHash.insert(NodeType::Unassigned, "Unassigned");
}

const QString& NodeType::getNodeTypeName(NodeType_t nodeType) {
    QHash<NodeType_t, QString>::iterator matchedTypeName = TypeNameHash.find(nodeType);
    return matchedTypeName != TypeNameHash.end() ? matchedTypeName.value() : UNKNOWN_NodeType_t_NAME;
}

Node::Node(const QUuid& uuid, NodeType_t type, const HifiSockAddr& publicSocket, const HifiSockAddr& localSocket) :
	NetworkPeer(uuid, publicSocket, localSocket),
    _type(type),
    _activeSocket(NULL),
    _symmetricSocket(),
    _connectionSecret(),
    _bytesReceivedMovingAverage(NULL),
    _linkedData(NULL),
    _isAlive(true),
    _clockSkewUsec(0),
    _mutex(),
    _clockSkewMovingPercentile(30, 0.8f)   // moving 80th percentile of 30 samples
{
    
}

Node::~Node() {
    delete _linkedData;
    delete _bytesReceivedMovingAverage;
}

void Node::recordBytesReceived(int bytesReceived) {
    if (!_bytesReceivedMovingAverage) {
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

void Node::updateClockSkewUsec(int clockSkewSample) {
    _clockSkewMovingPercentile.updatePercentile((float)clockSkewSample);
    _clockSkewUsec = (int)_clockSkewMovingPercentile.getValueAtPercentile();
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

void Node::setSymmetricSocket(const HifiSockAddr& symmetricSocket) {
    if (_activeSocket == &_symmetricSocket) {
        // if the active socket was the symmetric socket then reset it to NULL
        _activeSocket = NULL;
    }
    
    _symmetricSocket = symmetricSocket;
}

void Node::activateLocalSocket() {
    qDebug() << "Activating local socket for network peer with ID" << uuidStringWithoutCurlyBraces(_uuid);
    _activeSocket = &_localSocket;
}

void Node::activatePublicSocket() {
    qDebug() << "Activating public socket for network peer with ID" << uuidStringWithoutCurlyBraces(_uuid);
    _activeSocket = &_publicSocket;
}

void Node::activateSymmetricSocket() {
    qDebug() << "Activating symmetric socket for network peer with ID" << uuidStringWithoutCurlyBraces(_uuid);
    _activeSocket = &_symmetricSocket;
}

QDataStream& operator<<(QDataStream& out, const Node& node) {
    out << node._type;
    out << node._uuid;
    out << node._publicSocket;
    out << node._localSocket;
    
    return out;
}

QDataStream& operator>>(QDataStream& in, Node& node) {
    in >> node._type;
    in >> node._uuid;
    in >> node._publicSocket;
    in >> node._localSocket;
    
    return in;
}

QDebug operator<<(QDebug debug, const Node &node) {
    debug.nospace() << NodeType::getNodeTypeName(node.getType()) << " (" << node.getType() << ")";
    debug << " " << node.getUUID().toString().toLocal8Bit().constData() << " ";
    debug.nospace() << node.getPublicSocket() << "/" << node.getLocalSocket();
    return debug.nospace();
}
