//
//  MessagesClient.cpp
//  libraries/networking/src
//
//  Created by Brad hefta-Gaub on 11/16/2015.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "MessagesClient.h"

#include <cstdint>

#include <QtCore/QBuffer>
#include <QtCore/QThread>

#include "NetworkLogging.h"
#include "NodeList.h"
#include "PacketReceiver.h"

MessagesClient::MessagesClient() {
    setCustomDeleter([](Dependency* dependency){
        static_cast<MessagesClient*>(dependency)->deleteLater();
    });
    auto nodeList = DependencyManager::get<NodeList>();
    auto& packetReceiver = nodeList->getPacketReceiver();
    packetReceiver.registerMessageListener(PacketType::MessagesData, this, "handleMessagesPacket");
    connect(nodeList.data(), &LimitedNodeList::nodeKilled, this, &MessagesClient::handleNodeKilled);
}

void MessagesClient::init() {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "init", Qt::BlockingQueuedConnection);
    }
}

void MessagesClient::handleMessagesPacket(QSharedPointer<NLPacketList> packetList, SharedNodePointer senderNode) {
    QByteArray packetData = packetList->getMessage();
    QBuffer packet{ &packetData };
    packet.open(QIODevice::ReadOnly);

    quint16 channelLength;
    packet.read(reinterpret_cast<char*>(&channelLength), sizeof(channelLength));
    auto channelData = packet.read(channelLength);
    QString channel = QString::fromUtf8(channelData);

    quint16 messageLength;
    packet.read(reinterpret_cast<char*>(&messageLength), sizeof(messageLength));
    auto messageData = packet.read(messageLength);
    QString message = QString::fromUtf8(messageData);

    emit messageReceived(channel, message);
}

void MessagesClient::sendMessage(const QString& channel, const QString& message) {
    auto nodeList = DependencyManager::get<NodeList>();
    SharedNodePointer messagesMixer = nodeList->soloNodeOfType(NodeType::MessagesMixer);
    
    if (messagesMixer) {
        auto packetList = NLPacketList::create(PacketType::MessagesData, QByteArray(), true, true);

        auto channelUtf8 = channel.toUtf8();
        quint16 channelLength = channelUtf8.length();
        packetList->writePrimitive(channelLength);
        packetList->write(channelUtf8);

        auto messageUtf8 = message.toUtf8();
        quint16 messageLength = messageUtf8.length();
        packetList->writePrimitive(messageLength);
        packetList->write(messageUtf8);

        nodeList->sendPacketList(std::move(packetList), *messagesMixer);
    }
}

// FIXME - we should keep track of the channels we are subscribed to locally, and
// in the event that they mixer goes away and/or comes back we should automatically
// resubscribe to those channels
void MessagesClient::subscribe(const QString& channel) {
    auto nodeList = DependencyManager::get<NodeList>();
    SharedNodePointer messagesMixer = nodeList->soloNodeOfType(NodeType::MessagesMixer);

    if (messagesMixer) {
        auto packetList = NLPacketList::create(PacketType::MessagesSubscribe, QByteArray(), true, true);
        packetList->write(channel.toUtf8());
        nodeList->sendPacketList(std::move(packetList), *messagesMixer);
    }
}

void MessagesClient::unsubscribe(const QString& channel) {
    auto nodeList = DependencyManager::get<NodeList>();
    SharedNodePointer messagesMixer = nodeList->soloNodeOfType(NodeType::MessagesMixer);

    if (messagesMixer) {
        auto packetList = NLPacketList::create(PacketType::MessagesUnsubscribe, QByteArray(), true, true);
        packetList->write(channel.toUtf8());
        nodeList->sendPacketList(std::move(packetList), *messagesMixer);
    }
}

void MessagesClient::handleNodeKilled(SharedNodePointer node) {
    if (node->getType() != NodeType::MessagesMixer) {
        return;
    }
    // FIXME - do we need to do any special bookkeeping for when the messages mixer is no longer available
}
