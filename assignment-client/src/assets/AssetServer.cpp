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

#include <QBuffer>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QCoreApplication>
#include <QEventLoop>
#include <QRunnable>
#include <QString>

#include "NetworkLogging.h"
#include "NodeType.h"
#include "SendAssetTask.h"

const QString ASSET_SERVER_LOGGING_TARGET_NAME = "asset-server";

AssetServer::AssetServer(NLPacket& packet) :
    ThreadedAssignment(packet),
    _taskPool(this)
{

    // Most of the work will be I/O bound, reading from disk and constructing packet objects,
    // so the ideal is greater than the number of cores on the system.
    _taskPool.setMaxThreadCount(20);

    auto& packetReceiver = DependencyManager::get<NodeList>()->getPacketReceiver();
    packetReceiver.registerListener(PacketType::AssetGet, this, "handleAssetGet");
    packetReceiver.registerListener(PacketType::AssetGetInfo, this, "handleAssetGetInfo");
    packetReceiver.registerMessageListener(PacketType::AssetUpload, this, "handleAssetUpload");
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
    QRegExp filenameRegex { "^[a-f0-9]{" + QString::number(HASH_HEX_LENGTH) + "}(\\..+)?$" };
    for (const auto& fileInfo : files) {
        auto filename = fileInfo.fileName();
        if (!filenameRegex.exactMatch(filename)) {
            qDebug() << "Found file: " << filename;
            if (!fileInfo.isReadable()) {
                qDebug() << "\tCan't open file for reading: " << filename;
                continue;
            }

            // Read file
            QFile file { fileInfo.absoluteFilePath() };
            file.open(QFile::ReadOnly);
            QByteArray data = file.readAll();

            auto hash = hashData(data);

            qDebug() << "\tMoving " << filename << " to " << hash;

            file.rename(_resourcesDirectory.absoluteFilePath(hash) + "." + fileInfo.suffix());
        }
    }
}

void AssetServer::handleAssetGetInfo(QSharedPointer<NLPacket> packet, SharedNodePointer senderNode) {
    QByteArray assetHash;
    MessageID messageID;
    uint8_t extensionLength;

    if (packet->getPayloadSize() < qint64(HASH_HEX_LENGTH + sizeof(messageID) + sizeof(extensionLength))) {
        qDebug() << "ERROR bad file request";
        return;
    }

    packet->readPrimitive(&messageID);
    assetHash = packet->readWithoutCopy(HASH_HEX_LENGTH);
    packet->readPrimitive(&extensionLength);
    QByteArray extension = packet->read(extensionLength);

    auto replyPacket = NLPacket::create(PacketType::AssetGetInfoReply);

    replyPacket->writePrimitive(messageID);
    replyPacket->write(assetHash);

    QString fileName = QString(assetHash) + "." + extension;
    QFileInfo fileInfo { _resourcesDirectory.filePath(fileName) };

    if (fileInfo.exists() && fileInfo.isReadable()) {
        qDebug() << "Opening file: " << fileInfo.filePath();
        replyPacket->writePrimitive(AssetServerError::NO_ERROR);
        replyPacket->writePrimitive(fileInfo.size());
    } else {
        qDebug() << "Asset not found: " << assetHash;
        replyPacket->writePrimitive(AssetServerError::ASSET_NOT_FOUND);
    }

    auto nodeList = DependencyManager::get<NodeList>();
    nodeList->sendPacket(std::move(replyPacket), *senderNode);
}

void AssetServer::handleAssetGet(QSharedPointer<NLPacket> packet, SharedNodePointer senderNode) {
    MessageID messageID;
    QByteArray assetHash;
    uint8_t extensionLength;
    DataOffset start;
    DataOffset end;

    auto minSize = qint64(sizeof(messageID) + HASH_HEX_LENGTH + sizeof(extensionLength) + sizeof(start) + sizeof(end));
    if (packet->getPayloadSize() < minSize) {
        qDebug() << "ERROR bad file request";
        return;
    }

    packet->readPrimitive(&messageID);
    assetHash = packet->read(HASH_HEX_LENGTH);
    packet->readPrimitive(&extensionLength);
    QByteArray extension = packet->read(extensionLength);
    packet->readPrimitive(&start);
    packet->readPrimitive(&end);

    qDebug() << "Received a request for the file (" << messageID << "): " << assetHash << " from " << start << " to " << end;

    // Queue task
    QString filePath = _resourcesDirectory.filePath(QString(assetHash) + "." + QString(extension));
    auto task = new SendAssetTask(messageID, assetHash, filePath, start, end, senderNode);
    _taskPool.start(task);
}

void AssetServer::handleAssetUpload(QSharedPointer<NLPacketList> packetList, SharedNodePointer senderNode) {
    auto data = packetList->getMessage();
    QBuffer buffer { &data };
    buffer.open(QIODevice::ReadOnly);

    MessageID messageID;
    buffer.read(reinterpret_cast<char*>(&messageID), sizeof(messageID));

    uint8_t extensionLength;
    buffer.read(reinterpret_cast<char*>(&extensionLength), sizeof(extensionLength));

    QByteArray extension = buffer.read(extensionLength);

    qDebug() << "Got extension: " << extension;

    uint64_t fileSize;
    buffer.read(reinterpret_cast<char*>(&fileSize), sizeof(fileSize));

    qDebug() << "Receiving a file of size " << fileSize;

    auto replyPacket = NLPacket::create(PacketType::AssetUploadReply);
    replyPacket->writePrimitive(messageID);

    if (fileSize > MAX_UPLOAD_SIZE) {
        replyPacket->writePrimitive(AssetServerError::ASSET_TOO_LARGE);
    } else {
        QByteArray fileData = buffer.read(fileSize);

        QString hash = hashData(fileData);

        qDebug() << "Got data: (" << hash << ") ";

        QFile file { _resourcesDirectory.filePath(QString(hash)) + "." + QString(extension) };

        if (file.exists()) {
            qDebug() << "[WARNING] This file already exists: " << hash;
        } else {
            file.open(QIODevice::WriteOnly);
            file.write(fileData);
            file.close();
        }
        replyPacket->writePrimitive(AssetServerError::NO_ERROR);
        replyPacket->write(hash.toLatin1());
    }

    auto nodeList = DependencyManager::get<NodeList>();
    nodeList->sendPacket(std::move(replyPacket), *senderNode);
}

QString AssetServer::hashData(const QByteArray& data) {
    return QString(QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex());
}

