//
//  AssetClient.cpp
//  libraries/networking/src
//
//  Created by Ryan Huffman on 2015/07/21
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AssetClient.h"

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

MessageID AssetClient::_currentID = 0;


AssetClient::AssetClient() {
    
    setCustomDeleter([](Dependency* dependency){
        static_cast<AssetClient*>(dependency)->deleteLater();
    });
    
    auto nodeList = DependencyManager::get<NodeList>();
    auto& packetReceiver = nodeList->getPacketReceiver();
    packetReceiver.registerListener(PacketType::AssetGetInfoReply, this, "handleAssetGetInfoReply");
    packetReceiver.registerListener(PacketType::AssetGetReply, this, "handleAssetGetReply", true);
    packetReceiver.registerListener(PacketType::AssetUploadReply, this, "handleAssetUploadReply");

    connect(nodeList.data(), &LimitedNodeList::nodeKilled, this, &AssetClient::handleNodeKilled);
}

void AssetClient::init() {
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
        qCDebug(asset_client) << "AssetClient disk cache setup at" << cachePath
                                << "(size:" << MAXIMUM_CACHE_SIZE / BYTES_PER_GIGABYTES << "GB)";
    }
}

AssetRequest* AssetClient::createRequest(const QString& hash, const QString& extension) {
    if (hash.length() != SHA256_HASH_HEX_LENGTH) {
        qCWarning(asset_client) << "Invalid hash size";
        return nullptr;
    }

    auto nodeList = DependencyManager::get<NodeList>();
    SharedNodePointer assetServer = nodeList->soloNodeOfType(NodeType::AssetServer);

    if (!assetServer) {
        qCWarning(asset_client).nospace() << "Could not request " << hash << "." << extension
            << " since you are not currently connected to an asset-server.";
        return nullptr;
    }

    auto request = new AssetRequest(hash, extension);

    // Move to the AssetClient thread in case we are not currently on that thread (which will usually be the case)
    request->moveToThread(thread());

    return request;
}

AssetUpload* AssetClient::createUpload(const QString& filename) {
    auto nodeList = DependencyManager::get<NodeList>();
    SharedNodePointer assetServer = nodeList->soloNodeOfType(NodeType::AssetServer);
    
    if (!assetServer) {
        qCWarning(asset_client) << "Could not upload" << filename << "since you are not currently connected to an asset-server.";
        return nullptr;
    }
    
    auto upload = new AssetUpload(this, filename);

    upload->moveToThread(thread());

    return upload;
}

bool AssetClient::getAsset(const QString& hash, const QString& extension, DataOffset start, DataOffset end,
                           ReceivedAssetCallback callback, ProgressCallback progressCallback) {
    if (hash.length() != SHA256_HASH_HEX_LENGTH) {
        qCWarning(asset_client) << "Invalid hash size";
        return false;
    }

    auto nodeList = DependencyManager::get<NodeList>();
    SharedNodePointer assetServer = nodeList->soloNodeOfType(NodeType::AssetServer);

    if (assetServer) {
        
        auto messageID = ++_currentID;
        
        auto payloadSize = sizeof(messageID) + SHA256_HASH_LENGTH + sizeof(uint8_t) + extension.length()
            + sizeof(start) + sizeof(end);
        auto packet = NLPacket::create(PacketType::AssetGet, payloadSize, true);
        
        qCDebug(asset_client) << "Requesting data from" << start << "to" << end << "of" << hash << "from asset-server.";
        
        packet->writePrimitive(messageID);

        packet->write(QByteArray::fromHex(hash.toLatin1()));

        packet->writePrimitive(uint8_t(extension.length()));
        packet->write(extension.toLatin1());

        packet->writePrimitive(start);
        packet->writePrimitive(end);

        nodeList->sendPacket(std::move(packet), *assetServer);

        _pendingRequests[assetServer][messageID] = { callback, progressCallback };

        return true;
    }

    return false;
}

bool AssetClient::getAssetInfo(const QString& hash, const QString& extension, GetInfoCallback callback) {
    auto nodeList = DependencyManager::get<NodeList>();
    SharedNodePointer assetServer = nodeList->soloNodeOfType(NodeType::AssetServer);

    if (assetServer) {
        auto messageID = ++_currentID;
        
        auto payloadSize = sizeof(messageID) + SHA256_HASH_LENGTH + sizeof(uint8_t) + extension.length();
        auto packet = NLPacket::create(PacketType::AssetGetInfo, payloadSize, true);
        
        packet->writePrimitive(messageID);
        packet->write(QByteArray::fromHex(hash.toLatin1()));
        packet->writePrimitive(uint8_t(extension.length()));
        packet->write(extension.toLatin1());

        nodeList->sendPacket(std::move(packet), *assetServer);

        _pendingInfoRequests[assetServer][messageID] = callback;

        return true;
    }

    return false;
}

void AssetClient::handleAssetGetInfoReply(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode) {
    MessageID messageID;
    message->readPrimitive(&messageID);
    auto assetHash = message->read(SHA256_HASH_LENGTH);
    
    AssetServerError error;
    message->readPrimitive(&error);

    AssetInfo info { assetHash.toHex(), 0 };

    if (error == AssetServerError::NoError) {
        message->readPrimitive(&info.size);
    }

    // Check if we have any pending requests for this node
    auto messageMapIt = _pendingInfoRequests.find(senderNode);
    if (messageMapIt != _pendingInfoRequests.end()) {

        // Found the node, get the MessageID -> Callback map
        auto& messageCallbackMap = messageMapIt->second;

        // Check if we have this pending request
        auto requestIt = messageCallbackMap.find(messageID);
        if (requestIt != messageCallbackMap.end()) {
            auto callback = requestIt->second;
            callback(true, error, info);
            messageCallbackMap.erase(requestIt);
        }

        // Although the messageCallbackMap may now be empty, we won't delete the node until we have disconnected from
        // it to avoid constantly creating/deleting the map on subsequent requests.
    }
}

