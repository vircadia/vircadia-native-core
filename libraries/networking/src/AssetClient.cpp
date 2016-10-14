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
    connect(nodeList.data(), &LimitedNodeList::clientConnectionToNodeReset,
            this, &::AssetClient::handleNodeClientConnectionReset);
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
        qInfo() << "ResourceManager disk cache setup at" << cachePath
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
        qInfo() << "AssetClient::clearCache(): Clearing disk cache.";
        cache->clear();
    } else {
        qCWarning(asset_client) << "No disk cache to clear.";
    }
}

void AssetClient::handleAssetMappingOperationReply(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode) {
    Q_ASSERT(QThread::currentThread() == thread());

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
    auto request = new GetMappingRequest(path);

    request->moveToThread(thread());

    return request;
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

MessageID AssetClient::getAsset(const QString& hash, DataOffset start, DataOffset end,
                                ReceivedAssetCallback callback, ProgressCallback progressCallback) {
    Q_ASSERT(QThread::currentThread() == thread());

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

        if (nodeList->sendPacket(std::move(packet), *assetServer) != -1) {
            _pendingRequests[assetServer][messageID] = { QSharedPointer<ReceivedMessage>(), callback, progressCallback };

            return messageID;
        }
    }

    callback(false, AssetServerError::NoError, QByteArray());
    return INVALID_MESSAGE_ID;
}

MessageID AssetClient::getAssetInfo(const QString& hash, GetInfoCallback callback) {
    Q_ASSERT(QThread::currentThread() == thread());

    auto nodeList = DependencyManager::get<NodeList>();
    SharedNodePointer assetServer = nodeList->soloNodeOfType(NodeType::AssetServer);

    if (assetServer) {
        auto messageID = ++_currentID;
        
        auto payloadSize = sizeof(messageID) + SHA256_HASH_LENGTH;
        auto packet = NLPacket::create(PacketType::AssetGetInfo, payloadSize, true);
        
        packet->writePrimitive(messageID);
        packet->write(QByteArray::fromHex(hash.toLatin1()));

        if (nodeList->sendPacket(std::move(packet), *assetServer) != -1) {
            _pendingInfoRequests[assetServer][messageID] = callback;

            return messageID;
        }
    }

    callback(false, AssetServerError::NoError, { "", 0 });
    return INVALID_MESSAGE_ID;
}

void AssetClient::handleAssetGetInfoReply(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode) {
    Q_ASSERT(QThread::currentThread() == thread());

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
    Q_ASSERT(QThread::currentThread() == thread());

    auto assetHash = message->readHead(SHA256_HASH_LENGTH);
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
    if (messageMapIt == _pendingRequests.end()) {
        return;
    }

    // Found the node, get the MessageID -> Callback map
    auto& messageCallbackMap = messageMapIt->second;

    // Check if we have this pending request
    auto requestIt = messageCallbackMap.find(messageID);
    if (requestIt == messageCallbackMap.end()) {
        // Although the messageCallbackMap may now be empty, we won't delete the node until we have disconnected from
        // it to avoid constantly creating/deleting the map on subsequent requests.
        return;
    }

    auto& callbacks = requestIt->second;

    // Store message in case we need to disconnect from it later.
    callbacks.message = message;

    if (message->isComplete()) {
        callbacks.completeCallback(true, error, message->readAll());
        messageCallbackMap.erase(requestIt);
    } else {
        auto weakNode = senderNode.toWeakRef();

        connect(message.data(), &ReceivedMessage::progress, this, [this, weakNode, messageID, length](qint64 size) {
            handleProgressCallback(weakNode, messageID, size, length);
        });
        connect(message.data(), &ReceivedMessage::completed, this, [this, weakNode, messageID]() {
            handleCompleteCallback(weakNode, messageID);
        });
    }
}

void AssetClient::handleProgressCallback(const QWeakPointer<Node>& node, MessageID messageID,
                                         qint64 size, DataOffset length) {
    auto senderNode = node.toStrongRef();

    if (!senderNode) {
        return;
    }

    // Check if we have any pending requests for this node
    auto messageMapIt = _pendingRequests.find(senderNode);
    if (messageMapIt == _pendingRequests.end()) {
        return;
    }

    // Found the node, get the MessageID -> Callback map
    auto& messageCallbackMap = messageMapIt->second;

    // Check if we have this pending request
    auto requestIt = messageCallbackMap.find(messageID);
    if (requestIt == messageCallbackMap.end()) {
        return;
    }

    auto& callbacks = requestIt->second;
    callbacks.progressCallback(size, length);
}

