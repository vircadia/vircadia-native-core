//
//  EntityScriptServerLogClient.cpp
//  interface/src
//
//  Created by Clement Brisset on 2/1/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "EntityScriptServerLogClient.h"

EntityScriptServerLogClient::EntityScriptServerLogClient() {
    auto nodeList = DependencyManager::get<NodeList>();
    auto& packetReceiver = nodeList->getPacketReceiver();
    packetReceiver.registerListener(PacketType::EntityServerScriptLog, this, "handleEntityServerScriptLogPacket");

    QObject::connect(nodeList.data(), &NodeList::nodeActivated, this, &EntityScriptServerLogClient::nodeActivated);
    QObject::connect(nodeList.data(), &NodeList::nodeKilled, this, &EntityScriptServerLogClient::nodeKilled);

    QObject::connect(nodeList.data(), &NodeList::canRezChanged, this, &EntityScriptServerLogClient::canRezChanged);
}

void EntityScriptServerLogClient::connectNotify(const QMetaMethod& signal) {
    // This needs to be delayed or "receivers()" will return completely inconsistent values
    QMetaObject::invokeMethod(this, "connectionsChanged", Qt::QueuedConnection);
}

void EntityScriptServerLogClient::disconnectNotify(const QMetaMethod& signal) {
    // This needs to be delayed or "receivers()" will return completely inconsistent values
    QMetaObject::invokeMethod(this, "connectionsChanged", Qt::QueuedConnection);
}

void EntityScriptServerLogClient::connectionsChanged() {
    auto numReceivers = receivers(SIGNAL(receivedNewLogLines(QString)));
    if (!_subscribed && numReceivers > 0) {
        enableToEntityServerScriptLog(DependencyManager::get<NodeList>()->getThisNodeCanRez());
    } else if (_subscribed && numReceivers == 0) {
        enableToEntityServerScriptLog(false);
    }
}

void EntityScriptServerLogClient::enableToEntityServerScriptLog(bool enable) {
    auto nodeList = DependencyManager::get<NodeList>();

    if (auto node = nodeList->soloNodeOfType(NodeType::EntityScriptServer)) {
        auto packet = NLPacket::create(PacketType::EntityServerScriptLog, sizeof(bool), true);
        packet->writePrimitive(enable);
        nodeList->sendPacket(std::move(packet), *node);

        if (_subscribed != enable) {
            if (enable) {
                emit receivedNewLogLines("====================== Subscribed to the Entity Script Server's log ======================");
            } else {
                emit receivedNewLogLines("==================== Unsubscribed from the Entity Script Server's log ====================");
            }
        }
        _subscribed = enable;
    }
}

void EntityScriptServerLogClient::handleEntityServerScriptLogPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode) {
    emit receivedNewLogLines(QString::fromUtf8(message->readAll()));
}

void EntityScriptServerLogClient::nodeActivated(SharedNodePointer activatedNode) {
    if (_subscribed && activatedNode->getType() == NodeType::EntityScriptServer) {
        _subscribed = false;
        enableToEntityServerScriptLog(DependencyManager::get<NodeList>()->getThisNodeCanRez());
    }
}

void EntityScriptServerLogClient::nodeKilled(SharedNodePointer killedNode) {
    if (killedNode->getType() == NodeType::EntityScriptServer) {
        emit receivedNewLogLines("====================== Connection to the Entity Script Server lost ======================");
    }
}

void EntityScriptServerLogClient::canRezChanged(bool canRez) {
    auto numReceivers = receivers(SIGNAL(receivedNewLogLines(QString)));
    if (numReceivers > 0) {
        enableToEntityServerScriptLog(canRez);
    }
}
