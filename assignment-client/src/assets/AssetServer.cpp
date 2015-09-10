//
//  AssetServer.cpp
//  assignment-client/src/assets
//
//  Created by Ryan Huffman on 2015/07/21
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#include "AssetServer.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QString>

#include "NetworkLogging.h"
#include "NodeType.h"
#include "SendAssetTask.h"
#include "UploadAssetTask.h"

const QString ASSET_SERVER_LOGGING_TARGET_NAME = "asset-server";

AssetServer::AssetServer(NLPacket& packet) :
    ThreadedAssignment(packet),
    _taskPool(this)
{

    // Most of the work will be I/O bound, reading from disk and constructing packet objects,
    // so the ideal is greater than the number of cores on the system.
    static const int TASK_POOL_THREAD_COUNT = 50;
    _taskPool.setMaxThreadCount(TASK_POOL_THREAD_COUNT);

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
    QRegExp filenameRegex { "^[a-f0-9]{" + QString::number(SHA256_HASH_HEX_LENGTH) + "}(\\..+)?$" };
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
            auto hexHash = hash.toHex();

            qDebug() << "\tMoving " << filename << " to " << hexHash;

            file.rename(_resourcesDirectory.absoluteFilePath(hexHash) + "." + fileInfo.suffix());
        }
    }
}

void AssetServer::handleAssetGetInfo(QSharedPointer<NLPacket> packet, SharedNodePointer senderNode) {
    QByteArray assetHash;
    MessageID messageID;
    uint8_t extensionLength;

    if (packet->getPayloadSize() < qint64(SHA256_HASH_LENGTH + sizeof(messageID) + sizeof(extensionLength))) {
        qDebug() << "ERROR bad file request";
        return;
    }

    packet->readPrimitive(&messageID);
    assetHash = packet->readWithoutCopy(SHA256_HASH_LENGTH);
    packet->readPrimitive(&extensionLength);
    QByteArray extension = packet->read(extensionLength);

    auto replyPacket = NLPacket::create(PacketType::AssetGetInfoReply);

    QByteArray hexHash = assetHash.toHex();

    replyPacket->writePrimitive(messageID);
    replyPacket->write(assetHash);

    QString fileName = QString(hexHash) + "." + extension;
    QFileInfo fileInfo { _resourcesDirectory.filePath(fileName) };

    if (fileInfo.exists() && fileInfo.isReadable()) {
        qDebug() << "Opening file: " << fileInfo.filePath();
        replyPacket->writePrimitive(AssetServerError::NoError);
        replyPacket->writePrimitive(fileInfo.size());
    } else {
        qDebug() << "Asset not found: " << QString(hexHash);
        replyPacket->writePrimitive(AssetServerError::AssetNotFound);
    }

    auto nodeList = DependencyManager::get<NodeList>();
    nodeList->sendPacket(std::move(replyPacket), *senderNode);
}

void AssetServer::handleAssetGet(QSharedPointer<NLPacket> packet, SharedNodePointer senderNode) {

    auto minSize = qint64(sizeof(MessageID) + SHA256_HASH_LENGTH + sizeof(uint8_t) + sizeof(DataOffset) + sizeof(DataOffset));
    
    if (packet->getPayloadSize() < minSize) {
        qDebug() << "ERROR bad file request";
        return;
    }

    // Queue task
    auto task = new SendAssetTask(packet, senderNode, _resourcesDirectory);
    _taskPool.start(task);
}

void AssetServer::handleAssetUpload(QSharedPointer<NLPacketList> packetList, SharedNodePointer senderNode) {
    
    if (senderNode->getCanRez()) {
        qDebug() << "Starting an UploadAssetTask for upload from" << uuidStringWithoutCurlyBraces(senderNode->getUUID());
        
        auto task = new UploadAssetTask(packetList, senderNode, _resourcesDirectory);
        _taskPool.start(task);
    } else {
        // this is a node the domain told us is not allowed to rez entities
        // for now this also means it isn't allowed to add assets
        // so return a packet with error that indicates that
        
        auto permissionErrorPacket = NLPacket::create(PacketType::AssetUploadReply, sizeof(MessageID) + sizeof(AssetServerError));
        
        MessageID messageID;
        packetList->readPrimitive(&messageID);
        
        // write the message ID and a permission denied error
        permissionErrorPacket->writePrimitive(messageID);
        permissionErrorPacket->writePrimitive(AssetServerError::PermissionDenied);
        
        // send off the packet
        auto nodeList = DependencyManager::get<NodeList>();
        nodeList->sendPacket(std::move(permissionErrorPacket), *senderNode);
    }
}