void AssetClient::handleCompleteCallback(const QWeakPointer<Node>& node, MessageID messageID) {
    auto senderNode = node.toStrongRef();

    if (!senderNode) {
        qCWarning(asset_client) << "Got completed asset for node that no longer exists";
        return;
    }

    // Check if we have any pending requests for this node
    auto messageMapIt = _pendingRequests.find(senderNode);
    if (messageMapIt == _pendingRequests.end()) {
        qCWarning(asset_client) << "Got completed asset for a node that doesn't have any pending requests";
        return;
    }

    // Found the node, get the MessageID -> Callback map
    auto& messageCallbackMap = messageMapIt->second;

    // Check if we have this pending request
    auto requestIt = messageCallbackMap.find(messageID);
    if (requestIt == messageCallbackMap.end()) {
        qCWarning(asset_client) << "Got completed asset for a request that doesn't exist";
        return;
    }

    auto& callbacks = requestIt->second;
    auto& message = callbacks.message;

    if (!message) {
        qCWarning(asset_client) << "Got completed asset for a message that doesn't exist";
        return;
    }


    if (message->failed()) {
        callbacks.completeCallback(false, AssetServerError::NoError, QByteArray());
    } else {
        callbacks.completeCallback(true, AssetServerError::NoError, message->readAll());
    }

    // We should never get to this point without the associated senderNode and messageID
    // in our list of pending requests. If the senderNode had disconnected or the message
    // had been canceled, we should have been disconnected from the ReceivedMessage
    // signals and thus never had this lambda called.
    messageCallbackMap.erase(messageID);
}


MessageID AssetClient::getAssetMapping(const AssetPath& path, MappingOperationCallback callback) {
    Q_ASSERT(QThread::currentThread() == thread());

    auto nodeList = DependencyManager::get<NodeList>();
    SharedNodePointer assetServer = nodeList->soloNodeOfType(NodeType::AssetServer);

    if (assetServer) {
        auto packetList = NLPacketList::create(PacketType::AssetMappingOperation, QByteArray(), true, true);

        auto messageID = ++_currentID;
        packetList->writePrimitive(messageID);

        packetList->writePrimitive(AssetMappingOperationType::Get);

        packetList->writeString(path);

        if (nodeList->sendPacketList(std::move(packetList), *assetServer) != -1) {
            _pendingMappingRequests[assetServer][messageID] = callback;

            return messageID;
        }
    }

    callback(false, AssetServerError::NoError, QSharedPointer<ReceivedMessage>());
    return INVALID_MESSAGE_ID;
}

MessageID AssetClient::getAllAssetMappings(MappingOperationCallback callback) {
    Q_ASSERT(QThread::currentThread() == thread());

    auto nodeList = DependencyManager::get<NodeList>();
    SharedNodePointer assetServer = nodeList->soloNodeOfType(NodeType::AssetServer);
    
    if (assetServer) {
        auto packetList = NLPacketList::create(PacketType::AssetMappingOperation, QByteArray(), true, true);

        auto messageID = ++_currentID;
        packetList->writePrimitive(messageID);

        packetList->writePrimitive(AssetMappingOperationType::GetAll);

        if (nodeList->sendPacketList(std::move(packetList), *assetServer) != -1) {
            _pendingMappingRequests[assetServer][messageID] = callback;

            return messageID;
        }
    }

    callback(false, AssetServerError::NoError, QSharedPointer<ReceivedMessage>());
    return INVALID_MESSAGE_ID;
}

MessageID AssetClient::deleteAssetMappings(const AssetPathList& paths, MappingOperationCallback callback) {
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

        if (nodeList->sendPacketList(std::move(packetList), *assetServer) != -1) {
            _pendingMappingRequests[assetServer][messageID] = callback;

            return messageID;
        }
    }

    callback(false, AssetServerError::NoError, QSharedPointer<ReceivedMessage>());
    return INVALID_MESSAGE_ID;
}

MessageID AssetClient::setAssetMapping(const QString& path, const AssetHash& hash, MappingOperationCallback callback) {
    Q_ASSERT(QThread::currentThread() == thread());

    auto nodeList = DependencyManager::get<NodeList>();
    SharedNodePointer assetServer = nodeList->soloNodeOfType(NodeType::AssetServer);
    
    if (assetServer) {
        auto packetList = NLPacketList::create(PacketType::AssetMappingOperation, QByteArray(), true, true);

        auto messageID = ++_currentID;
        packetList->writePrimitive(messageID);

        packetList->writePrimitive(AssetMappingOperationType::Set);

        packetList->writeString(path);
        packetList->write(QByteArray::fromHex(hash.toUtf8()));

        if (nodeList->sendPacketList(std::move(packetList), *assetServer) != -1) {
            _pendingMappingRequests[assetServer][messageID] = callback;

            return messageID;
        }
    }

    callback(false, AssetServerError::NoError, QSharedPointer<ReceivedMessage>());
    return INVALID_MESSAGE_ID;
}

