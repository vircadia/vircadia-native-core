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

#include <QThread>

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
                updateClient(client);
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

void MetavoxelClientManager::setSphere(const glm::vec3& center, float radius, const QColor& color) {
    Sphere* sphere = new Sphere();
    sphere->setTranslation(center);
    sphere->setScale(radius);
    sphere->setColor(color);
    setSpanner(sphere);
}

void MetavoxelClientManager::setSpanner(const SharedObjectPointer& object, bool reliable) {
    MetavoxelEditMessage edit = { QVariant::fromValue(SetSpannerEdit(object)) };
    applyEdit(edit, reliable);
}

void MetavoxelClientManager::applyEdit(const MetavoxelEditMessage& edit, bool reliable) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "applyEdit", Q_ARG(const MetavoxelEditMessage&, edit), Q_ARG(bool, reliable));
        return;
    }
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
        node->setLinkedData(createClient(node));
    }
}

MetavoxelClient* MetavoxelClientManager::createClient(const SharedNodePointer& node) {
    return new MetavoxelClient(node, this);
}

void MetavoxelClientManager::updateClient(MetavoxelClient* client) {
    client->update();
}

MetavoxelClient::MetavoxelClient(const SharedNodePointer& node, MetavoxelClientManager* manager) :
    Endpoint(node, new PacketRecord(), new PacketRecord()),
    _manager(manager),
    _reliableDeltaChannel(NULL),
    _reliableDeltaID(0) {
    
    connect(_sequencer.getReliableInputChannel(RELIABLE_DELTA_CHANNEL_INDEX),
        SIGNAL(receivedMessage(const QVariant&, Bitstream&)), SLOT(handleMessage(const QVariant&, Bitstream&)));
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

void MetavoxelClient::handleMessage(const QVariant& message, Bitstream& in) {
    int userType = message.userType(); 
    if (userType == MetavoxelDeltaMessage::Type) {
        PacketRecord* receiveRecord = getLastAcknowledgedReceiveRecord();
        if (_reliableDeltaChannel) {    
            _remoteData.readDelta(receiveRecord->getData(), receiveRecord->getLOD(), in, _remoteDataLOD = _reliableDeltaLOD);
            _sequencer.getInputStream().persistReadMappings(in.getAndResetReadMappings());
            in.clearPersistentMappings();
            _reliableDeltaChannel = NULL;
        
        } else {
            _remoteData.readDelta(receiveRecord->getData(), receiveRecord->getLOD(), in,
                _remoteDataLOD = getLastAcknowledgedSendRecord()->getLOD());
            in.reset();
        }
        // copy to local and reapply local edits
        _data = _remoteData;
        foreach (const DatagramSequencer::HighPriorityMessage& message, _sequencer.getHighPriorityMessages()) {
            if (message.data.userType() == MetavoxelEditMessage::Type) {
                message.data.value<MetavoxelEditMessage>().apply(_data, _sequencer.getWeakSharedObjectHash());
            }
        }
    } else if (userType == MetavoxelDeltaPendingMessage::Type) {
        // check the id to make sure this is not a delta we've already processed
        int id = message.value<MetavoxelDeltaPendingMessage>().id;
        if (id > _reliableDeltaID) {
            _reliableDeltaID = id;
            _reliableDeltaChannel = _sequencer.getReliableInputChannel(RELIABLE_DELTA_CHANNEL_INDEX);
            _reliableDeltaChannel->getBitstream().copyPersistentMappings(_sequencer.getInputStream());
            _reliableDeltaLOD = getLastAcknowledgedSendRecord()->getLOD();
            PacketRecord* receiveRecord = getLastAcknowledgedReceiveRecord();
            _remoteDataLOD = receiveRecord->getLOD();
            _remoteData = receiveRecord->getData();
        }
    } else {
        Endpoint::handleMessage(message, in);
    }
}

PacketRecord* MetavoxelClient::maybeCreateSendRecord() const {
    return new PacketRecord(_reliableDeltaChannel ? _reliableDeltaLOD : _manager->getLOD());
}

PacketRecord* MetavoxelClient::maybeCreateReceiveRecord() const {
    return new PacketRecord(_remoteDataLOD, _remoteData);
}

