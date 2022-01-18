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

#include <ThreadHelpers.h>

#include "NetworkLogging.h"
#include "NodeList.h"
#include "PacketReceiver.h"

MessagesClient::MessagesClient() {
    setCustomDeleter([](Dependency* dependency){
        static_cast<MessagesClient*>(dependency)->deleteLater();
    });
    auto nodeList = DependencyManager::get<NodeList>();
    auto& packetReceiver = nodeList->getPacketReceiver();
    packetReceiver.registerListener(PacketType::MessagesData,  
        PacketReceiver::makeSourcedListenerReference<MessagesClient>(this, &MessagesClient::handleMessagesPacket));
    connect(nodeList.data(), &LimitedNodeList::nodeActivated, this, &MessagesClient::handleNodeActivated);
}

void MessagesClient::decodeMessagesPacket(QSharedPointer<ReceivedMessage> receivedMessage, QString& channel, 
                                                bool& isText, QString& message, QByteArray& data, QUuid& senderID) {
    quint16 channelLength;
    receivedMessage->readPrimitive(&channelLength);
    auto channelData = receivedMessage->read(channelLength);
    channel = QString::fromUtf8(channelData);

    receivedMessage->readPrimitive(&isText);

    quint32 messageLength;
    receivedMessage->readPrimitive(&messageLength);
    auto messageData = receivedMessage->read(messageLength);
    if (isText) {
        message = QString::fromUtf8(messageData);
    } else {
        data = messageData;
    }

    QByteArray bytesSenderID = receivedMessage->read(NUM_BYTES_RFC4122_UUID);
    if (bytesSenderID.length() == NUM_BYTES_RFC4122_UUID) {
        senderID = QUuid::fromRfc4122(bytesSenderID);
    } else {
        QUuid emptyUUID;
        senderID = emptyUUID; // packet was missing UUID use default instead
    }
}

std::unique_ptr<NLPacketList> MessagesClient::encodeMessagesPacket(QString channel, QString message, QUuid senderID) {
    auto packetList = NLPacketList::create(PacketType::MessagesData, QByteArray(), true, true);

    auto channelUtf8 = channel.toUtf8();
    quint16 channelLength = channelUtf8.length();
    packetList->writePrimitive(channelLength);
    packetList->write(channelUtf8);

    bool isTextMessage = true;
    packetList->writePrimitive(isTextMessage);

    auto messageUtf8 = message.toUtf8();
    quint32 messageLength = messageUtf8.length();
    packetList->writePrimitive(messageLength);
    packetList->write(messageUtf8);

    packetList->write(senderID.toRfc4122());

    return packetList;
}

std::unique_ptr<NLPacketList> MessagesClient::encodeMessagesDataPacket(QString channel, QByteArray data, QUuid senderID) {
    auto packetList = NLPacketList::create(PacketType::MessagesData, QByteArray(), true, true);

    auto channelUtf8 = channel.toUtf8();
    quint16 channelLength = channelUtf8.length();
    packetList->writePrimitive(channelLength);
    packetList->write(channelUtf8);

    bool isTextMessage = false;
    packetList->writePrimitive(isTextMessage);

    quint32 dataLength = data.length();
    packetList->writePrimitive(dataLength);
    packetList->write(data);

    packetList->write(senderID.toRfc4122());

    return packetList;
}


void MessagesClient::handleMessagesPacket(QSharedPointer<ReceivedMessage> receivedMessage, SharedNodePointer senderNode) {
    QString channel, message;
    QByteArray data;
    bool isText { false };
    QUuid senderID;
    decodeMessagesPacket(receivedMessage, channel, isText, message, data, senderID);
    if (isText) {
        emit messageReceived(channel, message, senderID, false);
    } else {
        emit dataReceived(channel, data, senderID, false);
    }
}

void MessagesClient::sendMessage(QString channel, QString message, bool localOnly) {
    auto nodeList = DependencyManager::get<NodeList>();
    QUuid senderID = nodeList->getSessionUUID();
    if (localOnly) {
        emit messageReceived(channel, message, senderID, true);
    } else {
        SharedNodePointer messagesMixer = nodeList->soloNodeOfType(NodeType::MessagesMixer);
        if (messagesMixer) {
            auto packetList = encodeMessagesPacket(channel, message, senderID);
            nodeList->sendPacketList(std::move(packetList), *messagesMixer);
        } else {
            emit messageReceived(channel, message, senderID, true);
        }
    }
}

void MessagesClient::sendData(QString channel, QByteArray data, bool localOnly) {
    auto nodeList = DependencyManager::get<NodeList>();
    QUuid senderID = nodeList->getSessionUUID();
    if (localOnly) {
        emit dataReceived(channel, data, senderID, true);
    } else {
        SharedNodePointer messagesMixer = nodeList->soloNodeOfType(NodeType::MessagesMixer);
        if (messagesMixer) {
            QUuid senderID = nodeList->getSessionUUID();
            auto packetList = encodeMessagesDataPacket(channel, data, senderID);
            nodeList->sendPacketList(std::move(packetList), *messagesMixer);
        } else {
            emit dataReceived(channel, data, senderID, true);
        }
    }
}

void MessagesClient::sendLocalMessage(QString channel, QString message) {
    sendMessage(channel, message, true);
}

void MessagesClient::subscribe(QString channel) {
    _subscribedChannels << channel;
    auto nodeList = DependencyManager::get<NodeList>();
    SharedNodePointer messagesMixer = nodeList->soloNodeOfType(NodeType::MessagesMixer);

    if (messagesMixer) {
        auto packetList = NLPacketList::create(PacketType::MessagesSubscribe, QByteArray(), true, true);
        packetList->write(channel.toUtf8());
        nodeList->sendPacketList(std::move(packetList), *messagesMixer);
    }
}

void MessagesClient::unsubscribe(QString channel) {
    _subscribedChannels.remove(channel);
    auto nodeList = DependencyManager::get<NodeList>();
    SharedNodePointer messagesMixer = nodeList->soloNodeOfType(NodeType::MessagesMixer);

    if (messagesMixer) {
        auto packetList = NLPacketList::create(PacketType::MessagesUnsubscribe, QByteArray(), true, true);
        packetList->write(channel.toUtf8());
        nodeList->sendPacketList(std::move(packetList), *messagesMixer);
    }
}

void MessagesClient::handleNodeActivated(SharedNodePointer node) {
    if (node->getType() == NodeType::MessagesMixer) {
        for (const auto& channel : _subscribedChannels) {
            subscribe(channel);
        }
    }
}

void MessagesClient::startThread() {
    moveToNewNamedThread(this, "Messages Client Thread");
}
