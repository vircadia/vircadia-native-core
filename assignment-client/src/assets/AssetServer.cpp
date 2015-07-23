//
//  AssetServer.cpp
//
//  Created by Ryan Huffman on 2015/07/21
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#include "AssetServer.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QCoreApplication>
#include <QEventLoop>
#include <QString>

#include <NodeType.h>

const QString ASSET_SERVER_LOGGING_TARGET_NAME = "asset-server";
const int HASH_HEX_LENGTH = 32;
using MessageID = int;

AssetServer::AssetServer(NLPacket& packet) : ThreadedAssignment(packet) {
    auto& packetReceiver = DependencyManager::get<NodeList>()->getPacketReceiver();
    packetReceiver.registerListener(PacketType::AssetGet, this, "handleAssetGet");
    packetReceiver.registerListener(PacketType::AssetUpload, this, "handleAssetUpload");
}

AssetServer::~AssetServer() {
}

void AssetServer::run() {
    ThreadedAssignment::commonInit(ASSET_SERVER_LOGGING_TARGET_NAME, NodeType::AssetServer);

    auto nodeList = DependencyManager::get<NodeList>();
    nodeList->addNodeTypeToInterestSet(NodeType::Agent);

    _resourcesDirectory = QDir(QCoreApplication::applicationDirPath()).filePath("resources/assets");
    if (!_resourcesDirectory.exists()) {
        qDebug() << "Creating resources directory";
        _resourcesDirectory.mkpath(".");
    }
    qDebug() << "Serving files from: " << _resourcesDirectory.path();

    // Scan for new files
    qDebug() << "Looking for new files in asset directory";
    auto files = _resourcesDirectory.entryInfoList(QDir::Files);
    QRegExp filenameRegex { "^[a-f0-9]{32}$" };
    for (auto fileInfo : files) {
        auto filename = fileInfo.fileName();
        if (!filenameRegex.exactMatch(filename)) {
            qDebug() << "Found file: " << filename;
            if (!fileInfo.isReadable()) {
                qDebug() << "\tCan't open file for reading: " << filename;
                continue;
            }
        }
    }

    while (!_isFinished) {
        // since we're a while loop we need to help Qt's event processing
        QCoreApplication::processEvents();
    }
}

void AssetServer::handleAssetGet(QSharedPointer<NLPacket> packet, SharedNodePointer senderNode) {
    QByteArray assetHash;

    if (packet->getPayloadSize() < HASH_HEX_LENGTH) {
        qDebug() << "ERROR bad file request";
        return;
    }

    assetHash = packet->read(HASH_HEX_LENGTH);
    qDebug() << "Got a request for the file: " << assetHash;

    // We need to reply...
    QFile file { _resourcesDirectory.filePath(QString(assetHash)) };
    qDebug() << "Opening file: " << QString(QFileInfo(assetHash).fileName());
    bool found = file.open(QIODevice::ReadOnly);

    auto assetPacket = NLPacket::create(PacketType::AssetGetReply);

    assetPacket->write(assetHash, HASH_HEX_LENGTH);
    assetPacket->writePrimitive(found);

    const int MAX_LENGTH = 1024;

    if (found) {
        QByteArray data = file.read(MAX_LENGTH);
        assetPacket->writePrimitive(data.size());
        assetPacket->write(data, data.size());
    } else {
        qDebug() << "File not found";
    }

    auto nodeList = DependencyManager::get<NodeList>();
    nodeList->sendPacket(std::move(assetPacket), *senderNode);
}

void AssetServer::handleAssetUpload(QSharedPointer<NLPacket> packet, SharedNodePointer senderNode) {
    MessageID messageID;
    packet->readPrimitive(&messageID);

    char extensionLength;
    packet->readPrimitive(&extensionLength);

    char extension[extensionLength];
    packet->read(extension, extensionLength);

    qDebug() << "Got extension: " << extension;

    int fileSize;
    packet->readPrimitive(&fileSize);

    const int MAX_LENGTH = 1024;
    fileSize = std::min(MAX_LENGTH, fileSize);
    qDebug() << "Receiving a file of size " << fileSize;

    QByteArray data = packet->read(fileSize);

    QString hash = QString(QCryptographicHash::hash(data, QCryptographicHash::Md5).toHex());

    qDebug() << "Got data: (" << hash << ") " << data;

    QFile file { _resourcesDirectory.filePath(QString(hash)) };

    if (file.exists()) {
        qDebug() << "[WARNING] This file already exists";
    } else {
        file.open(QIODevice::WriteOnly);
        file.write(data);
    }

    auto replyPacket = NLPacket::create(PacketType::AssetUploadReply);

    replyPacket->writePrimitive(messageID);

    replyPacket->writePrimitive(true);
    replyPacket->write(hash.toLatin1().constData(), HASH_HEX_LENGTH);

    auto nodeList = DependencyManager::get<NodeList>();
    nodeList->sendPacket(std::move(replyPacket), *senderNode);
}
