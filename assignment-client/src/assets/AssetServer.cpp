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

#include <QtCore/QCoreApplication>
#include <QtCore/QCryptographicHash>
#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonDocument>
#include <QtCore/QString>

#include "NetworkLogging.h"
#include "NodeType.h"
#include "SendAssetTask.h"
#include "UploadAssetTask.h"
#include <ServerPathUtils.h>

const QString ASSET_SERVER_LOGGING_TARGET_NAME = "asset-server";

AssetServer::AssetServer(ReceivedMessage& message) :
    ThreadedAssignment(message),
    _taskPool(this)
{

    // Most of the work will be I/O bound, reading from disk and constructing packet objects,
    // so the ideal is greater than the number of cores on the system.
    static const int TASK_POOL_THREAD_COUNT = 50;
    _taskPool.setMaxThreadCount(TASK_POOL_THREAD_COUNT);

    auto& packetReceiver = DependencyManager::get<NodeList>()->getPacketReceiver();
    packetReceiver.registerListener(PacketType::AssetGet, this, "handleAssetGet");
    packetReceiver.registerListener(PacketType::AssetGetInfo, this, "handleAssetGetInfo");
    packetReceiver.registerListener(PacketType::AssetUpload, this, "handleAssetUpload");
    packetReceiver.registerListener(PacketType::AssetMappingOperation, this, "handleAssetMappingOperation");
}

void AssetServer::run() {

    qDebug() << "Waiting for connection to domain to request settings from domain-server.";

    // wait until we have the domain-server settings, otherwise we bail
    DomainHandler& domainHandler = DependencyManager::get<NodeList>()->getDomainHandler();
    connect(&domainHandler, &DomainHandler::settingsReceived, this, &AssetServer::completeSetup);
    connect(&domainHandler, &DomainHandler::settingsReceiveFail, this, &AssetServer::domainSettingsRequestFailed);

    ThreadedAssignment::commonInit(ASSET_SERVER_LOGGING_TARGET_NAME, NodeType::AssetServer);
}

static const QString ASSET_FILES_SUBDIR = "files";

void AssetServer::completeSetup() {
    auto nodeList = DependencyManager::get<NodeList>();

    auto& domainHandler = nodeList->getDomainHandler();
    const QJsonObject& settingsObject = domainHandler.getSettingsObject();

    static const QString ASSET_SERVER_SETTINGS_KEY = "asset_server";

    if (!settingsObject.contains(ASSET_SERVER_SETTINGS_KEY)) {
        qCritical() << "Received settings from the domain-server with no asset-server section. Stopping assignment.";
        setFinished(true);
        return;
    }

    auto assetServerObject = settingsObject[ASSET_SERVER_SETTINGS_KEY].toObject();

    // get the path to the asset folder from the domain server settings
    static const QString ASSETS_PATH_OPTION = "assets_path";
    auto assetsJSONValue = assetServerObject[ASSETS_PATH_OPTION];

    if (!assetsJSONValue.isString()) {
        qCritical() << "Received an assets path from the domain-server that could not be parsed. Stopping assignment.";
        setFinished(true);
        return;
    }

    auto assetsPathString = assetsJSONValue.toString();
    QDir assetsPath { assetsPathString };
    QString absoluteFilePath = assetsPath.absolutePath();

    if (assetsPath.isRelative()) {
        // if the domain settings passed us a relative path, make an absolute path that is relative to the
        // default data directory
        absoluteFilePath = ServerPathUtils::getDataFilePath("assets/" + assetsPathString);
    }

    _resourcesDirectory = QDir(absoluteFilePath);

    qDebug() << "Creating resources directory";
    _resourcesDirectory.mkpath(".");
    _resourcesDirectory.mkpath(ASSET_FILES_SUBDIR);

    _filesDirectory = _resourcesDirectory;
    _filesDirectory.cd(ASSET_FILES_SUBDIR);

    // load whatever mappings we currently have from the local file
    loadMappingsFromFile();
    
    qInfo() << "Serving files from: " << _filesDirectory.path();

    // Check the asset directory to output some information about what we have
    auto files = _filesDirectory.entryList(QDir::Files);

    QRegExp hashFileRegex { "^[a-f0-9]{" + QString::number(SHA256_HASH_HEX_LENGTH) + "}$" };
    auto hashedFiles = files.filter(hashFileRegex);

    qInfo() << "There are" << hashedFiles.size() << "asset files in the asset directory.";

    performMappingMigration();

    nodeList->addNodeTypeToInterestSet(NodeType::Agent);
}

