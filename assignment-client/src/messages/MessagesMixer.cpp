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

#include <LogHandler.h>
#include <NodeList.h>
#include <udt/PacketHeaders.h>
#include <SharedUtil.h>
#include <UUID.h>
#include <TryLocker.h>

#include "MessagesMixerClientData.h"
#include "MessagesMixer.h"

const QString MESSAGES_MIXER_LOGGING_NAME = "messages-mixer";

const int MESSAGES_MIXER_BROADCAST_FRAMES_PER_SECOND = 60;
const unsigned int MESSAGES_DATA_SEND_INTERVAL_MSECS = (1.0f / (float) MESSAGES_MIXER_BROADCAST_FRAMES_PER_SECOND) * 1000;

MessagesMixer::MessagesMixer(NLPacket& packet) :
    ThreadedAssignment(packet),
    _broadcastThread(),
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
    packetReceiver.registerListener(PacketType::MessagesData, this, "handleMessagesPacket");
    packetReceiver.registerMessageListener(PacketType::MessagesData, this, "handleMessagesPacketList");
}

MessagesMixer::~MessagesMixer() {
    if (_broadcastTimer) {
        _broadcastTimer->deleteLater();
    }

    _broadcastThread.quit();
    _broadcastThread.wait();
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

void MessagesMixer::handleMessagesPacketList(QSharedPointer<NLPacketList> packetList, SharedNodePointer senderNode) {
    qDebug() << "MessagesMixer::handleMessagesPacketList()... senderNode:" << senderNode->getUUID();

    auto nodeList = DependencyManager::get<NodeList>();
    //nodeList->updateNodeWithDataFromPacket(packet, senderNode);

    QByteArray data = packetList->getMessage();
    auto packetType = packetList->getType();

    if (packetType == PacketType::MessagesData) {
        QString message = QString::fromUtf8(data);
        qDebug() << "got a messages packet:" << message;

        // this was an avatar we were sending to other people
        // send a kill packet for it to our other nodes
        //auto killPacket = NLPacket::create(PacketType::KillAvatar, NUM_BYTES_RFC4122_UUID);
        //killPacket->write(killedNode->getUUID().toRfc4122());
        //nodeList->broadcastToNodes(std::move(killPacket), NodeSet() << NodeType::Agent);

    }
}

void MessagesMixer::handleMessagesPacket(QSharedPointer<NLPacket> packet, SharedNodePointer sendingNode) {
    qDebug() << "MessagesMixer::handleMessagesPacket()... senderNode:" << sendingNode->getUUID();

    /*
    auto nodeList = DependencyManager::get<NodeList>();
    //nodeList->updateNodeWithDataFromPacket(packet, senderNode);

    QByteArray data = packetList->getMessage();
    auto packetType = packetList->getType();

    if (packetType == PacketType::MessagesData) {
        QString message = QString::fromUtf8(data);
        qDebug() << "got a messages packet:" << message;

        // this was an avatar we were sending to other people
        // send a kill packet for it to our other nodes
        //auto killPacket = NLPacket::create(PacketType::KillAvatar, NUM_BYTES_RFC4122_UUID);
        //killPacket->write(killedNode->getUUID().toRfc4122());
        //nodeList->broadcastToNodes(std::move(killPacket), NodeSet() << NodeType::Agent);

    }
    */
}

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

        MessagesMixerClientData* clientData = static_cast<MessagesMixerClientData*>(node->getLinkedData());
        if (clientData) {
            MutexTryLocker lock(clientData->getMutex());
            if (lock.isLocked()) {
                clientData->loadJSONStats(messagesStats);

                // add the diff between the full outbound bandwidth and the measured bandwidth for AvatarData send only
                messagesStats["delta_full_vs_avatar_data_kbps"] =
                    messagesStats[NODE_OUTBOUND_KBPS_STAT_KEY].toDouble() - messagesStats[OUTBOUND_MESSAGES_DATA_STATS_KEY].toDouble();
            }
        }

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
        node->setLinkedData(new MessagesMixerClientData());
    };

    /*
    // setup the timer that will be fired on the broadcast thread
    _broadcastTimer = new QTimer;
    _broadcastTimer->setInterval(MESSAGES_DATA_SEND_INTERVAL_MSECS);
    _broadcastTimer->moveToThread(&_broadcastThread);

    // connect appropriate signals and slots
    connect(_broadcastTimer, &QTimer::timeout, this, &MessagesMixer::broadcastMessagesData, Qt::DirectConnection);
    connect(&_broadcastThread, SIGNAL(started()), _broadcastTimer, SLOT(start()));
    */

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

    // start the broadcastThread
    //_broadcastThread.start();
}

void MessagesMixer::parseDomainServerSettings(const QJsonObject& domainSettings) {
    qDebug() << "MessagesMixer::parseDomainServerSettings() domainSettings:" << domainSettings;
    const QString MESSAGES_MIXER_SETTINGS_KEY = "messages_mixer";
    const QString NODE_SEND_BANDWIDTH_KEY = "max_node_send_bandwidth";

    const float DEFAULT_NODE_SEND_BANDWIDTH = 1.0f;
    QJsonValue nodeBandwidthValue = domainSettings[MESSAGES_MIXER_SETTINGS_KEY].toObject()[NODE_SEND_BANDWIDTH_KEY];
    if (!nodeBandwidthValue.isDouble()) {
        qDebug() << NODE_SEND_BANDWIDTH_KEY << "is not a double - will continue with default value";
    }

    _maxKbpsPerNode = nodeBandwidthValue.toDouble(DEFAULT_NODE_SEND_BANDWIDTH) * KILO_PER_MEGA;
    qDebug() << "The maximum send bandwidth per node is" << _maxKbpsPerNode << "kbps.";
}
