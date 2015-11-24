//
//  MessagesMixer.cpp
//  assignment-client/src/messages
//
//  Created by Brad hefta-Gaub on 11/16/2015.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtCore/QCoreApplication>
#include <QtCore/QJsonObject>
#include <QBuffer>
#include <LogHandler.h>
#include <MessagesClient.h>
#include <NodeList.h>
#include <udt/PacketHeaders.h>
#include "MessagesMixer.h"

const QString MESSAGES_MIXER_LOGGING_NAME = "messages-mixer";

MessagesMixer::MessagesMixer(ReceivedMessage& message) : ThreadedAssignment(message)
{
    connect(DependencyManager::get<NodeList>().data(), &NodeList::nodeKilled, this, &MessagesMixer::nodeKilled);
    auto& packetReceiver = DependencyManager::get<NodeList>()->getPacketReceiver();
    packetReceiver.registerListener(PacketType::MessagesData, this, "handleMessages");
    packetReceiver.registerListener(PacketType::MessagesSubscribe, this, "handleMessagesSubscribe");
    packetReceiver.registerListener(PacketType::MessagesUnsubscribe, this, "handleMessagesUnsubscribe");
}

void MessagesMixer::nodeKilled(SharedNodePointer killedNode) {
    for (auto& channel : _channelSubscribers) {
        channel.remove(killedNode->getUUID());
    }
}

void MessagesMixer::handleMessages(QSharedPointer<ReceivedMessage> receivedMessage, SharedNodePointer senderNode) {
    QString channel, message;
    QUuid senderID;
    MessagesClient::decodeMessagesPacket(receivedMessage, channel, message, senderID);

    auto nodeList = DependencyManager::get<NodeList>();

    nodeList->eachMatchingNode(
        [&](const SharedNodePointer& node)->bool {
        return node->getType() == NodeType::Agent && node->getActiveSocket() &&
                _channelSubscribers[channel].contains(node->getUUID());
    },
        [&](const SharedNodePointer& node) {
        auto packetList = MessagesClient::encodeMessagesPacket(channel, message, senderID);
        nodeList->sendPacketList(std::move(packetList), *node);
    });
}

void MessagesMixer::handleMessagesSubscribe(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode) {
    QString channel = QString::fromUtf8(message->getMessage());
    _channelSubscribers[channel] << senderNode->getUUID();
}

void MessagesMixer::handleMessagesUnsubscribe(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode) {
    QString channel = QString::fromUtf8(message->getMessage());
    if (_channelSubscribers.contains(channel)) {
        _channelSubscribers[channel].remove(senderNode->getUUID());
    }
}

void MessagesMixer::sendStatsPacket() {
    QJsonObject statsObject, messagesMixerObject;

    // add stats for each listerner
    DependencyManager::get<NodeList>()->eachNode([&](const SharedNodePointer& node) {
        QJsonObject clientStats;
        clientStats[USERNAME_UUID_REPLACEMENT_STATS_KEY] = uuidStringWithoutCurlyBraces(node->getUUID());
        clientStats["outbound_kbps"] = node->getOutboundBandwidth();
        clientStats["inbound_kbps"] = node->getInboundBandwidth();
        messagesMixerObject[uuidStringWithoutCurlyBraces(node->getUUID())] = clientStats;
    });

    statsObject["messages"] = messagesMixerObject;
    ThreadedAssignment::addPacketStatsAndSendStatsPacket(statsObject);
}

void MessagesMixer::run() {
    ThreadedAssignment::commonInit(MESSAGES_MIXER_LOGGING_NAME, NodeType::MessagesMixer);
    DependencyManager::get<NodeList>()->addNodeTypeToInterestSet(NodeType::Agent);
}