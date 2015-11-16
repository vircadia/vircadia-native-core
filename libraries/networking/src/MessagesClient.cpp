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

void MessagesClient::handleMessagesPacket(QSharedPointer<NLPacket> packet, SharedNodePointer senderNode) {
    auto packetType = packet->getType();

    if (packetType == PacketType::MessagesData) {
        qDebug() << "got a messages packet";
    }
}

void MessagesClient::sendMessage(const QString& channel, const QString& message) {
    auto nodeList = DependencyManager::get<NodeList>();
    SharedNodePointer messagesMixer = nodeList->soloNodeOfType(NodeType::MessagesMixer);
    
    if (messagesMixer) {
        auto packetList = NLPacketList::create(PacketType::MessagesData, QByteArray(), true, true);

        #if 0
        auto messageID = ++_currentID;
        packetList->writePrimitive(messageID);

        packetList->writePrimitive(static_cast<uint8_t>(extension.length()));
        packetList->write(extension.toLatin1().constData(), extension.length());

        uint64_t size = data.length();
        packetList->writePrimitive(size);
        packetList->write(data.constData(), size);

        nodeList->sendPacketList(std::move(packetList), *assetServer);
        #endif
    }
}

void MessagesClient::handleNodeKilled(SharedNodePointer node) {
    if (node->getType() != NodeType::MessagesMixer) {
        return;
    }

}
