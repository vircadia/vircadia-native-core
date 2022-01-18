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

#include "MessagesMixer.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QJsonObject>
#include <QBuffer>
#include <LogHandler.h>
#include <MessagesClient.h>
#include <NodeList.h>
#include <udt/PacketHeaders.h>

const QString MESSAGES_MIXER_LOGGING_NAME = "messages-mixer";
const int MESSAGES_MIXER_RATE_LIMITER_INTERVAL = 1000; // 1 second

MessagesMixer::MessagesMixer(ReceivedMessage& message) : ThreadedAssignment(message)
{
    connect(DependencyManager::get<NodeList>().data(), &NodeList::nodeKilled, this, &MessagesMixer::nodeKilled);
    auto& packetReceiver = DependencyManager::get<NodeList>()->getPacketReceiver();
    packetReceiver.registerListener(PacketType::MessagesData,
        PacketReceiver::makeSourcedListenerReference<MessagesMixer>(this, &MessagesMixer::handleMessages));
    packetReceiver.registerListener(PacketType::MessagesSubscribe,
        PacketReceiver::makeSourcedListenerReference<MessagesMixer>(this, &MessagesMixer::handleMessagesSubscribe));
    packetReceiver.registerListener(PacketType::MessagesUnsubscribe,
        PacketReceiver::makeSourcedListenerReference<MessagesMixer>(this, &MessagesMixer::handleMessagesUnsubscribe));
}

void MessagesMixer::nodeKilled(SharedNodePointer killedNode) {
    for (auto& channel : _channelSubscribers) {
        channel.remove(killedNode->getUUID());
    }
}

void MessagesMixer::handleMessages(QSharedPointer<ReceivedMessage> receivedMessage, SharedNodePointer senderNode) {
    QString channel, message;
    QByteArray data;
    QUuid senderID;
    bool isText;
    auto senderUUID = senderNode->getUUID();
    MessagesClient::decodeMessagesPacket(receivedMessage, channel, isText, message, data, senderID);

    auto nodeList = DependencyManager::get<NodeList>();

    auto itr = _allSubscribers.find(senderUUID);
    if (itr == _allSubscribers.end()) {
        _allSubscribers[senderUUID] = 1;
    } else if (*itr >= _maxMessagesPerSecond) {
        return;
    } else {
        *itr += 1;
    }

    nodeList->eachMatchingNode(
        [&](const SharedNodePointer& node)->bool {
        return node->getActiveSocket() && _channelSubscribers[channel].contains(node->getUUID());
    },
        [&](const SharedNodePointer& node) {
        auto packetList = isText ? MessagesClient::encodeMessagesPacket(channel, message, senderID) :
                                   MessagesClient::encodeMessagesDataPacket(channel, data, senderID);
        nodeList->sendPacketList(std::move(packetList), *node);
    });
}

void MessagesMixer::handleMessagesSubscribe(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode) {
    auto senderUUID = senderNode->getUUID();
    QString channel = QString::fromUtf8(message->getMessage());

    _channelSubscribers[channel] << senderUUID;
}

void MessagesMixer::handleMessagesUnsubscribe(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode) {
    auto senderUUID = senderNode->getUUID();
    QString channel = QString::fromUtf8(message->getMessage());

    if (_channelSubscribers.contains(channel)) {
        _channelSubscribers[channel].remove(senderUUID);
    }
}

void MessagesMixer::sendStatsPacket() {
    QJsonObject statsObject, messagesMixerObject;

    // add stats for each listerner
    DependencyManager::get<NodeList>()->eachNode([&](const SharedNodePointer& node) {
        QJsonObject clientStats;
        clientStats[USERNAME_UUID_REPLACEMENT_STATS_KEY] = uuidStringWithoutCurlyBraces(node->getUUID());
        clientStats["outbound_kbps"] = node->getOutboundKbps();
        clientStats["inbound_kbps"] = node->getInboundKbps();
        messagesMixerObject[uuidStringWithoutCurlyBraces(node->getUUID())] = clientStats;
    });

    statsObject["messages"] = messagesMixerObject;
    ThreadedAssignment::addPacketStatsAndSendStatsPacket(statsObject);
}

void MessagesMixer::run() {
    auto nodeList = DependencyManager::get<NodeList>();
    nodeList->addSetOfNodeTypesToNodeInterestSet({ NodeType::Agent, NodeType::EntityScriptServer });
    DomainHandler& domainHandler = nodeList->getDomainHandler();
    connect(&domainHandler, &DomainHandler::settingsReceived, this, &MessagesMixer::domainSettingsRequestComplete);

    ThreadedAssignment::commonInit(MESSAGES_MIXER_LOGGING_NAME, NodeType::MessagesMixer);

    startMaxMessagesProcessor();
}

void MessagesMixer::domainSettingsRequestComplete() {
    auto nodeList = DependencyManager::get<NodeList>();

    // parse the settings to pull out the values we need
    parseDomainServerSettings(nodeList->getDomainHandler().getSettingsObject());
}

void MessagesMixer::parseDomainServerSettings(const QJsonObject& domainSettings) {
    const QString MESSAGES_MIXER_SETTINGS_KEY = "messages_mixer";
    QJsonObject messagesMixerGroupObject = domainSettings[MESSAGES_MIXER_SETTINGS_KEY].toObject();

    const QString NODE_MESSAGES_PER_SECOND_KEY = "max_node_messages_per_second";
    QJsonValue maxMessagesPerSecondValue = messagesMixerGroupObject.value(NODE_MESSAGES_PER_SECOND_KEY);
    _maxMessagesPerSecond = maxMessagesPerSecondValue.toInt(DEFAULT_NODE_MESSAGES_PER_SECOND);
}

void MessagesMixer::processMaxMessagesContainer() {
    _allSubscribers.clear();
}

void MessagesMixer::startMaxMessagesProcessor() {
    if (_maxMessagesTimer) {
        stopMaxMessagesProcessor();
    }

    _maxMessagesTimer = new QTimer();
    connect(_maxMessagesTimer, &QTimer::timeout, this, &MessagesMixer::processMaxMessagesContainer);
    _maxMessagesTimer->start(MESSAGES_MIXER_RATE_LIMITER_INTERVAL); // Clear the container every second.
}

void MessagesMixer::stopMaxMessagesProcessor() {
    _maxMessagesTimer->stop();
    _maxMessagesTimer->deleteLater();
    _maxMessagesTimer = nullptr;
}
