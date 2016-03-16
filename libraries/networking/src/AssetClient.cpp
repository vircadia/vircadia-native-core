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
#include <QtScript/QScriptEngine>
#include <QtNetwork/QNetworkDiskCache>

#include "AssetRequest.h"
#include "AssetUpload.h"
#include "AssetUtils.h"
#include "MappingRequest.h"
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

    packetReceiver.registerListener(PacketType::AssetMappingOperationReply, this, "handleAssetMappingOperationReply");
    packetReceiver.registerListener(PacketType::AssetGetInfoReply, this, "handleAssetGetInfoReply");
    packetReceiver.registerListener(PacketType::AssetGetReply, this, "handleAssetGetReply", true);
    packetReceiver.registerListener(PacketType::AssetUploadReply, this, "handleAssetUploadReply");

    connect(nodeList.data(), &LimitedNodeList::nodeKilled, this, &AssetClient::handleNodeKilled);
}

void AssetClient::init() {
    Q_ASSERT(QThread::currentThread() == thread());

    // Setup disk cache if not already
    auto& networkAccessManager = NetworkAccessManager::getInstance();
    if (!networkAccessManager.cache()) {
        QString cachePath = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
        cachePath = !cachePath.isEmpty() ? cachePath : "interfaceCache";

        QNetworkDiskCache* cache = new QNetworkDiskCache();
        cache->setMaximumCacheSize(MAXIMUM_CACHE_SIZE);
        cache->setCacheDirectory(cachePath);
        networkAccessManager.setCache(cache);
        qDebug() << "ResourceManager disk cache setup at" << cachePath
                 << "(size:" << MAXIMUM_CACHE_SIZE / BYTES_PER_GIGABYTES << "GB)";
    }
}


void AssetClient::cacheInfoRequest(QObject* reciever, QString slot) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "cacheInfoRequest", Qt::QueuedConnection,
                                  Q_ARG(QObject*, reciever), Q_ARG(QString, slot));
        return;
    }


    if (auto* cache = qobject_cast<QNetworkDiskCache*>(NetworkAccessManager::getInstance().cache())) {
        QMetaObject::invokeMethod(reciever, slot.toStdString().data(), Qt::QueuedConnection,
                                  Q_ARG(QString, cache->cacheDirectory()),
                                  Q_ARG(qint64, cache->cacheSize()),
                                  Q_ARG(qint64, cache->maximumCacheSize()));
    } else {
        qCWarning(asset_client) << "No disk cache to get info from.";
    }
}

void AssetClient::clearCache() {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "clearCache", Qt::QueuedConnection);
        return;
    }

    if (auto cache = NetworkAccessManager::getInstance().cache()) {
        qDebug() << "AssetClient::clearCache(): Clearing disk cache.";
        cache->clear();
    } else {
        qCWarning(asset_client) << "No disk cache to clear.";
    }
}

void AssetClient::handleAssetMappingOperationReply(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode) {
    MessageID messageID;
    message->readPrimitive(&messageID);
    
    AssetServerError error;
    message->readPrimitive(&error);

    // Check if we have any pending requests for this node
    auto messageMapIt = _pendingMappingRequests.find(senderNode);
    if (messageMapIt != _pendingMappingRequests.end()) {

        // Found the node, get the MessageID -> Callback map
        auto& messageCallbackMap = messageMapIt->second;

        // Check if we have this pending request
        auto requestIt = messageCallbackMap.find(messageID);
        if (requestIt != messageCallbackMap.end()) {
            auto callback = requestIt->second;
            callback(true, error, message);
            messageCallbackMap.erase(requestIt);
        }

        // Although the messageCallbackMap may now be empty, we won't delete the node until we have disconnected from
        // it to avoid constantly creating/deleting the map on subsequent requests.
    }
}

bool haveAssetServer() {
    auto nodeList = DependencyManager::get<NodeList>();
    SharedNodePointer assetServer = nodeList->soloNodeOfType(NodeType::AssetServer);
    
    if (!assetServer) {
        qCWarning(asset_client) << "Could not complete AssetClient operation "
            << "since you are not currently connected to an asset-server.";
        return false;
    }
    
    return true;
}

GetMappingRequest* AssetClient::createGetMappingRequest(const AssetPath& path) {
    return new GetMappingRequest(path);
}

GetAllMappingsRequest* AssetClient::createGetAllMappingsRequest() {
    auto request = new GetAllMappingsRequest();

    request->moveToThread(thread());

    return request;
}

DeleteMappingsRequest* AssetClient::createDeleteMappingsRequest(const AssetPathList& paths) {
    auto request = new DeleteMappingsRequest(paths);

    request->moveToThread(thread());

    return request;
}

SetMappingRequest* AssetClient::createSetMappingRequest(const AssetPath& path, const AssetHash& hash) {
    auto request = new SetMappingRequest(path, hash);

    request->moveToThread(thread());

    return request;
}

