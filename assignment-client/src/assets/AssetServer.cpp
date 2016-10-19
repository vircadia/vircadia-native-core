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

#include <ServerPathUtils.h>

#include "NetworkLogging.h"
#include "NodeType.h"
#include "SendAssetTask.h"
#include "UploadAssetTask.h"

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

    static const QString MAX_BANDWIDTH_OPTION = "max_bandwidth";
    auto maxBandwidthValue = assetServerObject[MAX_BANDWIDTH_OPTION];
    auto maxBandwidthFloat = maxBandwidthValue.toDouble(-1);

    if (maxBandwidthFloat > 0.0) {
        const int BITS_PER_MEGABITS = 1000 * 1000;
        int maxBandwidth = maxBandwidthFloat * BITS_PER_MEGABITS;
        nodeList->setConnectionMaxBandwidth(maxBandwidth);
        qInfo() << "Set maximum bandwith per connection to" << maxBandwidthFloat << "Mb/s."
                    " (" << maxBandwidth << "bits/s)";
    }

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
    _filesDirectory = _resourcesDirectory;

    if (!_resourcesDirectory.mkpath(ASSET_FILES_SUBDIR) || !_filesDirectory.cd(ASSET_FILES_SUBDIR)) {
        qCritical() << "Unable to create file directory for asset-server files. Stopping assignment.";
        setFinished(true);
        return;
    }

    // load whatever mappings we currently have from the local file
    if (loadMappingsFromFile()) {
        qInfo() << "Serving files from: " << _filesDirectory.path();

        // Check the asset directory to output some information about what we have
        auto files = _filesDirectory.entryList(QDir::Files);

        QRegExp hashFileRegex { ASSET_HASH_REGEX_STRING };
        auto hashedFiles = files.filter(hashFileRegex);

        qInfo() << "There are" << hashedFiles.size() << "asset files in the asset directory.";

        if (_fileMappings.count() > 0) {
            cleanupUnmappedFiles();
        }

        nodeList->addNodeTypeToInterestSet(NodeType::Agent);
    } else {
        qCritical() << "Asset Server assignment will not continue because mapping file could not be loaded.";
        setFinished(true);
    }

}