void AssetServer::performMappingMigration() {
    QRegExp hashFileRegex { "^[a-f0-9]{" + QString::number(SHA256_HASH_HEX_LENGTH) + "}(\\.[\\w]+)+$" };

    auto files = _resourcesDirectory.entryInfoList(QDir::Files);

    for (const auto& fileInfo : files) {
        if (hashFileRegex.exactMatch(fileInfo.fileName())) {
            // we have a pre-mapping file that we should migrate to the new mapping system
            qDebug() << "Migrating pre-mapping file" << fileInfo.fileName();

            // rename the file to the same name with no extension
            QFile oldFile { fileInfo.absoluteFilePath() };

            auto oldAbsolutePath = fileInfo.absoluteFilePath();
            auto oldFilename = fileInfo.fileName();
            auto hash = oldFilename.left(SHA256_HASH_HEX_LENGTH);
            auto fullExtension = oldFilename.mid(oldFilename.indexOf('.'));

            qDebug() << "\tMoving" << oldAbsolutePath << "to" << oldAbsolutePath.replace(fullExtension, "");

            bool renamed = oldFile.copy(_filesDirectory.filePath(hash));
            if (!renamed) {
                qWarning() << "\tCould not migrate pre-mapping file" << fileInfo.fileName();
            } else {
                qDebug() << "\tRenamed pre-mapping file" << fileInfo.fileName();

                // add a new mapping with the old extension and a truncated version of the hash
                static const int TRUNCATED_HASH_NUM_CHAR = 16;
                auto fakeFileName = hash.left(TRUNCATED_HASH_NUM_CHAR) + fullExtension;

                qDebug() << "\tAdding a migration mapping from" << fakeFileName << "to" << hash;

                auto it = _fileMappings.find(fakeFileName);
                if (it == _fileMappings.end()) {
                    _fileMappings[fakeFileName] = hash;


                    if (writeMappingsToFile()) {
                        // mapping added and persisted, we can remove the migrated file
                        oldFile.remove();
                        qDebug() << "\tMigration completed for" << oldFilename;
                    }
                } else {
                    qDebug() << "\tCould not add migration mapping for" << hash << "since a mapping for" << fakeFileName
                        << "already exists.";
                }
            }
        }
    }
}

void AssetServer::handleAssetMappingOperation(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode) {
    MessageID messageID;
    message->readPrimitive(&messageID);

    AssetMappingOperationType operationType;
    message->readPrimitive(&operationType);

    auto replyPacket = NLPacketList::create(PacketType::AssetMappingOperationReply, QByteArray(), true, true);
    replyPacket->writePrimitive(messageID);

    switch (operationType) {
        case AssetMappingOperationType::Get: {
            handleGetMappingOperation(*message, senderNode, *replyPacket);
            break;
        }
        case AssetMappingOperationType::GetAll: {
            handleGetAllMappingOperation(*message, senderNode, *replyPacket);
            break;
        }
        case AssetMappingOperationType::Set: {
            handleSetMappingOperation(*message, senderNode, *replyPacket);
            break;
        }
        case AssetMappingOperationType::Delete: {
            handleDeleteMappingOperation(*message, senderNode, *replyPacket);
            break;
        }
    }

    auto nodeList = DependencyManager::get<NodeList>();
    nodeList->sendPacketList(std::move(replyPacket), *senderNode);
}

