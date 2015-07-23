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

#include "NodeList.h"
#include "PacketReceiver.h"

const int HASH_HEX_LENGTH = 32;
MessageID AssetClient::_currentID = 0;

AssetClient::AssetClient() {
    auto& packetReceiver = DependencyManager::get<NodeList>()->getPacketReceiver();
    packetReceiver.registerListener(PacketType::AssetGetReply, this, "handleAssetGetReply");
    packetReceiver.registerListener(PacketType::AssetUploadReply, this, "handleAssetUploadReply");
}

bool AssetClient::getAsset(QString hash, ReceivedAssetCallback callback) {
    if (hash.length() != 32) {
        qDebug() << "Invalid hash size";
        return false;
    }

    auto nodeList = DependencyManager::get<NodeList>();
    SharedNodePointer assetServer = nodeList->soloNodeOfType(NodeType::AssetServer);

    if (assetServer) {
        auto packet = NLPacket::create(PacketType::AssetGet);
        packet->write(hash.toLatin1().constData(), 32);
        nodeList->sendPacket(std::move(packet), *assetServer);

        _pendingRequests[hash] = callback;

        return true;
    }

    return false;
}

void AssetClient::handleAssetGetReply(QSharedPointer<NLPacket> packet, SharedNodePointer senderNode) {
    auto assetHash = packet->read(HASH_HEX_LENGTH);
    qDebug() << "Got reply for asset: " << assetHash;

    bool success;
    packet->readPrimitive(&success);
    QByteArray data;

    if (success) {
        int length;
        packet->readPrimitive(&length);
        char assetData[length];
        packet->read(assetData, length);

        data = QByteArray(assetData, length);
    } else {
        qDebug() << "Failure getting asset";
    }

    if (_pendingRequests.contains(assetHash)) {
        auto callback = _pendingRequests.take(assetHash);
        callback(success, data);
    }
}

bool AssetClient::uploadAsset(QByteArray data, QString extension, UploadResultCallback callback) {
    auto nodeList = DependencyManager::get<NodeList>();
    SharedNodePointer assetServer = nodeList->soloNodeOfType(NodeType::AssetServer);
    if (assetServer) {
        auto packet = NLPacket::create(PacketType::AssetUpload);

        auto messageID = _currentID++;
        packet->writePrimitive(messageID);

        packet->writePrimitive(static_cast<char>(extension.length()));
        packet->write(extension.toLatin1().constData(), extension.length());

        qDebug() << "Extension length: " << extension.length();
        qDebug() << "Extension: " << extension;

        int size = data.length();
        packet->writePrimitive(size);
        packet->write(data.constData(), size);

        nodeList->sendPacket(std::move(packet), *assetServer);

        _pendingUploads[messageID] = callback;

        return true;
    }
    return false;
}

void AssetClient::handleAssetUploadReply(QSharedPointer<NLPacket> packet, SharedNodePointer senderNode) {
    qDebug() << "Got asset upload reply";
    MessageID messageID;
    packet->readPrimitive(&messageID);

    bool success;
    packet->readPrimitive(&success);

    QString hashString { "" };

    if (success) {
        auto hashData = packet->read(HASH_HEX_LENGTH);

        hashString = QString(hashData);

        qDebug() << "Hash: " << hashString;
    } else {
        qDebug() << "Error uploading file";
    }

    if (_pendingUploads.contains(messageID)) {
        auto callback = _pendingUploads.take(messageID);
        callback(success, hashString);
    }
}
