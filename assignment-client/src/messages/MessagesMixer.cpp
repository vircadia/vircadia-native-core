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

MessagesMixer::MessagesMixer(NLPacket& packet) :
    ThreadedAssignment(packet)
{
    connect(DependencyManager::get<NodeList>().data(), &NodeList::nodeKilled, this, &MessagesMixer::nodeKilled);
    auto& packetReceiver = DependencyManager::get<NodeList>()->getPacketReceiver();
    packetReceiver.registerMessageListener(PacketType::MessagesData, this, "handleMessages");
    packetReceiver.registerMessageListener(PacketType::MessagesSubscribe, this, "handleMessagesSubscribe");
    packetReceiver.registerMessageListener(PacketType::MessagesUnsubscribe, this, "handleMessagesUnsubscribe");
}

MessagesMixer::~MessagesMixer() {
}

void MessagesMixer::nodeKilled(SharedNodePointer killedNode) {
    for (auto& channel : _channelSubscribers) {
        channel.remove(killedNode->getUUID());
    }
}

void MessagesMixer::handleMessages(QSharedPointer<NLPacketList> packetList, SharedNodePointer senderNode) {
    Q_ASSERT(packetList->getType() == PacketType::MessagesData);

    QString channel, message;
    QUuid senderID;
    MessagesClient::decodeMessagesPacket(packetList, channel, message, senderID);

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

void MessagesMixer::handleMessagesSubscribe(QSharedPointer<NLPacketList> packetList, SharedNodePointer senderNode) {
    Q_ASSERT(packetList->getType() == PacketType::MessagesSubscribe);
    QString channel = QString::fromUtf8(packetList->getMessage());
    qDebug() << "Node [" << senderNode->getUUID() << "] subscribed to channel:" << channel;
    _channelSubscribers[channel] << senderNode->getUUID();
}

void MessagesMixer::handleMessagesUnsubscribe(QSharedPointer<NLPacketList> packetList, SharedNodePointer senderNode) {
    Q_ASSERT(packetList->getType() == PacketType::MessagesUnsubscribe);
    QString channel = QString::fromUtf8(packetList->getMessage());
    qDebug() << "Node [" << senderNode->getUUID() << "] unsubscribed from channel:" << channel;

    if (_channelSubscribers.contains(channel)) {
        _channelSubscribers[channel].remove(senderNode->getUUID());
    }
}

// FIXME - make these stats relevant
void MessagesMixer::sendStatsPacket() {
    QJsonObject statsObject;
    QJsonObject messagesObject;
    auto nodeList = DependencyManager::get<NodeList>();

    // add stats for each listerner
    nodeList->eachNode([&](const SharedNodePointer& node) {
        QJsonObject messagesStats;
        messagesStats[USERNAME_UUID_REPLACEMENT_STATS_KEY] = uuidStringWithoutCurlyBraces(node->getUUID());
        messagesStats["outbound_kbps"] = node->getOutboundBandwidth();
        messagesStats["inbound_kbps"] = node->getInboundBandwidth();
        messagesObject[uuidStringWithoutCurlyBraces(node->getUUID())] = messagesStats;
    });

    statsObject["messages"] = messagesObject;
    ThreadedAssignment::addPacketStatsAndSendStatsPacket(statsObject);
}

void MessagesMixer::run() {
    ThreadedAssignment::commonInit(MESSAGES_MIXER_LOGGING_NAME, NodeType::MessagesMixer);
    auto nodeList = DependencyManager::get<NodeList>();
    nodeList->addNodeTypeToInterestSet(NodeType::Agent);
}
