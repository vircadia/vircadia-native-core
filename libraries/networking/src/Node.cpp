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

#include "Node.h"

#include <cstring>
#include <stdio.h>

#include <QtCore/QDataStream>
#include <QtCore/QDebug>

#include <UUID.h>

#include "NetworkLogging.h"
#include "NodePermissions.h"
#include "SharedUtil.h"

const QString UNKNOWN_NodeType_t_NAME = "Unknown";

int NodePtrMetaTypeId = qRegisterMetaType<Node*>("Node*");
int sharedPtrNodeMetaTypeId = qRegisterMetaType<QSharedPointer<Node>>("QSharedPointer<Node>");
int sharedNodePtrMetaTypeId = qRegisterMetaType<SharedNodePointer>("SharedNodePointer");

static const QHash<NodeType_t, QString> TYPE_NAME_HASH {
    { NodeType::DomainServer, "Domain Server" },
    { NodeType::EntityServer, "Entity Server" },
    { NodeType::Agent, "Agent" },
    { NodeType::AudioMixer, "Audio Mixer" },
    { NodeType::AvatarMixer, "Avatar Mixer" },
    { NodeType::MessagesMixer, "Messages Mixer" },
    { NodeType::AssetServer, "Asset Server" },
    { NodeType::EntityScriptServer, "Entity Script Server" },
    { NodeType::UpstreamAudioMixer, "Upstream Audio Mixer" },
    { NodeType::UpstreamAvatarMixer, "Upstream Avatar Mixer" },
    { NodeType::DownstreamAudioMixer, "Downstream Audio Mixer" },
    { NodeType::DownstreamAvatarMixer, "Downstream Avatar Mixer" },
    { NodeType::Unassigned, "Unassigned" }
};

const QString& NodeType::getNodeTypeName(NodeType_t nodeType) {
    const auto matchedTypeName = TYPE_NAME_HASH.find(nodeType);
    return matchedTypeName != TYPE_NAME_HASH.end() ? matchedTypeName.value() : UNKNOWN_NodeType_t_NAME;
}

bool NodeType::isUpstream(NodeType_t nodeType) {
    return nodeType == NodeType::UpstreamAudioMixer || nodeType == NodeType::UpstreamAvatarMixer;
}

bool NodeType::isDownstream(NodeType_t nodeType) {
    return nodeType == NodeType::DownstreamAudioMixer || nodeType == NodeType::DownstreamAvatarMixer;
}

NodeType_t NodeType::upstreamType(NodeType_t primaryType) {
    switch (primaryType) {
        case AudioMixer:
            return UpstreamAudioMixer;
        case AvatarMixer:
            return UpstreamAvatarMixer;
        default:
            return Unassigned;
    }
}

NodeType_t NodeType::downstreamType(NodeType_t primaryType) {
    switch (primaryType) {
        case AudioMixer:
            return DownstreamAudioMixer;
        case AvatarMixer:
            return DownstreamAvatarMixer;
        default:
            return Unassigned;
    }
}

NodeType_t NodeType::fromString(QString type) {
    return TYPE_NAME_HASH.key(type, NodeType::Unassigned);
}