void AssetServer::sendStatsPacket() {
    QJsonObject statsObject;
    
    auto stats = DependencyManager::get<NodeList>()->sampleStatsForAllConnections();
    
    QString baseName("AssetServer");
    
    statsObject[baseName + ".num_connections"] = (int)stats.size();
    for (const auto& stat : stats) {
        QString uuid = QUuid().toString();
        if (auto node = DependencyManager::get<NodeList>()->findNodeWithAddr(stat.first)) {
            uuid = node->getUUID().toString();
        }
        
        QString connKey = baseName + "." + uuid + ".connection.";
        QString sendKey = baseName + "." + uuid + ".sending.";
        QString recvKey = baseName + "." + uuid + ".receiving.";
        qDebug() << "Testing" << sendKey << recvKey;
        
        auto endTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(stat.second.endTime);
        QDateTime date = QDateTime::fromMSecsSinceEpoch(endTimeMs.count());
        
        statsObject[connKey + "lastHeard"] = date.toString();
        statsObject[connKey + "estimatedBandwith"] = stat.second.estimatedBandwith;
        statsObject[connKey + "rtt"] = stat.second.rtt;
        statsObject[connKey + "congestionWindowSize"] = stat.second.congestionWindowSize;
        statsObject[connKey + "packetSendPeriod"] = stat.second.packetSendPeriod;
        
        statsObject[sendKey + "sendRate"] = stat.second.sendRate;
        statsObject[sendKey + "sentPackets"] = stat.second.sentPackets;
        statsObject[sendKey + "receivedACK"] = stat.second.events[udt::ConnectionStats::Stats::ReceivedACK];
        statsObject[sendKey + "processedACK"] = stat.second.events[udt::ConnectionStats::Stats::ProcessedACK];
        statsObject[sendKey + "receivedLightACK"] = stat.second.events[udt::ConnectionStats::Stats::ReceivedLightACK];
        statsObject[sendKey + "receivedNAK"] = stat.second.events[udt::ConnectionStats::Stats::ReceivedNAK];
        statsObject[sendKey + "receivedTimeoutNAK"] = stat.second.events[udt::ConnectionStats::Stats::ReceivedTimeoutNAK];
        statsObject[sendKey + "sentACK2"] = stat.second.events[udt::ConnectionStats::Stats::SentACK2];
        statsObject[sendKey + "retransmission"] = stat.second.events[udt::ConnectionStats::Stats::Retransmission];
        
        statsObject[recvKey + "receiveRate"] = stat.second.receiveRate;
        statsObject[recvKey + "receivedPackets"] = stat.second.receivedPackets;
        statsObject[recvKey + "SentACK"] = stat.second.events[udt::ConnectionStats::Stats::SentACK];
        statsObject[recvKey + "SentLightACK"] = stat.second.events[udt::ConnectionStats::Stats::SentLightACK];
        statsObject[recvKey + "SentNAK"] = stat.second.events[udt::ConnectionStats::Stats::SentNAK];
        statsObject[recvKey + "SentTimeoutNAK"] = stat.second.events[udt::ConnectionStats::Stats::SentTimeoutNAK];
        statsObject[recvKey + "ReceivedACK2"] = stat.second.events[udt::ConnectionStats::Stats::ReceivedACK2];
        statsObject[recvKey + "Duplicate"] = stat.second.events[udt::ConnectionStats::Stats::Duplicate];
    }
    
    // send off the stats packets
    ThreadedAssignment::addPacketStatsAndSendStatsPacket(statsObject);
}
