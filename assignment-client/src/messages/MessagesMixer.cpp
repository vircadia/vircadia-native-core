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

#include <cfloat>
#include <random>

#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtCore/QJsonObject>
#include <QtCore/QTimer>
#include <QtCore/QThread>
#include <QBuffer>

#include <LogHandler.h>
#include <NodeList.h>
#include <udt/PacketHeaders.h>
#include <SharedUtil.h>
#include <UUID.h>
#include <TryLocker.h>

#include "MessagesMixer.h"

const QString MESSAGES_MIXER_LOGGING_NAME = "messages-mixer";

const int MESSAGES_MIXER_BROADCAST_FRAMES_PER_SECOND = 60;
const unsigned int MESSAGES_DATA_SEND_INTERVAL_MSECS = (1.0f / (float) MESSAGES_MIXER_BROADCAST_FRAMES_PER_SECOND) * 1000;

MessagesMixer::MessagesMixer(NLPacket& packet) :
    ThreadedAssignment(packet),
    _lastFrameTimestamp(QDateTime::currentMSecsSinceEpoch()),
    _trailingSleepRatio(1.0f),
    _performanceThrottlingRatio(0.0f),
    _sumListeners(0),
    _numStatFrames(0),
    _sumBillboardPackets(0),
    _sumIdentityPackets(0)
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

// An 80% chance of sending a identity packet within a 5 second interval.
// assuming 60 htz update rate.
const float BILLBOARD_AND_IDENTITY_SEND_PROBABILITY = 1.0f / 187.0f;

void MessagesMixer::nodeKilled(SharedNodePointer killedNode) {
    qDebug() << "MessagesMixer::nodeKilled()... node:" << killedNode->getUUID();

    if (killedNode->getType() == NodeType::Agent
        && killedNode->getLinkedData()) {
        auto nodeList = DependencyManager::get<NodeList>();

        // we also want to remove sequence number data for this avatar on our other avatars
        // so invoke the appropriate method on the MessagesMixerClientData for other avatars
        nodeList->eachMatchingNode(
            [&](const SharedNodePointer& node)->bool {
                if (!node->getLinkedData()) {
                    return false;
                }

                if (node->getUUID() == killedNode->getUUID()) {
                    return false;
                }

                return true;
            },
            [&](const SharedNodePointer& node) {
                qDebug() << "eachMatchingNode()... node:" << node->getUUID();
            }
        );
    }
}

void MessagesMixer::handleMessages(QSharedPointer<NLPacketList> packetList, SharedNodePointer senderNode) {
    Q_ASSERT(packetList->getType() == PacketType::MessagesData);

    qDebug() << "MessagesMixer::handleMessages()... senderNode:" << senderNode->getUUID();

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

    qDebug() << "got a messages:" << message << "on channel:" << channel << "from node:" << senderNode->getUUID();

    // this was an avatar we were sending to other people
    // send a kill packet for it to our other nodes
    //auto killPacket = NLPacket::create(PacketType::KillAvatar, NUM_BYTES_RFC4122_UUID);
    //killPacket->write(killedNode->getUUID().toRfc4122());
    //nodeList->broadcastToNodes(std::move(killPacket), NodeSet() << NodeType::Agent);
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
    statsObject["average_listeners_last_second"] = (float) _sumListeners / (float) _numStatFrames;
    statsObject["trailing_sleep_percentage"] = _trailingSleepRatio * 100;
    statsObject["performance_throttling_ratio"] = _performanceThrottlingRatio;

    QJsonObject messagesObject;

    auto nodeList = DependencyManager::get<NodeList>();
    // add stats for each listerner
    nodeList->eachNode([&](const SharedNodePointer& node) {
        QJsonObject messagesStats;

        const QString NODE_OUTBOUND_KBPS_STAT_KEY = "outbound_kbps";
        const QString NODE_INBOUND_KBPS_STAT_KEY = "inbound_kbps";

        // add the key to ask the domain-server for a username replacement, if it has it
        messagesStats[USERNAME_UUID_REPLACEMENT_STATS_KEY] = uuidStringWithoutCurlyBraces(node->getUUID());
        messagesStats[NODE_OUTBOUND_KBPS_STAT_KEY] = node->getOutboundBandwidth();
        messagesStats[NODE_INBOUND_KBPS_STAT_KEY] = node->getInboundBandwidth();

        messagesObject[uuidStringWithoutCurlyBraces(node->getUUID())] = messagesStats;
    });

    statsObject["messages"] = messagesObject;
    ThreadedAssignment::addPacketStatsAndSendStatsPacket(statsObject);

    _sumListeners = 0;
    _numStatFrames = 0;
}

void MessagesMixer::run() {
    ThreadedAssignment::commonInit(MESSAGES_MIXER_LOGGING_NAME, NodeType::MessagesMixer);

    NodeType_t owningNodeType = DependencyManager::get<NodeList>()->getOwnerType();
    qDebug() << "owningNodeType:" << owningNodeType;

    auto nodeList = DependencyManager::get<NodeList>();
    nodeList->addNodeTypeToInterestSet(NodeType::Agent);

    nodeList->linkedDataCreateCallback = [] (Node* node) {
        // no need to link data
    };

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
    qDebug() << "MessagesMixer::parseDomainServerSettings() domainSettings:" << domainSettings;
    const QString MESSAGES_MIXER_SETTINGS_KEY = "messages_mixer";

    // TODO - if we want options, parse them here...
    //
    // QJsonValue nodeBandwidthValue = domainSettings[MESSAGES_MIXER_SETTINGS_KEY].toObject()[NODE_SEND_BANDWIDTH_KEY];
    // if (!nodeBandwidthValue.isDouble()) {
    //     qDebug() << NODE_SEND_BANDWIDTH_KEY << "is not a double - will continue with default value";
    // }
}