void AssetServer::handleGetMappingOperation(ReceivedMessage& message, SharedNodePointer senderNode, NLPacketList& replyPacket) {
    QString assetPath = message.readString();

    auto it = _fileMappings.find(assetPath);
    if (it != _fileMappings.end()) {
        auto assetHash = it->toString();
        qDebug() << "Found mapping for: " << assetPath << "=>" << assetHash;
        replyPacket.writePrimitive(AssetServerError::NoError);
        replyPacket.write(QByteArray::fromHex(assetHash.toUtf8()));
    }
    else {
        qDebug() << "Mapping not found for: " << assetPath;
        replyPacket.writePrimitive(AssetServerError::AssetNotFound);
    }
}

void AssetServer::handleGetAllMappingOperation(ReceivedMessage& message, SharedNodePointer senderNode, NLPacketList& replyPacket) {
    replyPacket.writePrimitive(AssetServerError::NoError);

    auto count = _fileMappings.size();

    replyPacket.writePrimitive(count);

    for (auto it = _fileMappings.cbegin(); it != _fileMappings.cend(); ++ it) {
        replyPacket.writeString(it.key());
        replyPacket.write(QByteArray::fromHex(it.value().toString().toUtf8()));
    }
}

void AssetServer::handleSetMappingOperation(ReceivedMessage& message, SharedNodePointer senderNode, NLPacketList& replyPacket) {
    if (senderNode->getCanRez()) {
        QString assetPath = message.readString();
        auto assetHash = message.read(SHA256_HASH_LENGTH).toHex();

        if (setMapping(assetPath, assetHash)) {
            replyPacket.writePrimitive(AssetServerError::NoError);
        } else {
            replyPacket.writePrimitive(AssetServerError::MappingOperationFailed);
        }
    } else {
        replyPacket.writePrimitive(AssetServerError::PermissionDenied);
    }
}

void AssetServer::handleDeleteMappingOperation(ReceivedMessage& message, SharedNodePointer senderNode, NLPacketList& replyPacket) {
    if (senderNode->getCanRez()) {
        QString assetPath = message.readString();

        if (deleteMapping(assetPath)) {
            replyPacket.writePrimitive(AssetServerError::NoError);
        } else {
            replyPacket.writePrimitive(AssetServerError::MappingOperationFailed);
        }
    } else {
        replyPacket.writePrimitive(AssetServerError::PermissionDenied);
    }
}

