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

#include <QtCore/QDataStream>
#include <QtCore/QDebug>

#include <UUID.h>

#include "NetworkLogging.h"
#include "NodePermissions.h"
#include "SharedUtil.h"

#include "Node.h"

const QString UNKNOWN_NodeType_t_NAME = "Unknown";

int NodePtrMetaTypeId = qRegisterMetaType<Node*>("Node*");
int sharedPtrNodeMetaTypeId = qRegisterMetaType<QSharedPointer<Node>>("QSharedPointer<Node>");
int sharedNodePtrMetaTypeId = qRegisterMetaType<SharedNodePointer>("SharedNodePointer");

namespace NodeType {
    QHash<NodeType_t, QString> TypeNameHash;
}

void NodeType::init() {
    TypeNameHash.insert(NodeType::DomainServer, "Domain Server");
    TypeNameHash.insert(NodeType::EntityServer, "Entity Server");
    TypeNameHash.insert(NodeType::Agent, "Agent");
    TypeNameHash.insert(NodeType::AudioMixer, "Audio Mixer");
    TypeNameHash.insert(NodeType::AvatarMixer, "Avatar Mixer");
    TypeNameHash.insert(NodeType::MessagesMixer, "Messages Mixer");
    TypeNameHash.insert(NodeType::AssetServer, "Asset Server");
    TypeNameHash.insert(NodeType::Unassigned, "Unassigned");
}

const QString& NodeType::getNodeTypeName(NodeType_t nodeType) {
    QHash<NodeType_t, QString>::iterator matchedTypeName = TypeNameHash.find(nodeType);
    return matchedTypeName != TypeNameHash.end() ? matchedTypeName.value() : UNKNOWN_NodeType_t_NAME;
}

Node::Node(const QUuid& uuid, NodeType_t type, const HifiSockAddr& publicSocket,
           const HifiSockAddr& localSocket, const NodePermissions& permissions, const QUuid& connectionSecret,
           QObject* parent) :
    NetworkPeer(uuid, publicSocket, localSocket, parent),
    _type(type),
    _connectionSecret(connectionSecret),
    _isAlive(true),
    _pingMs(-1),  // "Uninitialized"
    _clockSkewUsec(0),
    _mutex(),
    _clockSkewMovingPercentile(30, 0.8f),   // moving 80th percentile of 30 samples
    _permissions(permissions)
{
    // Update socket's object name
    setType(_type);
    _ignoreRadiusEnabled = false;
}

void Node::setType(char type) {
    _type = type;
    
    auto typeString = NodeType::getNodeTypeName(type);
    _publicSocket.setObjectName(typeString);
    _localSocket.setObjectName(typeString);
    _symmetricSocket.setObjectName(typeString);
}

void Node::updateClockSkewUsec(qint64 clockSkewSample) {
    _clockSkewMovingPercentile.updatePercentile(clockSkewSample);
    _clockSkewUsec = (quint64)_clockSkewMovingPercentile.getValueAtPercentile();
}

void Node::parseIgnoreRequestMessage(QSharedPointer<ReceivedMessage> message) {    
    while (message->getBytesLeftToRead()) {
        // parse out the UUID being ignored from the packet
        QUuid ignoredUUID = QUuid::fromRfc4122(message->readWithoutCopy(NUM_BYTES_RFC4122_UUID));

        addIgnoredNode(ignoredUUID);
    }
}

void Node::addIgnoredNode(const QUuid& otherNodeID) {
    if (!otherNodeID.isNull() && otherNodeID != _uuid) {
        qCDebug(networking) << "Adding" << uuidStringWithoutCurlyBraces(otherNodeID) << "to ignore set for"
        << uuidStringWithoutCurlyBraces(_uuid);

        // add the session UUID to the set of ignored ones for this listening node
        _ignoredNodeIDSet.insert(otherNodeID);
    } else {
        qCWarning(networking) << "Node::addIgnoredNode called with null ID or ID of ignoring node.";
    }
}

void Node::parseIgnoreRadiusRequestMessage(QSharedPointer<ReceivedMessage> message) {
    bool enabled;
    float radius;
    message->readPrimitive(&enabled);
    message->readPrimitive(&radius);
    _ignoreRadiusEnabled = enabled;
    _ignoreRadius = radius;
}

QDataStream& operator<<(QDataStream& out, const Node& node) {
    out << node._type;
    out << node._uuid;
    out << node._publicSocket;
    out << node._localSocket;
    out << node._permissions;
    return out;
}

QDataStream& operator>>(QDataStream& in, Node& node) {
    in >> node._type;
    in >> node._uuid;
    in >> node._publicSocket;
    in >> node._localSocket;
    in >> node._permissions;
    return in;
}

QDebug operator<<(QDebug debug, const Node& node) {
    debug.nospace() << NodeType::getNodeTypeName(node.getType());
    if (node.getType() == NodeType::Unassigned) {
        debug.nospace() << " (1)";
    } else {
        debug.nospace() << " (" << node.getType() << ")";
    }
    debug << " " << node.getUUID().toString().toLocal8Bit().constData() << " ";
    debug.nospace() << node.getPublicSocket() << "/" << node.getLocalSocket();
    return debug.nospace();
}