RenameMappingRequest* AssetClient::createRenameMappingRequest(const AssetPath& oldPath, const AssetPath& newPath) {
    auto request = new RenameMappingRequest(oldPath, newPath);

    request->moveToThread(thread());

    return request;
}

AssetRequest* AssetClient::createRequest(const AssetHash& hash) {
    auto request = new AssetRequest(hash);

    // Move to the AssetClient thread in case we are not currently on that thread (which will usually be the case)
    request->moveToThread(thread());

    return request;
}

AssetUpload* AssetClient::createUpload(const QString& filename) {
    auto upload = new AssetUpload(filename);

    upload->moveToThread(thread());

    return upload;
}

AssetUpload* AssetClient::createUpload(const QByteArray& data) {
    auto upload = new AssetUpload(data);

    upload->moveToThread(thread());

    return upload;
}

bool AssetClient::getAsset(const QString& hash, DataOffset start, DataOffset end,
                           ReceivedAssetCallback callback, ProgressCallback progressCallback) {
    if (hash.length() != SHA256_HASH_HEX_LENGTH) {
        qCWarning(asset_client) << "Invalid hash size";
        return false;
    }

    auto nodeList = DependencyManager::get<NodeList>();
    SharedNodePointer assetServer = nodeList->soloNodeOfType(NodeType::AssetServer);

    if (assetServer) {
        
        auto messageID = ++_currentID;
        
        auto payloadSize = sizeof(messageID) + SHA256_HASH_LENGTH + sizeof(start) + sizeof(end);
        auto packet = NLPacket::create(PacketType::AssetGet, payloadSize, true);
        
        qCDebug(asset_client) << "Requesting data from" << start << "to" << end << "of" << hash << "from asset-server.";
        
        packet->writePrimitive(messageID);

        packet->write(QByteArray::fromHex(hash.toLatin1()));

        packet->writePrimitive(start);
        packet->writePrimitive(end);

        nodeList->sendPacket(std::move(packet), *assetServer);

        _pendingRequests[assetServer][messageID] = { callback, progressCallback };

        return true;
    } else {
        callback(false, AssetServerError::NoError, QByteArray());
        return false;
    }


}

bool AssetClient::getAssetInfo(const QString& hash, GetInfoCallback callback) {
    auto nodeList = DependencyManager::get<NodeList>();
    SharedNodePointer assetServer = nodeList->soloNodeOfType(NodeType::AssetServer);

    if (assetServer) {
        auto messageID = ++_currentID;
        
        auto payloadSize = sizeof(messageID) + SHA256_HASH_LENGTH;
        auto packet = NLPacket::create(PacketType::AssetGetInfo, payloadSize, true);
        
        packet->writePrimitive(messageID);
        packet->write(QByteArray::fromHex(hash.toLatin1()));

        nodeList->sendPacket(std::move(packet), *assetServer);

        _pendingInfoRequests[assetServer][messageID] = callback;

        return true;
    } else {
        callback(false, AssetServerError::NoError, { "", 0 });
        return false;
    }
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
    message->readHeadPrimitive(&messageID);

    AssetServerError error;
    message->readHeadPrimitive(&error);

    DataOffset length = 0;
    if (!error) {
        message->readHeadPrimitive(&length);
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
                connect(message.data(), &ReceivedMessage::progress, this, [this, length, message, callbacks]() {
                    callbacks.progressCallback(message->getSize(), length);
                });
                connect(message.data(), &ReceivedMessage::completed, this, [this, message, error, callbacks]() {
                    if (message->failed()) {
                        callbacks.completeCallback(false, AssetServerError::NoError, QByteArray());
                    } else {
                        callbacks.completeCallback(true, error, message->readAll());
                    }
                });
            }
            messageCallbackMap.erase(requestIt);
        }

        // Although the messageCallbackMap may now be empty, we won't delete the node until we have disconnected from
        // it to avoid constantly creating/deleting the map on subsequent requests.
    }
}

bool AssetClient::getAssetMapping(const AssetPath& path, MappingOperationCallback callback) {
    auto nodeList = DependencyManager::get<NodeList>();
    SharedNodePointer assetServer = nodeList->soloNodeOfType(NodeType::AssetServer);

    if (assetServer) {
        auto packetList = NLPacketList::create(PacketType::AssetMappingOperation, QByteArray(), true, true);

        auto messageID = ++_currentID;
        packetList->writePrimitive(messageID);

        packetList->writePrimitive(AssetMappingOperationType::Get);

        packetList->writeString(path);

        nodeList->sendPacketList(std::move(packetList), *assetServer);

        _pendingMappingRequests[assetServer][messageID] = callback;

        return true;
    } else {
        callback(false, AssetServerError::NoError, QSharedPointer<ReceivedMessage>());
        return false;
    }
}