void AssetServer::handleAssetGetInfo(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode) {
    QByteArray assetHash;
    MessageID messageID;

    if (message->getSize() < qint64(SHA256_HASH_LENGTH + sizeof(messageID))) {
        qDebug() << "ERROR bad file request";
        return;
    }

    message->readPrimitive(&messageID);
    assetHash = message->readWithoutCopy(SHA256_HASH_LENGTH);

    auto replyPacket = NLPacket::create(PacketType::AssetGetInfoReply);

    QByteArray hexHash = assetHash.toHex();

    replyPacket->writePrimitive(messageID);
    replyPacket->write(assetHash);

    QString fileName = QString(hexHash);
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

void AssetServer::handleAssetGet(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode) {

    auto minSize = qint64(sizeof(MessageID) + SHA256_HASH_LENGTH + sizeof(DataOffset) + sizeof(DataOffset));
    
    if (message->getSize() < minSize) {
        qDebug() << "ERROR bad file request";
        return;
    }

    // Queue task
    auto task = new SendAssetTask(message, senderNode, _filesDirectory);
    _taskPool.start(task);
}

void AssetServer::handleAssetUpload(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode) {
    
    if (senderNode->getCanRez()) {
        qDebug() << "Starting an UploadAssetTask for upload from" << uuidStringWithoutCurlyBraces(senderNode->getUUID());
        
        auto task = new UploadAssetTask(message, senderNode, _filesDirectory);
        _taskPool.start(task);
    } else {
        // this is a node the domain told us is not allowed to rez entities
        // for now this also means it isn't allowed to add assets
        // so return a packet with error that indicates that
        
        auto permissionErrorPacket = NLPacket::create(PacketType::AssetUploadReply, sizeof(MessageID) + sizeof(AssetServerError));
        
        MessageID messageID;
        message->readPrimitive(&messageID);
        
        // write the message ID and a permission denied error
        permissionErrorPacket->writePrimitive(messageID);
        permissionErrorPacket->writePrimitive(AssetServerError::PermissionDenied);
        
        // send off the packet
        auto nodeList = DependencyManager::get<NodeList>();
        nodeList->sendPacket(std::move(permissionErrorPacket), *senderNode);
    }
}

void AssetServer::sendStatsPacket() {
    QJsonObject serverStats;
    
    auto stats = DependencyManager::get<NodeList>()->sampleStatsForAllConnections();
    
    for (const auto& stat : stats) {
        QJsonObject nodeStats;
        auto endTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(stat.second.endTime);
        QDateTime date = QDateTime::fromMSecsSinceEpoch(endTimeMs.count());
        
        static const float USEC_PER_SEC = 1000000.0f;
        static const float MEGABITS_PER_BYTE = 8.0f / 1000000.0f; // Bytes => Mbits
        float elapsed = (float)(stat.second.endTime - stat.second.startTime).count() / USEC_PER_SEC; // sec
        float megabitsPerSecPerByte = MEGABITS_PER_BYTE / elapsed; // Bytes => Mb/s
        
        QJsonObject connectionStats;
        connectionStats["1. Last Heard"] = date.toString();
        connectionStats["2. Est. Max (P/s)"] = stat.second.estimatedBandwith;
        connectionStats["3. RTT (ms)"] = stat.second.rtt;
        connectionStats["4. CW (P)"] = stat.second.congestionWindowSize;
        connectionStats["5. Period (us)"] = stat.second.packetSendPeriod;
        connectionStats["6. Up (Mb/s)"] = stat.second.sentBytes * megabitsPerSecPerByte;
        connectionStats["7. Down (Mb/s)"] = stat.second.receivedBytes * megabitsPerSecPerByte;
        nodeStats["Connection Stats"] = connectionStats;
        
        using Events = udt::ConnectionStats::Stats::Event;
        const auto& events = stat.second.events;
        
        QJsonObject upstreamStats;
        upstreamStats["1. Sent (P/s)"] = stat.second.sendRate;
        upstreamStats["2. Sent Packets"] = stat.second.sentPackets;
        upstreamStats["3. Recvd ACK"] = events[Events::ReceivedACK];
        upstreamStats["4. Procd ACK"] = events[Events::ProcessedACK];
        upstreamStats["5. Recvd LACK"] = events[Events::ReceivedLightACK];
        upstreamStats["6. Recvd NAK"] = events[Events::ReceivedNAK];
        upstreamStats["7. Recvd TNAK"] = events[Events::ReceivedTimeoutNAK];
        upstreamStats["8. Sent ACK2"] = events[Events::SentACK2];
        upstreamStats["9. Retransmitted"] = events[Events::Retransmission];
        nodeStats["Upstream Stats"] = upstreamStats;
        
        QJsonObject downstreamStats;
        downstreamStats["1. Recvd (P/s)"] = stat.second.receiveRate;
        downstreamStats["2. Recvd Packets"] = stat.second.receivedPackets;
        downstreamStats["3. Sent ACK"] = events[Events::SentACK];
        downstreamStats["4. Sent LACK"] = events[Events::SentLightACK];
        downstreamStats["5. Sent NAK"] = events[Events::SentNAK];
        downstreamStats["6. Sent TNAK"] = events[Events::SentTimeoutNAK];
        downstreamStats["7. Recvd ACK2"] = events[Events::ReceivedACK2];
        downstreamStats["8. Duplicates"] = events[Events::Duplicate];
        nodeStats["Downstream Stats"] = downstreamStats;
        
        QString uuid;
        auto nodelist = DependencyManager::get<NodeList>();
        if (stat.first == nodelist->getDomainHandler().getSockAddr()) {
            uuid = uuidStringWithoutCurlyBraces(nodelist->getDomainHandler().getUUID());
            nodeStats[USERNAME_UUID_REPLACEMENT_STATS_KEY] = "DomainServer";
        } else {
            auto node = nodelist->findNodeWithAddr(stat.first);
            uuid = uuidStringWithoutCurlyBraces(node ? node->getUUID() : QUuid());
            nodeStats[USERNAME_UUID_REPLACEMENT_STATS_KEY] = uuid;
        }
        
        serverStats[uuid] = nodeStats;
    }
    
    // send off the stats packets
    ThreadedAssignment::addPacketStatsAndSendStatsPacket(serverStats);
}

static const QString MAP_FILE_NAME = "map.json";

void AssetServer::loadMappingsFromFile() {

    auto mapFilePath = _resourcesDirectory.absoluteFilePath(MAP_FILE_NAME);

    QFile mapFile { mapFilePath };
    if (mapFile.exists()) {
        if (mapFile.open(QIODevice::ReadOnly)) {
            auto jsonDocument = QJsonDocument::fromJson(mapFile.readAll());
            _fileMappings = jsonDocument.object().toVariantHash();

            qInfo() << "Loaded" << _fileMappings.count() << "mappings from map file at" << mapFilePath;
        } else {
            qCritical() << "Failed to read mapping file at" << mapFilePath << "- assignment with not continue.";
            setFinished(true);
        }
    } else {
        qInfo() << "No existing mappings loaded from file since no file was found at" << mapFilePath;
    }
}

bool AssetServer::writeMappingsToFile() {
    auto mapFilePath = _resourcesDirectory.absoluteFilePath(MAP_FILE_NAME);

    QFile mapFile { mapFilePath };
    if (mapFile.open(QIODevice::WriteOnly)) {
        auto jsonObject = QJsonObject::fromVariantHash(_fileMappings);
        QJsonDocument jsonDocument { jsonObject };

        if (mapFile.write(jsonDocument.toJson()) != -1) {
            qDebug() << "Wrote JSON mappings to file at" << mapFilePath;
            return true;
        } else {
            qWarning() << "Failed to write JSON mappings to file at" << mapFilePath;
        }
    } else {
        qWarning() << "Failed to open map file at" << mapFilePath;
    }

    return false;
}

AssetHash AssetServer::getMapping(AssetPath path) {
    return _fileMappings.value(path).toString();
}

bool AssetServer::setMapping(AssetPath path, AssetHash hash) {
    // remember what the old mapping was in case persistence fails
    auto oldMapping = _fileMappings.value(path).toString();

    // update the in memory QHash
    _fileMappings[path] = hash;

    // attempt to write to file
    if (writeMappingsToFile()) {
        // persistence succeeded, we are good to go
        return true;
    } else {
        // failed to persist this mapping to file - put back the old one in our in-memory representation
        if (oldMapping.isEmpty()) {
            _fileMappings.remove(path);
        } else {
            _fileMappings[path] = oldMapping;
        }

        return false;
    }
}

bool AssetServer::deleteMapping(AssetPath path) {
    // keep the old mapping in case the delete fails
    auto oldMapping = _fileMappings.take(path);

    if (!oldMapping.isNull()) {
        // deleted the old mapping, attempt to persist to file
        if (writeMappingsToFile()) {
            // persistence succeeded we are good to go
            return true;
        } else {
            // we didn't delete the previous mapping, put it back in our in-memory representation
            _fileMappings[path] = oldMapping.toString();
        }
    }

    return false;
}
