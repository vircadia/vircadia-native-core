//
//  AssetClient.cpp
//
//  Created by Ryan Huffman on 2015/07/21
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AssetClient.h"

#include <QBuffer>
#include <QThread>

#include "AssetRequest.h"
#include "AssetUpload.h"
#include "NodeList.h"
#include "PacketReceiver.h"
#include "AssetUtils.h"

MessageID AssetClient::_currentID = 0;


AssetClient::AssetClient() {
    auto& packetReceiver = DependencyManager::get<NodeList>()->getPacketReceiver();
    packetReceiver.registerListener(PacketType::AssetGetInfoReply, this, "handleAssetGetInfoReply");
    packetReceiver.registerMessageListener(PacketType::AssetGetReply, this, "handleAssetGetReply");
    packetReceiver.registerListener(PacketType::AssetUploadReply, this, "handleAssetUploadReply");
}

AssetRequest* AssetClient::createRequest(QString hash) {
    if (QThread::currentThread() != thread()) {
        AssetRequest* req;
        QMetaObject::invokeMethod(this, "createRequest",
            Qt::BlockingQueuedConnection, 
            Q_RETURN_ARG(AssetRequest*, req),
            Q_ARG(QString, hash));
        return req;
    }

    if (hash.length() != HASH_HEX_LENGTH) {
        qDebug() << "Invalid hash size";
        return nullptr;
    }

    auto nodeList = DependencyManager::get<NodeList>();
    SharedNodePointer assetServer = nodeList->soloNodeOfType(NodeType::AssetServer);

    if (assetServer) {
        return new AssetRequest(this, hash);
    }

    return nullptr;
}

AssetUpload* AssetClient::createUpload(QString filename) {
    if (QThread::currentThread() != thread()) {
        AssetUpload* upload;
        QMetaObject::invokeMethod(this, "createUpload",
                                  Qt::BlockingQueuedConnection,
                                  Q_RETURN_ARG(AssetUpload*, upload),
                                  Q_ARG(QString, filename));
        return upload;
    }
    
    auto nodeList = DependencyManager::get<NodeList>();
    SharedNodePointer assetServer = nodeList->soloNodeOfType(NodeType::AssetServer);
    
    if (assetServer) {
        return new AssetUpload(this, filename);
    }
    
    return nullptr;
}

bool AssetClient::getAsset(QString hash, DataOffset start, DataOffset end, ReceivedAssetCallback callback) {
    if (hash.length() != HASH_HEX_LENGTH) {
        qDebug() << "Invalid hash size";
        return false;
    }

    auto nodeList = DependencyManager::get<NodeList>();
    SharedNodePointer assetServer = nodeList->soloNodeOfType(NodeType::AssetServer);

    if (assetServer) {
        auto packet = NLPacket::create(PacketType::AssetGet);

        auto messageID = ++_currentID;
        
        qDebug() << "Requesting data from" << start << "to" << end << "of" << hash << "from asset-server.";
        
        packet->writePrimitive(messageID);
        packet->write(hash.toLatin1().constData(), HASH_HEX_LENGTH);
        packet->writePrimitive(start);
        packet->writePrimitive(end);

        nodeList->sendPacket(std::move(packet), *assetServer);

        _pendingRequests[messageID] = callback;

        return true;
    }

    return false;
}

bool AssetClient::getAssetInfo(QString hash, GetInfoCallback callback) {
    auto nodeList = DependencyManager::get<NodeList>();
    SharedNodePointer assetServer = nodeList->soloNodeOfType(NodeType::AssetServer);

    if (assetServer) {
        auto packet = NLPacket::create(PacketType::AssetGetInfo);

        auto messageID = ++_currentID;
        packet->writePrimitive(messageID);
        packet->write(hash.toLatin1().constData(), HASH_HEX_LENGTH);

        nodeList->sendPacket(std::move(packet), *assetServer);

        _pendingInfoRequests[messageID] = callback;

        return true;
    }

    return false;
}

void AssetClient::handleAssetGetInfoReply(QSharedPointer<NLPacket> packet, SharedNodePointer senderNode) {
    MessageID messageID;
    packet->readPrimitive(&messageID);
    auto assetHash = QString(packet->read(HASH_HEX_LENGTH));
    
    AssetServerError error;
    packet->readPrimitive(&error);

    AssetInfo info { assetHash, 0 };

    if (error == NO_ERROR) {
        packet->readPrimitive(&info.size);
    }

    if (_pendingInfoRequests.contains(messageID)) {
        auto callback = _pendingInfoRequests.take(messageID);
        callback(error == NO_ERROR, info);
    }
}

void AssetClient::handleAssetGetReply(QSharedPointer<NLPacketList> packetList, SharedNodePointer senderNode) {
    QByteArray data = packetList->getMessage();
    QBuffer packet { &data };
    packet.open(QIODevice::ReadOnly);

    auto assetHash = packet.read(HASH_HEX_LENGTH);
    qDebug() << "Got reply for asset: " << assetHash;

    MessageID messageID;
    packet.read(reinterpret_cast<char*>(&messageID), sizeof(messageID));

    AssetServerError error;
    packet.read(reinterpret_cast<char*>(&error), sizeof(AssetServerError));
    QByteArray assetData;

    if (!error) {
        DataOffset length;
        packet.read(reinterpret_cast<char*>(&length), sizeof(DataOffset));
        data = packet.read(length);
    } else {
        qDebug() << "Failure getting asset: " << error;
    }

    if (_pendingRequests.contains(messageID)) {
        auto callback = _pendingRequests.take(messageID);
        callback(error == NO_ERROR, data);
    }
}

bool AssetClient::uploadAsset(QByteArray data, QString extension, UploadResultCallback callback) {
    auto nodeList = DependencyManager::get<NodeList>();
    SharedNodePointer assetServer = nodeList->soloNodeOfType(NodeType::AssetServer);
    
    if (assetServer) {
        auto packetList = std::unique_ptr<NLPacketList>(new NLPacketList(PacketType::AssetUpload, QByteArray(), true, true));

        auto messageID = ++_currentID;
        packetList->writePrimitive(messageID);

        packetList->writePrimitive(static_cast<uint8_t>(extension.length()));
        packetList->write(extension.toLatin1().constData(), extension.length());

        qDebug() << "Extension length: " << extension.length();
        qDebug() << "Extension: " << extension;

        uint64_t size = data.length();
        packetList->writePrimitive(size);
        packetList->write(data.constData(), size);

        nodeList->sendPacketList(std::move(packetList), *assetServer);

        _pendingUploads[messageID] = callback;

        return true;
    }
    return false;
}

void AssetClient::handleAssetUploadReply(QSharedPointer<NLPacket> packet, SharedNodePointer senderNode) {
    MessageID messageID;
    packet->readPrimitive(&messageID);

    AssetServerError error;
    packet->readPrimitive(&error);

    QString hashString { "" };

    if (error) {
        qDebug() << "Error uploading file to asset server";
    } else {
        auto hashData = packet->read(HASH_HEX_LENGTH);

        hashString = QString(hashData);

        qDebug() << "Successfully uploaded asset to asset-server - SHA256 hash is " << hashString;
    }

    if (_pendingUploads.contains(messageID)) {
        auto callback = _pendingUploads.take(messageID);
        callback(error == NO_ERROR, hashString);
    }
}