Node::Node(const QUuid& uuid, NodeType_t type, const HifiSockAddr& publicSocket,
    const HifiSockAddr& localSocket, QObject* parent) :
    NetworkPeer(uuid, publicSocket, localSocket, parent),
    _type(type),
    _pingMs(-1),  // "Uninitialized"
    _clockSkewUsec(0),
    _mutex(),
    _clockSkewMovingPercentile(30, 0.8f)   // moving 80th percentile of 30 samples
{
    // Update socket's object name
    setType(_type);
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

Node::NodesIgnoredPair Node::parseIgnoreRequestMessage(QSharedPointer<ReceivedMessage> message) {
    bool addToIgnore;
    message->readPrimitive(&addToIgnore);

    std::vector<QUuid> nodesIgnored;

    while (message->getBytesLeftToRead()) {
        // parse out the UUID being ignored from the packet
        QUuid ignoredUUID = QUuid::fromRfc4122(message->readWithoutCopy(NUM_BYTES_RFC4122_UUID));

        if (addToIgnore) {
            addIgnoredNode(ignoredUUID);
        } else {
            removeIgnoredNode(ignoredUUID);
        }

        nodesIgnored.push_back(ignoredUUID);
    }

    return { nodesIgnored, addToIgnore };
}

void Node::addIgnoredNode(const QUuid& otherNodeID) {
    if (!otherNodeID.isNull() && otherNodeID != _uuid) {
        QWriteLocker lock { &_ignoredNodeIDSetLock };
        qCDebug(networking) << "Adding" << uuidStringWithoutCurlyBraces(otherNodeID) << "to ignore set for"
            << uuidStringWithoutCurlyBraces(_uuid);

        // add the session UUID to the set of ignored ones for this listening node
        if (std::find(_ignoredNodeIDs.begin(), _ignoredNodeIDs.end(), otherNodeID) == _ignoredNodeIDs.end()) {
            _ignoredNodeIDs.push_back(otherNodeID);
        }
    } else {
        qCWarning(networking) << "Node::addIgnoredNode called with null ID or ID of ignoring node.";
    }
}

void Node::removeIgnoredNode(const QUuid& otherNodeID) {
    if (!otherNodeID.isNull() && otherNodeID != _uuid) {
        QWriteLocker lock { &_ignoredNodeIDSetLock };
        qCDebug(networking) << "Removing" << uuidStringWithoutCurlyBraces(otherNodeID) << "from ignore set for"
            << uuidStringWithoutCurlyBraces(_uuid);

        // remove the session UUID from the set of ignored ones for this listening node, if it exists
        auto it = std::remove(_ignoredNodeIDs.begin(), _ignoredNodeIDs.end(), otherNodeID);
        if (it != _ignoredNodeIDs.end()) {
            _ignoredNodeIDs.erase(it);
        }
    } else {
        qCWarning(networking) << "Node::removeIgnoredNode called with null ID or ID of ignoring node.";
    }
}

bool Node::isIgnoringNodeWithID(const QUuid& nodeID) const {
    QReadLocker lock { &_ignoredNodeIDSetLock };

    // check if this node ID is present in the ignore node ID set
    return std::find(_ignoredNodeIDs.begin(), _ignoredNodeIDs.end(), nodeID) != _ignoredNodeIDs.end();
}

QDataStream& operator<<(QDataStream& out, const Node& node) {
    out << node._type;
    out << node._uuid;
    out << node._publicSocket;
    out << node._localSocket;
    out << node._permissions;
    out << node._isReplicated;
    out << node._localID;
    return out;
}

QDataStream& operator>>(QDataStream& in, Node& node) {
    in >> node._type;
    in >> node._uuid;
    in >> node._publicSocket;
    in >> node._localSocket;
    in >> node._permissions;
    in >> node._isReplicated;
    in >> node._localID;
    return in;
}

QDebug operator<<(QDebug debug, const Node& node) {
    debug.nospace() << NodeType::getNodeTypeName(node.getType());
    if (node.getType() == NodeType::Unassigned) {
        debug.nospace() << " (1)";
    } else {
        debug.nospace() << " (" << node.getType() << ")";
    }
    debug << " " << node.getUUID().toString().toLocal8Bit().constData() << "(" << node.getLocalID() << ") ";
    debug.nospace() << node.getPublicSocket() << "/" << node.getLocalSocket();
    return debug.nospace();
}

void Node::setConnectionSecret(const QUuid& connectionSecret) {
    if (_connectionSecret == connectionSecret) {
        return;
    }

    if (!_authenticateHash) {
        _authenticateHash.reset(new HMACAuth());
    }

    _connectionSecret = connectionSecret;
    _authenticateHash->setKey(_connectionSecret);
}