void AssetServer::cleanupUnmappedFiles() {
    QRegExp hashFileRegex { "^[a-f0-9]{" + QString::number(SHA256_HASH_HEX_LENGTH) + "}" };

    auto files = _filesDirectory.entryInfoList(QDir::Files);

    // grab the currently mapped hashes
    auto mappedHashes = _fileMappings.values();

    qInfo() << "Performing unmapped asset cleanup.";

    for (const auto& fileInfo : files) {
        if (hashFileRegex.exactMatch(fileInfo.fileName())) {
            if (!mappedHashes.contains(fileInfo.fileName())) {
                // remove the unmapped file
                QFile removeableFile { fileInfo.absoluteFilePath() };

                if (removeableFile.remove()) {
                    qDebug() << "\tDeleted" << fileInfo.fileName() << "from asset files directory since it is unmapped.";
                } else {
                    qDebug() << "\tAttempt to delete unmapped file" << fileInfo.fileName() << "failed";
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
            handleDeleteMappingsOperation(*message, senderNode, *replyPacket);
            break;
        }
        case AssetMappingOperationType::Rename: {
            handleRenameMappingOperation(*message, senderNode, *replyPacket);
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
        replyPacket.writePrimitive(AssetServerError::NoError);
        replyPacket.write(QByteArray::fromHex(assetHash.toUtf8()));
    } else {
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
    if (senderNode->getCanWriteToAssetServer()) {
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

void AssetServer::handleDeleteMappingsOperation(ReceivedMessage& message, SharedNodePointer senderNode, NLPacketList& replyPacket) {
    if (senderNode->getCanWriteToAssetServer()) {
        int numberOfDeletedMappings { 0 };
        message.readPrimitive(&numberOfDeletedMappings);

        QStringList mappingsToDelete;

        for (int i = 0; i < numberOfDeletedMappings; ++i) {
            mappingsToDelete << message.readString();
        }

        if (deleteMappings(mappingsToDelete)) {
            replyPacket.writePrimitive(AssetServerError::NoError);
        } else {
            replyPacket.writePrimitive(AssetServerError::MappingOperationFailed);
        }
    } else {
        replyPacket.writePrimitive(AssetServerError::PermissionDenied);
    }
}

void AssetServer::handleRenameMappingOperation(ReceivedMessage& message, SharedNodePointer senderNode, NLPacketList& replyPacket) {
    if (senderNode->getCanWriteToAssetServer()) {
        QString oldPath = message.readString();
        QString newPath = message.readString();

        if (renameMapping(oldPath, newPath)) {
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

    auto size = qint64(sizeof(MessageID) + SHA256_HASH_LENGTH + sizeof(AssetServerError) + sizeof(qint64));
    auto replyPacket = NLPacket::create(PacketType::AssetGetInfoReply, size, true);

    QByteArray hexHash = assetHash.toHex();

    replyPacket->writePrimitive(messageID);
    replyPacket->write(assetHash);

    QString fileName = QString(hexHash);
    QFileInfo fileInfo { _filesDirectory.filePath(fileName) };

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

    if (senderNode->getCanWriteToAssetServer()) {
        qDebug() << "Starting an UploadAssetTask for upload from" << uuidStringWithoutCurlyBraces(senderNode->getUUID());

        auto task = new UploadAssetTask(message, senderNode, _filesDirectory);
        _taskPool.start(task);
    } else {
        // this is a node the domain told us is not allowed to rez entities
        // for now this also means it isn't allowed to add assets
        // so return a packet with error that indicates that

        auto permissionErrorPacket = NLPacket::create(PacketType::AssetUploadReply, sizeof(MessageID) + sizeof(AssetServerError), true);

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

bool AssetServer::loadMappingsFromFile() {

    auto mapFilePath = _resourcesDirectory.absoluteFilePath(MAP_FILE_NAME);

    QFile mapFile { mapFilePath };
    if (mapFile.exists()) {
        if (mapFile.open(QIODevice::ReadOnly)) {
            QJsonParseError error;

            auto jsonDocument = QJsonDocument::fromJson(mapFile.readAll(), &error);

            if (error.error == QJsonParseError::NoError) {
                _fileMappings = jsonDocument.object().toVariantHash();

                // remove any mappings that don't match the expected format
                auto it = _fileMappings.begin();
                while (it != _fileMappings.end()) {
                    bool shouldDrop = false;

                    if (!isValidFilePath(it.key())) {
                        qWarning() << "Will not keep mapping for" << it.key() << "since it is not a valid path.";
                        shouldDrop = true;
                    }

                    if (!isValidHash(it.value().toString())) {
                        qWarning() << "Will not keep mapping for" << it.key() << "since it does not have a valid hash.";
                        shouldDrop = true;
                    }

                    if (shouldDrop) {
                        it = _fileMappings.erase(it);
                    } else {
                        ++it;
                    }
                }

                qInfo() << "Loaded" << _fileMappings.count() << "mappings from map file at" << mapFilePath;
                return true;
            }
        }

        qCritical() << "Failed to read mapping file at" << mapFilePath;
        return false;
    } else {
        qInfo() << "No existing mappings loaded from file since no file was found at" << mapFilePath;
    }

    return true;
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

bool AssetServer::setMapping(AssetPath path, AssetHash hash) {
    path = path.trimmed();

    if (!isValidFilePath(path)) {
        qWarning() << "Cannot set a mapping for invalid path:" << path << "=>" << hash;
        return false;
    }

    if (!isValidHash(hash)) {
        qWarning() << "Cannot set a mapping for invalid hash" << path << "=>" << hash;
        return false;
    }

    // remember what the old mapping was in case persistence fails
    auto oldMapping = _fileMappings.value(path).toString();

    // update the in memory QHash
    _fileMappings[path] = hash;

    // attempt to write to file
    if (writeMappingsToFile()) {
        // persistence succeeded, we are good to go
        qDebug() << "Set mapping:" << path << "=>" << hash;
        return true;
    } else {
        // failed to persist this mapping to file - put back the old one in our in-memory representation
        if (oldMapping.isEmpty()) {
            _fileMappings.remove(path);
        } else {
            _fileMappings[path] = oldMapping;
        }

        qWarning() << "Failed to persist mapping:" << path << "=>" << hash;

        return false;
    }
}

bool pathIsFolder(const AssetPath& path) {
    return path.endsWith('/');
}

bool AssetServer::deleteMappings(AssetPathList& paths) {
    // take a copy of the current mappings in case persistence of these deletes fails
    auto oldMappings = _fileMappings;

    QSet<QString> hashesToCheckForDeletion;

    // enumerate the paths to delete and remove them all
    for (auto& path : paths) {

        path = path.trimmed();

        // figure out if this path will delete a file or folder
        if (pathIsFolder(path)) {
            // enumerate the in memory file mappings and remove anything that matches
            auto it = _fileMappings.begin();
            auto sizeBefore = _fileMappings.size();

            while (it != _fileMappings.end()) {
                if (it.key().startsWith(path)) {
                    // add this hash to the list we need to check for asset removal from the server
                    hashesToCheckForDeletion << it.value().toString();

                    it = _fileMappings.erase(it);
                } else {
                    ++it;
                }
            }

            auto sizeNow = _fileMappings.size();
            if (sizeBefore != sizeNow) {
                qDebug() << "Deleted" << sizeBefore - sizeNow << "mappings in folder: " << path;
            } else {
                qDebug() << "Did not find any mappings to delete in folder:" << path;
            }

        } else {
            auto oldMapping = _fileMappings.take(path);
            if (!oldMapping.isNull()) {
                // add this hash to the list we need to check for asset removal from server
                hashesToCheckForDeletion << oldMapping.toString();

                qDebug() << "Deleted a mapping:" << path << "=>" << oldMapping.toString();
            } else {
                qDebug() << "Unable to delete a mapping that was not found:" << path;
            }
        }
    }

    // deleted the old mappings, attempt to persist to file
    if (writeMappingsToFile()) {
        // persistence succeeded we are good to go

        // grab the current mapped hashes
        auto mappedHashes = _fileMappings.values();

        // enumerate the mapped hashes and clear the list of hashes to check for anything that's present
        for (auto& hashVariant : mappedHashes) {
            auto it = hashesToCheckForDeletion.find(hashVariant.toString());
            if (it != hashesToCheckForDeletion.end()) {
                hashesToCheckForDeletion.erase(it);
            }
        }

        // we now have a set of hashes that are unmapped - we will delete those asset files
        for (auto& hash : hashesToCheckForDeletion) {
            // remove the unmapped file
            QFile removeableFile { _filesDirectory.absoluteFilePath(hash) };

            if (removeableFile.remove()) {
                qDebug() << "\tDeleted" << hash << "from asset files directory since it is now unmapped.";
            } else {
                qDebug() << "\tAttempt to delete unmapped file" << hash << "failed";
            }
        }

        return true;
    } else {
        qWarning() << "Failed to persist deleted mappings, rolling back";

        // we didn't delete the previous mapping, put it back in our in-memory representation
        _fileMappings = oldMappings;

        return false;
    }
}

bool AssetServer::renameMapping(AssetPath oldPath, AssetPath newPath) {
    oldPath = oldPath.trimmed();
    newPath = newPath.trimmed();

    if (!isValidFilePath(oldPath) || !isValidFilePath(newPath)) {
        qWarning() << "Cannot perform rename with invalid paths - both should have leading forward slashes:"
            << oldPath << "=>" << newPath;

        return false;
    }

    // figure out if this rename is for a file or folder
    if (pathIsFolder(oldPath)) {
        if (!pathIsFolder(newPath)) {
            // we were asked to rename a path to a folder to a path that isn't a folder, this is a fail
            qWarning() << "Cannot rename mapping from folder path" << oldPath << "to file path" << newPath;

            return false;
        }

        // take a copy of the old mappings
        auto oldMappings = _fileMappings;

        // iterate the current mappings and adjust any that matches the renamed folder
        auto it = oldMappings.begin();

        while (it != oldMappings.end()) {
            if (it.key().startsWith(oldPath)) {
                auto newKey = it.key();
                newKey.replace(0, oldPath.size(), newPath);

                // remove the old version from the in memory file mappings
                _fileMappings.remove(it.key());
                _fileMappings.insert(newKey, it.value());
            }

            ++it;
        }

        if (writeMappingsToFile()) {
            // persisted the changed mappings, return success
            qDebug() << "Renamed folder mapping:" << oldPath << "=>" << newPath;

            return true;
        } else {
            // couldn't persist the renamed paths, rollback and return failure
            _fileMappings = oldMappings;

            qWarning() << "Failed to persist renamed folder mapping:" << oldPath << "=>" << newPath;

            return false;
        }
    } else {
        if (pathIsFolder(newPath)) {
            // we were asked to rename a path to a file to a path that is a folder, this is a fail
            qWarning() << "Cannot rename mapping from file path" << oldPath << "to folder path" << newPath;

            return false;
        }

        // take the old hash to remove the old mapping
        auto oldSourceMapping = _fileMappings.take(oldPath).toString();

        // in case we're overwriting, keep the current destination mapping for potential rollback
        auto oldDestinationMapping = _fileMappings.value(newPath);

        if (!oldSourceMapping.isEmpty()) {
            _fileMappings[newPath] = oldSourceMapping;

            if (writeMappingsToFile()) {
                // persisted the renamed mapping, return success
                qDebug() << "Renamed mapping:" << oldPath << "=>" << newPath;

                return true;
            } else {
                // we couldn't persist the renamed mapping, rollback and return failure
                _fileMappings[oldPath] = oldSourceMapping;

                if (!oldDestinationMapping.isNull()) {
                    // put back the overwritten mapping for the destination path
                    _fileMappings[newPath] = oldDestinationMapping.toString();
                } else {
                    // clear the new mapping
                    _fileMappings.remove(newPath);
                }

                qDebug() << "Failed to persist renamed mapping:" << oldPath << "=>" << newPath;

                return false;
            }
        } else {
            // failed to find a mapping that was to be renamed, return failure
            return false;
        }
    }
}