void AssetClient::handleAssetGetReply(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode) {

    

    auto assetHash = message->read(SHA256_HASH_LENGTH);
    qCDebug(asset_client) << "Got reply for asset: " << assetHash.toHex();

    MessageID messageID;
    message->readPrimitive(&messageID);

    AssetServerError error;
    message->readPrimitive(&error);

    // QByteArray assetData;

    DataOffset length = 0;
    if (!error) {
        message->readPrimitive(&length);
    } else {
        qCWarning(asset_client) << "Failure getting asset: " << error;
    }

    // Check if we have any pending requests for this node
    auto messageMapIt = _pendingRequests.find(senderNode);
    if (messageMapIt != _pendingRequests.end()) {

        // Found the node, get the MessageID -> Callback map
        auto& messageCallbackMap = messageMapIt->second;

        // Check if we have this pending request
        auto requestIt = messageCallbackMap.find(messageID);
        if (requestIt != messageCallbackMap.end()) {
            auto& callbacks = requestIt->second;

            if (message->isComplete()) {
                callbacks.completeCallback(true, error, message->readAll());
            } else {
                connect(message.data(), &ReceivedMessage::progress, this, [this, length, message, callbacks](ReceivedMessage* msg) {
                    //qDebug() << "Progress: " << msg->getDataSize();
                    callbacks.progressCallback(msg->getSize(), length);
                });
                connect(message.data(), &ReceivedMessage::completed, this, [this, message, error, callbacks](ReceivedMessage* msg) {
                    callbacks.completeCallback(true, error, message->readAll());
                });
            }
            messageCallbackMap.erase(requestIt);
        }

        // Although the messageCallbackMap may now be empty, we won't delete the node until we have disconnected from
        // it to avoid constantly creating/deleting the map on subsequent requests.
    }
}

bool AssetClient::uploadAsset(const QByteArray& data, const QString& extension, UploadResultCallback callback) {
    auto nodeList = DependencyManager::get<NodeList>();
    SharedNodePointer assetServer = nodeList->soloNodeOfType(NodeType::AssetServer);
    
    if (assetServer) {
        auto packetList = NLPacketList::create(PacketType::AssetUpload, QByteArray(), true, true);

        auto messageID = ++_currentID;
        packetList->writePrimitive(messageID);

        packetList->writePrimitive(static_cast<uint8_t>(extension.length()));
        packetList->write(extension.toLatin1().constData(), extension.length());

        uint64_t size = data.length();
        packetList->writePrimitive(size);
        packetList->write(data.constData(), size);

        nodeList->sendPacketList(std::move(packetList), *assetServer);

        _pendingUploads[assetServer][messageID] = callback;

        return true;
    }
    return false;
}

void AssetClient::handleAssetUploadReply(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode) {
    MessageID messageID;
    message->readPrimitive(&messageID);

    AssetServerError error;
    message->readPrimitive(&error);

    QString hashString;

    if (error) {
        qCWarning(asset_client) << "Error uploading file to asset server";
    } else {
        auto hash = message->read(SHA256_HASH_LENGTH);
        hashString = hash.toHex();
        
        qCDebug(asset_client) << "Successfully uploaded asset to asset-server - SHA256 hash is " << hashString;
    }

    // Check if we have any pending requests for this node
    auto messageMapIt = _pendingUploads.find(senderNode);
    if (messageMapIt != _pendingUploads.end()) {

        // Found the node, get the MessageID -> Callback map
        auto& messageCallbackMap = messageMapIt->second;

        // Check if we have this pending request
        auto requestIt = messageCallbackMap.find(messageID);
        if (requestIt != messageCallbackMap.end()) {
            auto callback = requestIt->second;
            callback(true, error, hashString);
            messageCallbackMap.erase(requestIt);
        }

        // Although the messageCallbackMap may now be empty, we won't delete the node until we have disconnected from
        // it to avoid constantly creating/deleting the map on subsequent requests.
    }
}

void AssetClient::handleNodeKilled(SharedNodePointer node) {
    if (node->getType() != NodeType::AssetServer) {
        return;
    }

    {
        auto messageMapIt = _pendingRequests.find(node);
        if (messageMapIt != _pendingRequests.end()) {
            for (const auto& value : messageMapIt->second) {
                value.second.completeCallback(false, AssetServerError::NoError, QByteArray());
            }
            messageMapIt->second.clear();
        }
    }

    {
        auto messageMapIt = _pendingInfoRequests.find(node);
        if (messageMapIt != _pendingInfoRequests.end()) {
            AssetInfo info { "", 0 };
            for (const auto& value : messageMapIt->second) {
                value.second(false, AssetServerError::NoError, info);
            }
            messageMapIt->second.clear();
        }
    }

    {
        auto messageMapIt = _pendingUploads.find(node);
        if (messageMapIt != _pendingUploads.end()) {
            for (const auto& value : messageMapIt->second) {
                value.second(false, AssetServerError::NoError, "");
            }
            messageMapIt->second.clear();
        }
    }
}
