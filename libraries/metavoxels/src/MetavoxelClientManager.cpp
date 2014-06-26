//
//  MetavoxelClientManager.cpp
//  libraries/metavoxels/src
//
//  Created by Andrzej Kapolka on 6/26/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "MetavoxelClientManager.h"
#include "MetavoxelMessages.h"

void MetavoxelClientManager::init() {
    connect(NodeList::getInstance(), SIGNAL(nodeAdded(SharedNodePointer)), SLOT(maybeAttachClient(const SharedNodePointer&)));
}

void MetavoxelClientManager::update() {
    foreach (const SharedNodePointer& node, NodeList::getInstance()->getNodeHash()) {
        if (node->getType() == NodeType::MetavoxelServer) {
            QMutexLocker locker(&node->getMutex());
            MetavoxelClient* client = static_cast<MetavoxelClient*>(node->getLinkedData());
            if (client) {
                client->update();
            }
        }
    }
}

SharedObjectPointer MetavoxelClientManager::findFirstRaySpannerIntersection(const glm::vec3& origin,
        const glm::vec3& direction, const AttributePointer& attribute, float& distance) {
    SharedObjectPointer closestSpanner;
    float closestDistance = FLT_MAX;
    foreach (const SharedNodePointer& node, NodeList::getInstance()->getNodeHash()) {
        if (node->getType() == NodeType::MetavoxelServer) {
            QMutexLocker locker(&node->getMutex());
            MetavoxelClient* client = static_cast<MetavoxelClient*>(node->getLinkedData());
            if (client) {
                float clientDistance;
                SharedObjectPointer clientSpanner = client->getData().findFirstRaySpannerIntersection(
                    origin, direction, attribute, clientDistance);
                if (clientSpanner && clientDistance < closestDistance) {
                    closestSpanner = clientSpanner;
                    closestDistance = clientDistance;
                }
            }
        }
    }
    if (closestSpanner) {
        distance = closestDistance;
    }
    return closestSpanner;
}

void MetavoxelClientManager::applyEdit(const MetavoxelEditMessage& edit, bool reliable) {
    foreach (const SharedNodePointer& node, NodeList::getInstance()->getNodeHash()) {
        if (node->getType() == NodeType::MetavoxelServer) {
            QMutexLocker locker(&node->getMutex());
            MetavoxelClient* client = static_cast<MetavoxelClient*>(node->getLinkedData());
            if (client) {
                client->applyEdit(edit, reliable);
            }
        }
    }
}

MetavoxelLOD MetavoxelClientManager::getLOD() const {
    return MetavoxelLOD();
}

void MetavoxelClientManager::maybeAttachClient(const SharedNodePointer& node) {
    if (node->getType() == NodeType::MetavoxelServer) {
        QMutexLocker locker(&node->getMutex());
        node->setLinkedData(new MetavoxelClient(node, this));
    }
}

MetavoxelClient::MetavoxelClient(const SharedNodePointer& node, MetavoxelClientManager* manager) :
    Endpoint(node),
    _manager(manager) {
}

void MetavoxelClient::guide(MetavoxelVisitor& visitor) {
    visitor.setLOD(_manager->getLOD());
    _data.guide(visitor);
}

void MetavoxelClient::applyEdit(const MetavoxelEditMessage& edit, bool reliable) {
    if (reliable) {
        _sequencer.getReliableOutputChannel()->sendMessage(QVariant::fromValue(edit));
    
    } else {
        // apply immediately to local tree
        edit.apply(_data, _sequencer.getWeakSharedObjectHash());

        // start sending it out
        _sequencer.sendHighPriorityMessage(QVariant::fromValue(edit));
    }
}

void MetavoxelClient::writeUpdateMessage(Bitstream& out) {
    ClientStateMessage state = { _manager->getLOD() };
    out << QVariant::fromValue(state);
}

void MetavoxelClient::readMessage(Bitstream& in) {
    Endpoint::readMessage(in);
    
    // reapply local edits
    foreach (const DatagramSequencer::HighPriorityMessage& message, _sequencer.getHighPriorityMessages()) {
        if (message.data.userType() == MetavoxelEditMessage::Type) {
            message.data.value<MetavoxelEditMessage>().apply(_data, _sequencer.getWeakSharedObjectHash());
        }
    }
}

class ReceiveRecord : public PacketRecord {
public:
    
    ReceiveRecord(const MetavoxelLOD& lod = MetavoxelLOD(), const MetavoxelData& data = MetavoxelData());

    const MetavoxelData& getData() const { return _data; }

private:
    
    MetavoxelData _data;
};

ReceiveRecord::ReceiveRecord(const MetavoxelLOD& lod, const MetavoxelData& data) :
    PacketRecord(lod),
    _data(data) {
}

void MetavoxelClient::handleMessage(const QVariant& message, Bitstream& in) {
    if (message.userType() == MetavoxelDeltaMessage::Type) {
        ReceiveRecord* receiveRecord = static_cast<ReceiveRecord*>(getLastAcknowledgedReceiveRecord());
        _data.readDelta(receiveRecord->getData(), receiveRecord->getLOD(), in, getLastAcknowledgedSendRecord()->getLOD());
    
    } else {
        Endpoint::handleMessage(message, in);
    }
}

PacketRecord* MetavoxelClient::maybeCreateSendRecord(bool baseline) const {
    return baseline ? new PacketRecord() : new PacketRecord(_manager->getLOD());
}

PacketRecord* MetavoxelClient::maybeCreateReceiveRecord(bool baseline) const {
    return baseline ? new ReceiveRecord() : new ReceiveRecord(getLastAcknowledgedSendRecord()->getLOD(), _data);
}