MessageID AssetClient::renameAssetMapping(const AssetPath& oldPath, const AssetPath& newPath, MappingOperationCallback callback) {
    auto nodeList = DependencyManager::get<NodeList>();
    SharedNodePointer assetServer = nodeList->soloNodeOfType(NodeType::AssetServer);

    if (assetServer) {
        auto packetList = NLPacketList::create(PacketType::AssetMappingOperation, QByteArray(), true, true);

        auto messageID = ++_currentID;
        packetList->writePrimitive(messageID);

        packetList->writePrimitive(AssetMappingOperationType::Rename);

        packetList->writeString(oldPath);
        packetList->writeString(newPath);

        if (nodeList->sendPacketList(std::move(packetList), *assetServer) != -1) {
            _pendingMappingRequests[assetServer][messageID] = callback;

            return messageID;

        }
    }

    callback(false, AssetServerError::NoError, QSharedPointer<ReceivedMessage>());
    return INVALID_MESSAGE_ID;
}

bool AssetClient::cancelMappingRequest(MessageID id) {
    Q_ASSERT(QThread::currentThread() == thread());

    for (auto& kv : _pendingMappingRequests) {
        if (kv.second.erase(id)) {
            return true;
        }
    }
    return false;
}

bool AssetClient::cancelGetAssetInfoRequest(MessageID id) {
    Q_ASSERT(QThread::currentThread() == thread());

    for (auto& kv : _pendingInfoRequests) {
        if (kv.second.erase(id)) {
            return true;
        }
    }
    return false;
}

bool AssetClient::cancelGetAssetRequest(MessageID id) {
    Q_ASSERT(QThread::currentThread() == thread());

    // Search through each pending mapping request for id `id`
    for (auto& kv : _pendingRequests) {
        auto& messageCallbackMap = kv.second;
        auto requestIt = messageCallbackMap.find(id);
        if (requestIt != kv.second.end()) {

            auto& message = requestIt->second.message;
            if (message) {
                // disconnect from all signals emitting from the pending message
                disconnect(message.data(), nullptr, this, nullptr);
            }

            messageCallbackMap.erase(requestIt);

            return true;
        }
    }
    return false;
}

bool AssetClient::cancelUploadAssetRequest(MessageID id) {
    Q_ASSERT(QThread::currentThread() == thread());

    // Search through each pending mapping request for id `id`
    for (auto& kv : _pendingUploads) {
        if (kv.second.erase(id)) {
            return true;
        }
    }
    return false;
}

MessageID AssetClient::uploadAsset(const QByteArray& data, UploadResultCallback callback) {
    Q_ASSERT(QThread::currentThread() == thread());

    auto nodeList = DependencyManager::get<NodeList>();
    SharedNodePointer assetServer = nodeList->soloNodeOfType(NodeType::AssetServer);
    
    if (assetServer) {
        auto packetList = NLPacketList::create(PacketType::AssetUpload, QByteArray(), true, true);

        auto messageID = ++_currentID;
        packetList->writePrimitive(messageID);

        uint64_t size = data.length();
        packetList->writePrimitive(size);
        packetList->write(data.constData(), size);

        if (nodeList->sendPacketList(std::move(packetList), *assetServer) != -1) {
            _pendingUploads[assetServer][messageID] = callback;

            return messageID;
        }
    }

    callback(false, AssetServerError::NoError, QString());
    return INVALID_MESSAGE_ID;
}

void AssetClient::handleAssetUploadReply(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode) {
    Q_ASSERT(QThread::currentThread() == thread());

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
    Q_ASSERT(QThread::currentThread() == thread());

    if (node->getType() != NodeType::AssetServer) {
        return;
    }

    forceFailureOfPendingRequests(node);

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

void AssetClient::handleNodeClientConnectionReset(SharedNodePointer node) {
    // a client connection to a Node was reset
    // if it was an AssetServer we need to cause anything pending to fail so it is re-attempted

    if (node->getType() != NodeType::AssetServer) {
        return;
    }

    qCDebug(asset_client) << "AssetClient detected client connection reset handshake with Asset Server - failing any pending requests";

    forceFailureOfPendingRequests(node);
}

void AssetClient::forceFailureOfPendingRequests(SharedNodePointer node) {

    {
        auto messageMapIt = _pendingRequests.find(node);
        if (messageMapIt != _pendingRequests.end()) {
            for (const auto& value : messageMapIt->second) {
                auto& message = value.second.message;
                if (message) {
                    // Disconnect from all signals emitting from the pending message
                    disconnect(message.data(), nullptr, this, nullptr);
                }

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
        auto messageMapIt = _pendingMappingRequests.find(node);
        if (messageMapIt != _pendingMappingRequests.end()) {
            for (const auto& value : messageMapIt->second) {
                value.second(false, AssetServerError::NoError, QSharedPointer<ReceivedMessage>());
            }
            messageMapIt->second.clear();
        }
    }
}