bool AssetClient::getAllAssetMappings(MappingOperationCallback callback) {
    auto nodeList = DependencyManager::get<NodeList>();
    SharedNodePointer assetServer = nodeList->soloNodeOfType(NodeType::AssetServer);
    
    if (assetServer) {
        auto packetList = NLPacketList::create(PacketType::AssetMappingOperation, QByteArray(), true, true);

        auto messageID = ++_currentID;
        packetList->writePrimitive(messageID);

        packetList->writePrimitive(AssetMappingOperationType::GetAll);

        nodeList->sendPacketList(std::move(packetList), *assetServer);

        _pendingMappingRequests[assetServer][messageID] = callback;

        return true;
    } else {
        callback(false, AssetServerError::NoError, QSharedPointer<ReceivedMessage>());
        return false;
    }
}

bool AssetClient::deleteAssetMappings(const AssetPathList& paths, MappingOperationCallback callback) {
    auto nodeList = DependencyManager::get<NodeList>();
    SharedNodePointer assetServer = nodeList->soloNodeOfType(NodeType::AssetServer);
    
    if (assetServer) {
        auto packetList = NLPacketList::create(PacketType::AssetMappingOperation, QByteArray(), true, true);

        auto messageID = ++_currentID;
        packetList->writePrimitive(messageID);

        packetList->writePrimitive(AssetMappingOperationType::Delete);

        packetList->writePrimitive(int(paths.size()));

        for (auto& path: paths) {
            packetList->writeString(path);
        }

        nodeList->sendPacketList(std::move(packetList), *assetServer);

        _pendingMappingRequests[assetServer][messageID] = callback;

        return true;
    } else {
        callback(false, AssetServerError::NoError, QSharedPointer<ReceivedMessage>());
        return false;
    }
}

bool AssetClient::setAssetMapping(const QString& path, const AssetHash& hash, MappingOperationCallback callback) {
    auto nodeList = DependencyManager::get<NodeList>();
    SharedNodePointer assetServer = nodeList->soloNodeOfType(NodeType::AssetServer);
    
    if (assetServer) {
        auto packetList = NLPacketList::create(PacketType::AssetMappingOperation, QByteArray(), true, true);

        auto messageID = ++_currentID;
        packetList->writePrimitive(messageID);

        packetList->writePrimitive(AssetMappingOperationType::Set);

        packetList->writeString(path);
        packetList->write(QByteArray::fromHex(hash.toUtf8()));

        nodeList->sendPacketList(std::move(packetList), *assetServer);

        _pendingMappingRequests[assetServer][messageID] = callback;

        return true;
    } else {
        callback(false, AssetServerError::NoError, QSharedPointer<ReceivedMessage>());
        return false;
    }
}

bool AssetClient::renameAssetMapping(const AssetPath& oldPath, const AssetPath& newPath, MappingOperationCallback callback) {
    auto nodeList = DependencyManager::get<NodeList>();
    SharedNodePointer assetServer = nodeList->soloNodeOfType(NodeType::AssetServer);

    if (assetServer) {
        auto packetList = NLPacketList::create(PacketType::AssetMappingOperation, QByteArray(), true, true);

        auto messageID = ++_currentID;
        packetList->writePrimitive(messageID);

        packetList->writePrimitive(AssetMappingOperationType::Rename);

        packetList->writeString(oldPath);
        packetList->writeString(newPath);

        nodeList->sendPacketList(std::move(packetList), *assetServer);

        _pendingMappingRequests[assetServer][messageID] = callback;

        return true;

    } else {
        callback(false, AssetServerError::NoError, QSharedPointer<ReceivedMessage>());
        return false;
    }
}

bool AssetClient::uploadAsset(const QByteArray& data, UploadResultCallback callback) {
    auto nodeList = DependencyManager::get<NodeList>();
    SharedNodePointer assetServer = nodeList->soloNodeOfType(NodeType::AssetServer);
    
    if (assetServer) {
        auto packetList = NLPacketList::create(PacketType::AssetUpload, QByteArray(), true, true);

        auto messageID = ++_currentID;
        packetList->writePrimitive(messageID);

        uint64_t size = data.length();
        packetList->writePrimitive(size);
        packetList->write(data.constData(), size);

        nodeList->sendPacketList(std::move(packetList), *assetServer);

        _pendingUploads[assetServer][messageID] = callback;

        return true;
    } else {
        callback(false, AssetServerError::NoError, QString());
        return false;
    }
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

    {
        auto messageMapIt = _pendingMappingRequests.find(node);
        if (messageMapIt != _pendingMappingRequests.end()) {
            for (const auto& value : messageMapIt->second) {
                value.second(false, AssetServerError::NoError, QSharedPointer<ReceivedMessage>());
            }
            messageMapIt->second.clear();
        }
    }
}
