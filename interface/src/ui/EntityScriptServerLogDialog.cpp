//
//  EntityScriptServerLogDialog.cpp
//  interface/src/ui
//
//  Created by Clement Brisset on 1/31/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "EntityScriptServerLogDialog.h"

EntityScriptServerLogDialog::EntityScriptServerLogDialog(QWidget* parent) : BaseLogDialog(parent) {
    setWindowTitle("Entity Script Server Log");

    auto nodeList = DependencyManager::get<NodeList>();
    auto& packetReceiver = nodeList->getPacketReceiver();
    packetReceiver.registerListener(PacketType::EntityServerScriptLog, this, "handleEntityServerScriptLogPacket");

    QObject::connect(nodeList.data(), &NodeList::nodeActivated, this, &EntityScriptServerLogDialog::nodeActivated);
    QObject::connect(nodeList.data(), &NodeList::nodeKilled, this, &EntityScriptServerLogDialog::nodeKilled);

    QObject::connect(nodeList.data(), &NodeList::canRezChanged, this, &EntityScriptServerLogDialog::enableToEntityServerScriptLog);
    enableToEntityServerScriptLog(nodeList->getThisNodeCanRez());
}

EntityScriptServerLogDialog::~EntityScriptServerLogDialog() {
    enableToEntityServerScriptLog(false);
}
void EntityScriptServerLogDialog::enableToEntityServerScriptLog(bool enable) {
    auto nodeList = DependencyManager::get<NodeList>();

    if (auto node = nodeList->soloNodeOfType(NodeType::EntityScriptServer)) {
        auto packet = NLPacket::create(PacketType::EntityServerScriptLog, sizeof(bool), true);
        packet->writePrimitive(enable);
        nodeList->sendPacket(std::move(packet), *node);

        if (_subscribed != enable) {
            if (enable) {
                appendLogLine("====================== Subscribded to the Entity Script Server's log ======================");
            } else {
                appendLogLine("==================== Unsubscribded from the Entity Script Server's log ====================");
            }
        }
        _subscribed = enable;
    } else {
        qWarning() << "Entity Script Server not found";
    }
}

void EntityScriptServerLogDialog::handleEntityServerScriptLogPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode) {
    auto lines = QString::fromUtf8(message->readAll());
    QMetaObject::invokeMethod(this, "appendLogLine", Q_ARG(QString, lines));
}

void EntityScriptServerLogDialog::nodeActivated(SharedNodePointer activatedNode) {
    if (_subscribed && activatedNode->getType() == NodeType::EntityScriptServer) {
        _subscribed = false;
        enableToEntityServerScriptLog(DependencyManager::get<NodeList>()->getThisNodeCanRez());
    }
}

void EntityScriptServerLogDialog::nodeKilled(SharedNodePointer killedNode) {
    if (killedNode->getType() == NodeType::EntityScriptServer) {
        appendLogLine("====================== Connection to the Entity Script Server lost ======================");
    }
}
