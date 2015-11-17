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
#include <NodeList.h>
#include <udt/PacketHeaders.h>

#include "MessagesMixer.h"

const QString MESSAGES_MIXER_LOGGING_NAME = "messages-mixer";

MessagesMixer::MessagesMixer(NLPacket& packet) :
    ThreadedAssignment(packet)
{
    // make sure we hear about node kills so we can tell the other nodes
    connect(DependencyManager::get<NodeList>().data(), &NodeList::nodeKilled, this, &MessagesMixer::nodeKilled);

    auto& packetReceiver = DependencyManager::get<NodeList>()->getPacketReceiver();
    packetReceiver.registerMessageListener(PacketType::MessagesData, this, "handleMessages");
    packetReceiver.registerMessageListener(PacketType::MessagesSubscribe, this, "handleMessagesSubscribe");
    packetReceiver.registerMessageListener(PacketType::MessagesUnsubscribe, this, "handleMessagesUnsubscribe");
}

MessagesMixer::~MessagesMixer() {
}

void MessagesMixer::nodeKilled(SharedNodePointer killedNode) {
    // FIXME - remove the node from the subscription maps
}

void MessagesMixer::handleMessages(QSharedPointer<NLPacketList> packetList, SharedNodePointer senderNode) {
    Q_ASSERT(packetList->getType() == PacketType::MessagesData);

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
    
    auto nodeList = DependencyManager::get<NodeList>();

    nodeList->eachMatchingNode(
        [&](const SharedNodePointer& node)->bool {

        return node->getType() == NodeType::Agent && node->getActiveSocket() &&
                _channelSubscribers[channel].contains(node->getUUID());
    },
        [&](const SharedNodePointer& node) {

        auto packetList = NLPacketList::create(PacketType::MessagesData, QByteArray(), true, true);

        auto channelUtf8 = channel.toUtf8();
        quint16 channelLength = channelUtf8.length();
        packetList->writePrimitive(channelLength);
        packetList->write(channelUtf8);

        auto messageUtf8 = message.toUtf8();
        quint16 messageLength = messageUtf8.length();
        packetList->writePrimitive(messageLength);
        packetList->write(messageUtf8);

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

        // add the key to ask the domain-server for a username replacement, if it has it
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

    // wait until we have the domain-server settings, otherwise we bail
    DomainHandler& domainHandler = nodeList->getDomainHandler();

    qDebug() << "Waiting for domain settings from domain-server.";

    // block until we get the settingsRequestComplete signal
    QEventLoop loop;
    connect(&domainHandler, &DomainHandler::settingsReceived, &loop, &QEventLoop::quit);
    connect(&domainHandler, &DomainHandler::settingsReceiveFail, &loop, &QEventLoop::quit);
    domainHandler.requestDomainSettings();
    loop.exec();

    if (domainHandler.getSettingsObject().isEmpty()) {
        qDebug() << "Failed to retreive settings object from domain-server. Bailing on assignment.";
        setFinished(true);
        return;
    }

    // parse the settings to pull out the values we need
    parseDomainServerSettings(domainHandler.getSettingsObject());
}

void MessagesMixer::parseDomainServerSettings(const QJsonObject& domainSettings) {
    // TODO - if we want options, parse them here...
    const QString MESSAGES_MIXER_SETTINGS_KEY = "messages_mixer";
}
