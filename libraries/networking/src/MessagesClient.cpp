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
#include <QtCore/QStandardPaths>
#include <QtCore/QThread>
#include <QtNetwork/QNetworkDiskCache>

#include "AssetRequest.h"
#include "AssetUpload.h"
#include "AssetUtils.h"
#include "NetworkAccessManager.h"
#include "NetworkLogging.h"
#include "NodeList.h"
#include "PacketReceiver.h"
#include "ResourceCache.h"

MessagesClient::MessagesClient() {
    
    setCustomDeleter([](Dependency* dependency){
        static_cast<MessagesClient*>(dependency)->deleteLater();
    });
    
    auto nodeList = DependencyManager::get<NodeList>();
    auto& packetReceiver = nodeList->getPacketReceiver();

    packetReceiver.registerListener(PacketType::MessagesData, this, "handleMessagePacket");

    connect(nodeList.data(), &LimitedNodeList::nodeKilled, this, &MessagesClient::handleNodeKilled);
}

void MessagesClient::init() {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "init", Qt::BlockingQueuedConnection);
    }
    
    // Setup disk cache if not already
    QNetworkAccessManager& networkAccessManager = NetworkAccessManager::getInstance();
    if (!networkAccessManager.cache()) {
        QString cachePath = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
        cachePath = !cachePath.isEmpty() ? cachePath : "interfaceCache";
        
        QNetworkDiskCache* cache = new QNetworkDiskCache();
        cache->setMaximumCacheSize(MAXIMUM_CACHE_SIZE);
        cache->setCacheDirectory(cachePath);
        networkAccessManager.setCache(cache);
        qCDebug(asset_client) << "MessagesClient disk cache setup at" << cachePath
                                << "(size:" << MAXIMUM_CACHE_SIZE / BYTES_PER_GIGABYTES << "GB)";
    }
}

bool haveMessagesMixer() {
    auto nodeList = DependencyManager::get<NodeList>();
    SharedNodePointer messagesMixer = nodeList->soloNodeOfType(NodeType::MessagesMixer);
    
    if (!messagesMixer) {
        qCWarning(messages_client) << "Could not complete MessagesClient operation "
            << "since you are not currently connected to a messages-mixer.";
        return false;
    }
    
    return true;
}

void MessagesClient::handleMessagesPacket(QSharedPointer<NLPacketList> packetList, SharedNodePointer senderNode) {
    QByteArray data = packetList->getMessage();
    auto packetType = packetList->getType();

    if (packetType == PacketType::MessagesData) {
        QString message = QString::fromUtf8(data);
        qDebug() << "got a messages packet:" << message;
    }
}

void MessagesClient::sendMessage(const QString& channel, const QString& message) {
    qDebug() << "MessagesClient::sendMessage() channel:" << channel << "message:" << message;
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

void MessagesClient::handleNodeKilled(SharedNodePointer node) {
    if (node->getType() != NodeType::MessagesMixer) {
        return;
    }

}
