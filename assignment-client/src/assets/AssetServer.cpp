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

#include <thread>
#include <memory>

#include <QtCore/QCoreApplication>
#include <QtCore/QCryptographicHash>
#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QDirIterator>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonDocument>
#include <QtCore/QSaveFile>
#include <QtCore/QString>
#include <QtGui/QImageReader>
#include <QtCore/QVector>
#include <QtCore/QUrlQuery>

#include <ClientServerUtils.h>
#include <NodeType.h>
#include <SharedUtil.h>
#include <PathUtils.h>
#include <image/TextureProcessing.h>

#include "AssetServerLogging.h"
#include "BakeAssetTask.h"
#include "SendAssetTask.h"
#include "UploadAssetTask.h"

static const uint8_t MIN_CORES_FOR_MULTICORE = 4;
static const uint8_t CPU_AFFINITY_COUNT_HIGH = 2;
static const uint8_t CPU_AFFINITY_COUNT_LOW = 1;
#ifdef Q_OS_WIN
static const int INTERFACE_RUNNING_CHECK_FREQUENCY_MS = 1000;
#endif

static const QStringList BAKEABLE_MODEL_EXTENSIONS = { "fbx" };
static QStringList BAKEABLE_TEXTURE_EXTENSIONS;
static const QStringList BAKEABLE_SCRIPT_EXTENSIONS = { };

static const QString BAKED_MODEL_SIMPLE_NAME = "asset.fbx";
static const QString BAKED_TEXTURE_SIMPLE_NAME = "texture.ktx";
static const QString BAKED_SCRIPT_SIMPLE_NAME = "asset.js";

static const ModelBakeVersion CURRENT_MODEL_BAKE_VERSION = (ModelBakeVersion)((BakeVersion)ModelBakeVersion::COUNT - 1);
static const TextureBakeVersion CURRENT_TEXTURE_BAKE_VERSION = (TextureBakeVersion)((BakeVersion)TextureBakeVersion::COUNT - 1);
static const ScriptBakeVersion CURRENT_SCRIPT_BAKE_VERSION = (ScriptBakeVersion)((BakeVersion)ScriptBakeVersion::COUNT - 1);

BakedAssetType assetTypeForExtension(const QString& extension) {
    auto extensionLower = extension.toLower();
    if (BAKEABLE_MODEL_EXTENSIONS.contains(extensionLower)) {
        return BakedAssetType::Model;
    } else if (BAKEABLE_TEXTURE_EXTENSIONS.contains(extensionLower.toLocal8Bit())) {
        return BakedAssetType::Texture;
    } else if (BAKEABLE_SCRIPT_EXTENSIONS.contains(extensionLower)) {
        return BakedAssetType::Script;
    }
    return BakedAssetType::Undefined;
}

BakedAssetType assetTypeForFilename(const QString& filename) {
    auto dotIndex = filename.lastIndexOf(".");
    if (dotIndex == -1) {
        return BakedAssetType::Undefined;
    }

    auto extension = filename.mid(dotIndex + 1);
    return assetTypeForExtension(extension);
}

QString bakedFilenameForAssetType(BakedAssetType type) {
    switch (type) {
        case BakedAssetType::Model:
            return BAKED_MODEL_SIMPLE_NAME;
        case BakedAssetType::Texture:
            return BAKED_TEXTURE_SIMPLE_NAME;
        case BakedAssetType::Script:
            return BAKED_SCRIPT_SIMPLE_NAME;
        default:
            return "";
    }
}

BakeVersion currentBakeVersionForAssetType(BakedAssetType type) {
    switch (type) {
        case BakedAssetType::Model:
            return (BakeVersion)CURRENT_MODEL_BAKE_VERSION;
        case BakedAssetType::Texture:
            return (BakeVersion)CURRENT_TEXTURE_BAKE_VERSION;
        case BakedAssetType::Script:
            return (BakeVersion)CURRENT_SCRIPT_BAKE_VERSION;
        default:
            return 0;
    }
}

QString getBakeMapping(const AssetUtils::AssetHash& hash, const QString& relativeFilePath) {
    return AssetUtils::HIDDEN_BAKED_CONTENT_FOLDER + hash + "/" + relativeFilePath;
}

const QString ASSET_SERVER_LOGGING_TARGET_NAME = "asset-server";

void AssetServer::bakeAsset(const AssetUtils::AssetHash& assetHash, const AssetUtils::AssetPath& assetPath, const QString& filePath) {
    qDebug() << "Starting bake for: " << assetPath << assetHash;
    auto it = _pendingBakes.find(assetHash);
    if (it == _pendingBakes.end()) {
        auto task = std::make_shared<BakeAssetTask>(assetHash, assetPath, filePath);
        task->setAutoDelete(false);
        _pendingBakes[assetHash] = task;

        connect(task.get(), &BakeAssetTask::bakeComplete, this, &AssetServer::handleCompletedBake);
        connect(task.get(), &BakeAssetTask::bakeFailed, this, &AssetServer::handleFailedBake);
        connect(task.get(), &BakeAssetTask::bakeAborted, this, &AssetServer::handleAbortedBake);

        _bakingTaskPool.start(task.get());
    } else {
        qDebug() << "Already in queue";
    }
}

QString AssetServer::getPathToAssetHash(const AssetUtils::AssetHash& assetHash) {
    return _filesDirectory.absoluteFilePath(assetHash);
}

std::pair<AssetUtils::BakingStatus, QString> AssetServer::getAssetStatus(const AssetUtils::AssetPath& path, const AssetUtils::AssetHash& hash) {
    auto it = _pendingBakes.find(hash);
    if (it != _pendingBakes.end()) {
        return { (*it)->isBaking() ? AssetUtils::Baking : AssetUtils::Pending, "" };
    }

    if (path.startsWith(AssetUtils::HIDDEN_BAKED_CONTENT_FOLDER)) {
        return { AssetUtils::Baked, "" };
    }

    BakedAssetType type = assetTypeForFilename(path);
    if (type == BakedAssetType::Undefined) {
        return { AssetUtils::Irrelevant, "" };
    }

    bool loaded;
    AssetMeta meta;
    std::tie(loaded, meta) = readMetaFile(hash);

    // We create a meta file for Skyboxes at runtime when they get requested
    // Otherwise, textures don't get baked by themselves.
    if (type == BakedAssetType::Texture && !loaded) {
        return { AssetUtils::Irrelevant, "" };
    }

    QString bakedFilename = bakedFilenameForAssetType(type);
    auto bakedPath = getBakeMapping(hash, bakedFilename);
    if (loaded && !meta.redirectTarget.isEmpty()) {
        bakedPath = meta.redirectTarget;
    }

    auto jt = _fileMappings.find(bakedPath);
    if (jt != _fileMappings.end()) {
        if (jt->second == hash) {
            return { AssetUtils::NotBaked, "" };
        } else {
            return { AssetUtils::Baked, "" };
        }
    } else if (loaded && meta.failedLastBake) {
        return { AssetUtils::Error, meta.lastBakeErrors };
    }

    return { AssetUtils::Pending, "" };
}

void AssetServer::bakeAssets() {
    auto it = _fileMappings.cbegin();
    for (; it != _fileMappings.cend(); ++it) {
        auto path = it->first;
        auto hash = it->second;
        maybeBake(path, hash);
    }
}

void AssetServer::maybeBake(const AssetUtils::AssetPath& path, const AssetUtils::AssetHash& hash) {
    if (needsToBeBaked(path, hash)) {
        qDebug() << "Queuing bake of: " << path;
        bakeAsset(hash, path, getPathToAssetHash(hash));
    }
}

void AssetServer::createEmptyMetaFile(const AssetUtils::AssetHash& hash) {
    QString metaFilePath = "atp:/" + hash + "/meta.json";
    QFile metaFile { metaFilePath };

    if (!metaFile.exists()) {
        qDebug() << "Creating metafile for " << hash;
        if (metaFile.open(QFile::WriteOnly)) {
            qDebug() << "Created metafile for " << hash;
            metaFile.write("{}");
        }
    }
}

bool AssetServer::hasMetaFile(const AssetUtils::AssetHash& hash) {
    QString metaFilePath = AssetUtils::HIDDEN_BAKED_CONTENT_FOLDER + hash + "/meta.json";

    return _fileMappings.find(metaFilePath) != _fileMappings.end();
}

bool AssetServer::needsToBeBaked(const AssetUtils::AssetPath& path, const AssetUtils::AssetHash& assetHash) {
    if (path.startsWith(AssetUtils::HIDDEN_BAKED_CONTENT_FOLDER)) {
        return false;
    }

    BakedAssetType type = assetTypeForFilename(path);

    if (type == BakedAssetType::Undefined) {
        return false;
    }

    bool loaded;
    AssetMeta meta;
    std::tie(loaded, meta) = readMetaFile(assetHash);

    QString bakedFilename = bakedFilenameForAssetType(type);
    auto bakedPath = getBakeMapping(assetHash, bakedFilename);
    if (loaded && !meta.redirectTarget.isEmpty()) {
        bakedPath = meta.redirectTarget;
    }

    auto mappingIt = _fileMappings.find(bakedPath);
    bool bakedMappingExists = mappingIt != _fileMappings.end();

    // If the path is mapped to the original file's hash, baking has been disabled for this
    // asset
    if (bakedMappingExists && mappingIt->second == assetHash) {
        return false;
    }

    // We create a meta file for Skyboxes at runtime when they get requested
    // Otherwise, textures don't get baked by themselves.
    if (type == BakedAssetType::Texture && !loaded) {
        return false;
    }

    auto currentVersion = currentBakeVersionForAssetType(type);

    if (loaded && (meta.failedLastBake && meta.bakeVersion >= currentVersion)) {
        return false;
    }

    return !bakedMappingExists || (meta.bakeVersion < currentVersion);
}

bool interfaceRunning() {
    bool result = false;

#ifdef Q_OS_WIN
    QSharedMemory sharedMemory { getInterfaceSharedMemoryName() };
    result = sharedMemory.attach(QSharedMemory::ReadOnly);
    if (result) {
        sharedMemory.detach();
    }
#endif
    return result;
}

void updateConsumedCores() {
    static bool wasInterfaceRunning = false;
    bool isInterfaceRunning = interfaceRunning();
    // If state is unchanged, return early
    if (isInterfaceRunning == wasInterfaceRunning) {
        return;
    }

    wasInterfaceRunning = isInterfaceRunning;
    auto coreCount = std::thread::hardware_concurrency();
    if (isInterfaceRunning) {
        coreCount = coreCount > MIN_CORES_FOR_MULTICORE ? CPU_AFFINITY_COUNT_HIGH : CPU_AFFINITY_COUNT_LOW;
    }
    qCDebug(asset_server) << "Setting max consumed cores to " << coreCount;
    setMaxCores(coreCount);
}


AssetServer::AssetServer(ReceivedMessage& message) :
    ThreadedAssignment(message),
    _transferTaskPool(this),
    _bakingTaskPool(this),
    _filesizeLimit(AssetUtils::MAX_UPLOAD_SIZE)
{
    BAKEABLE_TEXTURE_EXTENSIONS = image::getSupportedFormats();
    qDebug() << "Supported baking texture formats:" << BAKEABLE_MODEL_EXTENSIONS;

    // Most of the work will be I/O bound, reading from disk and constructing packet objects,
    // so the ideal is greater than the number of cores on the system.
    static const int TASK_POOL_THREAD_COUNT = 50;
    _transferTaskPool.setMaxThreadCount(TASK_POOL_THREAD_COUNT);
    _bakingTaskPool.setMaxThreadCount(1);

    // Queue all requests until the Asset Server is fully setup
    auto& packetReceiver = DependencyManager::get<NodeList>()->getPacketReceiver();
    packetReceiver.registerListenerForTypes({ PacketType::AssetGet, PacketType::AssetGetInfo, PacketType::AssetUpload, PacketType::AssetMappingOperation },
        PacketReceiver::makeSourcedListenerReference<AssetServer>(this, &AssetServer::queueRequests));

#ifdef Q_OS_WIN
    updateConsumedCores();
    QTimer* timer = new QTimer(this);
    auto timerConnection = connect(timer, &QTimer::timeout, [] {
        updateConsumedCores();
    });
    connect(qApp, &QCoreApplication::aboutToQuit, [this, timerConnection] {
        disconnect(timerConnection);
    });
    timer->setInterval(INTERFACE_RUNNING_CHECK_FREQUENCY_MS);
    timer->setTimerType(Qt::CoarseTimer);
    timer->start();
#endif
}

void AssetServer::aboutToFinish() {

    // remove pending transfer tasks
    _transferTaskPool.clear();

    // abort each of our still running bake tasks, remove pending bakes that were never put on the thread pool
    auto it = _pendingBakes.begin();
    while (it != _pendingBakes.end()) {
        auto pendingRunnable =  _bakingTaskPool.tryTake(it->get());

        if (pendingRunnable) {
            it = _pendingBakes.erase(it);
        } else {
            qDebug() << "Aborting bake for" << it.key();
            it.value()->abort();
            ++it;
        }
    }

    // make sure all bakers are finished or aborted
    while (_pendingBakes.size() > 0) {
        QCoreApplication::processEvents();
    }
}

void AssetServer::run() {

    qCDebug(asset_server) << "Waiting for connection to domain to request settings from domain-server.";

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
        qCCritical(asset_server) << "Received settings from the domain-server with no asset-server section. Stopping assignment.";
        setFinished(true);
        return;
    }

    auto assetServerObject = settingsObject[ASSET_SERVER_SETTINGS_KEY].toObject();

    static const QString MAX_BANDWIDTH_OPTION = "max_bandwidth";
    auto maxBandwidthValue = assetServerObject[MAX_BANDWIDTH_OPTION];
    auto maxBandwidthFloat = maxBandwidthValue.toDouble(-1);

    const int BITS_PER_MEGABITS = 1000 * 1000;
    if (maxBandwidthFloat > 0.0) {
        int maxBandwidth = maxBandwidthFloat * BITS_PER_MEGABITS;
        nodeList->setConnectionMaxBandwidth(maxBandwidth);
        qCInfo(asset_server) << "Set maximum bandwith per connection to" << maxBandwidthFloat << "Mb/s."
                    " (" << maxBandwidth << "bits/s)";
    }

    // get the path to the asset folder from the domain server settings
    static const QString ASSETS_PATH_OPTION = "assets_path";
    auto assetsJSONValue = assetServerObject[ASSETS_PATH_OPTION];

    if (!assetsJSONValue.isString()) {
        qCCritical(asset_server) << "Received an assets path from the domain-server that could not be parsed. Stopping assignment.";
        setFinished(true);
        return;
    }

    auto assetsPathString = assetsJSONValue.toString();
    QDir assetsPath { assetsPathString };
    QString absoluteFilePath = assetsPath.absolutePath();

    if (assetsPath.isRelative()) {
        // if the domain settings passed us a relative path, make an absolute path that is relative to the
        // default data directory
        absoluteFilePath = PathUtils::getAppDataFilePath("assets/" + assetsPathString);
    }

    _resourcesDirectory = QDir(absoluteFilePath);

    qCDebug(asset_server) << "Creating resources directory";
    _resourcesDirectory.mkpath(".");
    _filesDirectory = _resourcesDirectory;

    if (!_resourcesDirectory.mkpath(ASSET_FILES_SUBDIR) || !_filesDirectory.cd(ASSET_FILES_SUBDIR)) {
        qCCritical(asset_server) << "Unable to create file directory for asset-server files. Stopping assignment.";
        setFinished(true);
        return;
    }

    // load whatever mappings we currently have from the local file
    if (loadMappingsFromFile()) {
        qCInfo(asset_server) << "Serving files from: " << _filesDirectory.path();

        // Check the asset directory to output some information about what we have
        auto files = _filesDirectory.entryList(QDir::Files);

        QRegExp hashFileRegex { AssetUtils::ASSET_HASH_REGEX_STRING };
        auto hashedFiles = files.filter(hashFileRegex);

        qCInfo(asset_server) << "There are" << hashedFiles.size() << "asset files in the asset directory.";

        if (_fileMappings.size() > 0) {
            cleanupUnmappedFiles();
            cleanupBakedFilesForDeletedAssets();
        }

        nodeList->addSetOfNodeTypesToNodeInterestSet({ NodeType::Agent, NodeType::EntityScriptServer });

        bakeAssets();
    } else {
        qCCritical(asset_server) << "Asset Server assignment will not continue because mapping file could not be loaded.";
        setFinished(true);
    }

    // get file size limit for an asset
    static const QString ASSETS_FILESIZE_LIMIT_OPTION = "assets_filesize_limit";
    auto assetsFilesizeLimitJSONValue = assetServerObject[ASSETS_FILESIZE_LIMIT_OPTION];
    auto assetsFilesizeLimit = (uint64_t)assetsFilesizeLimitJSONValue.toInt(AssetUtils::MAX_UPLOAD_SIZE);

    if (assetsFilesizeLimit != 0 && assetsFilesizeLimit < AssetUtils::MAX_UPLOAD_SIZE) {
        _filesizeLimit = assetsFilesizeLimit * BITS_PER_MEGABITS;
    }

    PathUtils::removeTemporaryApplicationDirs();
    PathUtils::removeTemporaryApplicationDirs("Oven");

    qCDebug(asset_server) << "Overriding temporary queuing packet handler.";
    // We're fully setup, override the request queueing handler and replay all requests
    auto& packetReceiver = DependencyManager::get<NodeList>()->getPacketReceiver();
    packetReceiver.registerListener(PacketType::AssetGet,
        PacketReceiver::makeSourcedListenerReference<AssetServer>(this, &AssetServer::handleAssetGet));
    packetReceiver.registerListener(PacketType::AssetGetInfo,
        PacketReceiver::makeSourcedListenerReference<AssetServer>(this, &AssetServer::handleAssetGetInfo));
    packetReceiver.registerListener(PacketType::AssetUpload,
        PacketReceiver::makeSourcedListenerReference<AssetServer>(this, &AssetServer::handleAssetUpload));
    packetReceiver.registerListener(PacketType::AssetMappingOperation,
        PacketReceiver::makeSourcedListenerReference<AssetServer>(this, &AssetServer::handleAssetMappingOperation));

    replayRequests();
}

void AssetServer::queueRequests(QSharedPointer<ReceivedMessage> packet, SharedNodePointer senderNode) {
    qCDebug(asset_server) << "Queuing requests until fully setup";

    QMutexLocker lock { &_queuedRequestsMutex };
    _queuedRequests.push_back({ packet, senderNode });

    // If we've stopped queueing but the callback was already in flight,
    // then replay it immediately.
    if (!_isQueueingRequests) {
        lock.unlock();
        replayRequests();
    }
}

void AssetServer::replayRequests() {
    RequestQueue queue;
    {
        QMutexLocker lock { &_queuedRequestsMutex };
        std::swap(queue, _queuedRequests);
        _isQueueingRequests = false;
    }

    qCDebug(asset_server) << "Replaying" << queue.size() << "requests.";

    for (const auto& request : queue) {
        switch (request.first->getType()) {
            case PacketType::AssetGet:
                handleAssetGet(request.first, request.second);
                break;
            case PacketType::AssetGetInfo:
                handleAssetGetInfo(request.first, request.second);
                break;
            case PacketType::AssetUpload:
                handleAssetUpload(request.first, request.second);
                break;
            case PacketType::AssetMappingOperation:
                handleAssetMappingOperation(request.first, request.second);
                break;
            default:
                qCWarning(asset_server) << "Unknown queued request type:" << request.first->getType();
                break;
        }
    }
}

void AssetServer::cleanupUnmappedFiles() {
    QRegExp hashFileRegex { AssetUtils::ASSET_HASH_REGEX_STRING };

    auto files = _filesDirectory.entryInfoList(QDir::Files);

    qCInfo(asset_server) << "Performing unmapped asset cleanup.";

    for (const auto& fileInfo : files) {
        auto filename = fileInfo.fileName();
        if (hashFileRegex.exactMatch(filename)) {
            bool matched { false };
            for (auto& pair : _fileMappings) {
                if (pair.second == filename) {
                    matched = true;
                    break;
                }
            }
            if (!matched) {
                // remove the unmapped file
                QFile removeableFile { fileInfo.absoluteFilePath() };

                if (removeableFile.remove()) {
                    qCDebug(asset_server) << "\tDeleted" << filename << "from asset files directory since it is unmapped.";

                    removeBakedPathsForDeletedAsset(filename);
                } else {
                    qCDebug(asset_server) << "\tAttempt to delete unmapped file" << filename << "failed";
                }
            }
        }
    }
}

void AssetServer::cleanupBakedFilesForDeletedAssets() {
    qCInfo(asset_server) << "Performing baked asset cleanup for deleted assets";

    std::set<AssetUtils::AssetHash> bakedHashes;

    for (const auto& it : _fileMappings) {
        // check if this is a mapping to baked content
        if (it.first.startsWith(AssetUtils::HIDDEN_BAKED_CONTENT_FOLDER)) {
            // extract the hash from the baked mapping
            AssetUtils::AssetHash hash = it.first.mid(AssetUtils::HIDDEN_BAKED_CONTENT_FOLDER.length(),
                                                      AssetUtils::SHA256_HASH_HEX_LENGTH);

            // add the hash to our set of hashes for which we have baked content
            bakedHashes.insert(hash);
        }
    }

    // enumerate the hashes for which we have baked content
    for (const auto& hash : bakedHashes) {
        // check if we have a mapping that points to this hash
        auto matchingMapping = std::find_if(std::begin(_fileMappings), std::end(_fileMappings),
                                            [&hash](const std::pair<AssetUtils::AssetPath, AssetUtils::AssetHash> mappingPair) {
            return mappingPair.second == hash;
        });

        if (matchingMapping == std::end(_fileMappings)) {
            // we didn't find a mapping for this hash, remove any baked content we still have for it
            removeBakedPathsForDeletedAsset(hash);
        }
    }
}

void AssetServer::handleAssetMappingOperation(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode) {
    using AssetMappingOperationType = AssetUtils::AssetMappingOperationType;

    MessageID messageID;
    message->readPrimitive(&messageID);

    AssetMappingOperationType operationType;
    message->readPrimitive(&operationType);

    auto replyPacket = NLPacketList::create(PacketType::AssetMappingOperationReply, QByteArray(), true, true);
    replyPacket->writePrimitive(messageID);

    bool canWriteToAssetServer = true;
    if (senderNode) {
        canWriteToAssetServer = senderNode->getCanWriteToAssetServer();
    }

    switch (operationType) {
        case AssetMappingOperationType::Get:
            handleGetMappingOperation(*message, *replyPacket);
            break;
        case AssetMappingOperationType::GetAll:
            handleGetAllMappingOperation(*replyPacket);
            break;
        case AssetMappingOperationType::Set:
            handleSetMappingOperation(*message, canWriteToAssetServer, *replyPacket);
            break;
        case AssetMappingOperationType::Delete:
            handleDeleteMappingsOperation(*message, canWriteToAssetServer, *replyPacket);
            break;
        case AssetMappingOperationType::Rename:
            handleRenameMappingOperation(*message, canWriteToAssetServer, *replyPacket);
            break;
        case AssetMappingOperationType::SetBakingEnabled:
            handleSetBakingEnabledOperation(*message, canWriteToAssetServer, *replyPacket);
            break;
    }

    auto nodeList = DependencyManager::get<NodeList>();
    if (senderNode) {
        nodeList->sendPacketList(std::move(replyPacket), *senderNode);
    } else {
        nodeList->sendPacketList(std::move(replyPacket), message->getSenderSockAddr());
    }
}

void AssetServer::handleGetMappingOperation(ReceivedMessage& message, NLPacketList& replyPacket) {
    QString assetPath = message.readString();

    QUrl url { assetPath };
    assetPath = url.path();

    auto it = _fileMappings.find(assetPath);
    if (it != _fileMappings.end()) {

        // check if we should re-direct to a baked asset
        auto originalAssetHash = it->second;
        QString redirectedAssetHash;
        quint8 wasRedirected = false;
        bool bakingDisabled = false;

        bool loaded;
        AssetMeta meta;
        std::tie(loaded, meta) = readMetaFile(originalAssetHash);

        auto type = assetTypeForFilename(assetPath);
        QString bakedRootFile = bakedFilenameForAssetType(type);
        QString bakedAssetPath = getBakeMapping(originalAssetHash, bakedRootFile);

        if (loaded && !meta.redirectTarget.isEmpty()) {
            bakedAssetPath = meta.redirectTarget;
        }

        auto bakedIt = _fileMappings.find(bakedAssetPath);
        if (bakedIt != _fileMappings.end()) {
            if (bakedIt->second != originalAssetHash) {
                qDebug() << "Did find baked version for: " << originalAssetHash << assetPath;
                // we found a baked version of the requested asset to serve, redirect to that
                redirectedAssetHash = bakedIt->second;
                wasRedirected = true;
            } else {
                qDebug() << "Did not find baked version for: " << originalAssetHash << assetPath << " (disabled)";
                bakingDisabled = true;
            }
        }

        replyPacket.writePrimitive(AssetUtils::AssetServerError::NoError);

        if (wasRedirected) {
            qDebug() << "Writing re-directed hash for" << originalAssetHash << "to" << redirectedAssetHash;
            replyPacket.write(QByteArray::fromHex(redirectedAssetHash.toUtf8()));

            // add a flag saying that this mapping request was redirect
            replyPacket.writePrimitive(wasRedirected);

            // include the re-directed path in case the caller needs to make relative path requests for the baked asset
            replyPacket.writeString(bakedAssetPath);

        } else {
            replyPacket.write(QByteArray::fromHex(originalAssetHash.toUtf8()));
            replyPacket.writePrimitive(wasRedirected);

            auto query = QUrlQuery(url.query());
            bool isSkybox = query.hasQueryItem("skybox");
            if (isSkybox && !loaded) {
                AssetMeta needsBakingMeta;
                needsBakingMeta.bakeVersion = NEEDS_BAKING_BAKE_VERSION;

                writeMetaFile(originalAssetHash, needsBakingMeta);
                if (!bakingDisabled) {
                    maybeBake(assetPath, originalAssetHash);
                }
            }
        }
    } else {
        replyPacket.writePrimitive(AssetUtils::AssetServerError::AssetNotFound);
    }
}

void AssetServer::handleGetAllMappingOperation(NLPacketList& replyPacket) {
    replyPacket.writePrimitive(AssetUtils::AssetServerError::NoError);

    uint32_t count = (uint32_t)_fileMappings.size();

    replyPacket.writePrimitive(count);

    for (auto it = _fileMappings.cbegin(); it != _fileMappings.cend(); ++ it) {
        auto mapping = it->first;
        auto hash = it->second;
        replyPacket.writeString(mapping);
        replyPacket.write(QByteArray::fromHex(hash.toUtf8()));

        AssetUtils::BakingStatus status;
        QString lastBakeErrors;
        std::tie(status, lastBakeErrors) = getAssetStatus(mapping, hash);
        replyPacket.writePrimitive(status);
        if (status == AssetUtils::Error) {
            replyPacket.writeString(lastBakeErrors);
        }
    }
}

void AssetServer::handleSetMappingOperation(ReceivedMessage& message, bool hasWriteAccess, NLPacketList& replyPacket) {
    if (hasWriteAccess) {
        QString assetPath = message.readString();

        auto assetHash = message.read(AssetUtils::SHA256_HASH_LENGTH).toHex();

        // don't process a set mapping operation that is inside the hidden baked folder
        if (assetPath.startsWith(AssetUtils::HIDDEN_BAKED_CONTENT_FOLDER)) {
            qCDebug(asset_server) << "Refusing to process a set mapping operation inside" << AssetUtils::HIDDEN_BAKED_CONTENT_FOLDER;
            replyPacket.writePrimitive(AssetUtils::AssetServerError::PermissionDenied);
        } else {
            if (setMapping(assetPath, assetHash)) {
                replyPacket.writePrimitive(AssetUtils::AssetServerError::NoError);
            } else {
                replyPacket.writePrimitive(AssetUtils::AssetServerError::MappingOperationFailed);
            }
        }

    } else {
        replyPacket.writePrimitive(AssetUtils::AssetServerError::PermissionDenied);
    }
}

void AssetServer::handleDeleteMappingsOperation(ReceivedMessage& message, bool hasWriteAccess, NLPacketList& replyPacket) {
    if (hasWriteAccess) {
        int numberOfDeletedMappings { 0 };
        message.readPrimitive(&numberOfDeletedMappings);

        QStringList mappingsToDelete;

        for (int i = 0; i < numberOfDeletedMappings; ++i) {
            auto mapping = message.readString();

            if (!mapping.startsWith(AssetUtils::HIDDEN_BAKED_CONTENT_FOLDER)) {
                mappingsToDelete << mapping;
            } else {
                qCDebug(asset_server) << "Refusing to delete mapping" << mapping
                    << "that is inside" << AssetUtils::HIDDEN_BAKED_CONTENT_FOLDER;
            }
        }

        if (deleteMappings(mappingsToDelete)) {
            replyPacket.writePrimitive(AssetUtils::AssetServerError::NoError);
        } else {
            replyPacket.writePrimitive(AssetUtils::AssetServerError::MappingOperationFailed);
        }
    } else {
        replyPacket.writePrimitive(AssetUtils::AssetServerError::PermissionDenied);
    }
}

void AssetServer::handleRenameMappingOperation(ReceivedMessage& message, bool hasWriteAccess, NLPacketList& replyPacket) {
    if (hasWriteAccess) {
        QString oldPath = message.readString();
        QString newPath = message.readString();

        if (oldPath.startsWith(AssetUtils::HIDDEN_BAKED_CONTENT_FOLDER) || newPath.startsWith(AssetUtils::HIDDEN_BAKED_CONTENT_FOLDER)) {
            qCDebug(asset_server) << "Cannot rename" << oldPath << "to" << newPath
                << "since one of the paths is inside" << AssetUtils::HIDDEN_BAKED_CONTENT_FOLDER;
            replyPacket.writePrimitive(AssetUtils::AssetServerError::PermissionDenied);
        } else {
            if (renameMapping(oldPath, newPath)) {
                replyPacket.writePrimitive(AssetUtils::AssetServerError::NoError);
            } else {
                replyPacket.writePrimitive(AssetUtils::AssetServerError::MappingOperationFailed);
            }
        }

    } else {
        replyPacket.writePrimitive(AssetUtils::AssetServerError::PermissionDenied);
    }
}

void AssetServer::handleSetBakingEnabledOperation(ReceivedMessage& message, bool hasWriteAccess, NLPacketList& replyPacket) {
    if (hasWriteAccess) {
        bool enabled { true };
        message.readPrimitive(&enabled);

        int numberOfMappings{ 0 };
        message.readPrimitive(&numberOfMappings);

        QStringList mappings;

        for (int i = 0; i < numberOfMappings; ++i) {
            mappings << message.readString();
        }

        if (setBakingEnabled(mappings, enabled)) {
            replyPacket.writePrimitive(AssetUtils::AssetServerError::NoError);
        } else {
            replyPacket.writePrimitive(AssetUtils::AssetServerError::MappingOperationFailed);
        }
    } else {
        replyPacket.writePrimitive(AssetUtils::AssetServerError::PermissionDenied);
    }
}

void AssetServer::handleAssetGetInfo(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode) {
    QByteArray assetHash;
    MessageID messageID;

    if (message->getSize() < qint64(AssetUtils::SHA256_HASH_LENGTH + sizeof(messageID))) {
        qCDebug(asset_server) << "ERROR bad file request";
        return;
    }

    message->readPrimitive(&messageID);
    assetHash = message->readWithoutCopy(AssetUtils::SHA256_HASH_LENGTH);

    auto size = qint64(sizeof(MessageID) + AssetUtils::SHA256_HASH_LENGTH + sizeof(AssetUtils::AssetServerError) + sizeof(qint64));
    auto replyPacket = NLPacket::create(PacketType::AssetGetInfoReply, size, true);

    QByteArray hexHash = assetHash.toHex();

    replyPacket->writePrimitive(messageID);
    replyPacket->write(assetHash);

    QString fileName = QString(hexHash);
    QFileInfo fileInfo { _filesDirectory.filePath(fileName) };

    if (fileInfo.exists() && fileInfo.isReadable()) {
        qCDebug(asset_server) << "Opening file: " << fileInfo.filePath();
        replyPacket->writePrimitive(AssetUtils::AssetServerError::NoError);
        replyPacket->writePrimitive(fileInfo.size());
    } else {
        qCDebug(asset_server) << "Asset not found: " << QString(hexHash);
        replyPacket->writePrimitive(AssetUtils::AssetServerError::AssetNotFound);
    }

    auto nodeList = DependencyManager::get<NodeList>();
    nodeList->sendPacket(std::move(replyPacket), *senderNode);
}

void AssetServer::handleAssetGet(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode) {

    auto minSize = qint64(sizeof(MessageID) + AssetUtils::SHA256_HASH_LENGTH + sizeof(AssetUtils::DataOffset) + sizeof(AssetUtils::DataOffset));

    if (message->getSize() < minSize) {
        qCDebug(asset_server) << "ERROR bad file request";
        return;
    }

    // Queue task
    auto task = new SendAssetTask(message, senderNode, _filesDirectory);
    _transferTaskPool.start(task);
}

void AssetServer::handleAssetUpload(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode) {
    bool canWriteToAssetServer = true;
    if (senderNode) {
        canWriteToAssetServer = senderNode->getCanWriteToAssetServer();
    }


    if (canWriteToAssetServer) {
        qCDebug(asset_server) << "Starting an UploadAssetTask for upload from" << message->getSourceID();

        auto task = new UploadAssetTask(message, senderNode, _filesDirectory, _filesizeLimit);
        _transferTaskPool.start(task);
    } else {
        // this is a node the domain told us is not allowed to rez entities
        // for now this also means it isn't allowed to add assets
        // so return a packet with error that indicates that

        auto permissionErrorPacket = NLPacket::create(PacketType::AssetUploadReply, sizeof(MessageID) + sizeof(AssetUtils::AssetServerError), true);

        MessageID messageID;
        message->readPrimitive(&messageID);

        // write the message ID and a permission denied error
        permissionErrorPacket->writePrimitive(messageID);
        permissionErrorPacket->writePrimitive(AssetUtils::AssetServerError::PermissionDenied);

        // send off the packet
        auto nodeList = DependencyManager::get<NodeList>();
        if (senderNode) {
            nodeList->sendPacket(std::move(permissionErrorPacket), *senderNode);
        } else {
            nodeList->sendPacket(std::move(permissionErrorPacket), message->getSenderSockAddr());
        }
    }
}

void AssetServer::sendStatsPacket() {
    QJsonObject serverStats;

    auto nodeList = DependencyManager::get<NodeList>();
    nodeList->eachNode([&](auto& node) {
        auto& stats = node->getConnectionStats();

        QJsonObject nodeStats;
        auto endTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(stats.endTime);
        QDateTime date = QDateTime::fromMSecsSinceEpoch(endTimeMs.count());

        static const float USEC_PER_SEC = 1000000.0f;
        static const float MEGABITS_PER_BYTE = 8.0f / 1000000.0f; // Bytes => Mbits
        float elapsed = (float)(stats.endTime - stats.startTime).count() / USEC_PER_SEC; // sec
        float megabitsPerSecPerByte = MEGABITS_PER_BYTE / elapsed; // Bytes => Mb/s

        QJsonObject connectionStats;
        connectionStats["1. Last Heard"] = date.toString();
        connectionStats["2. Est. Max (P/s)"] = stats.estimatedBandwith;
        connectionStats["3. RTT (ms)"] = stats.rtt;
        connectionStats["4. CW (P)"] = stats.congestionWindowSize;
        connectionStats["5. Period (us)"] = stats.packetSendPeriod;
        connectionStats["6. Up (Mb/s)"] = stats.sentBytes * megabitsPerSecPerByte;
        connectionStats["7. Down (Mb/s)"] = stats.receivedBytes * megabitsPerSecPerByte;
        connectionStats["last_heard_time_msecs"] = date.toUTC().toMSecsSinceEpoch();
        connectionStats["last_heard_ago_msecs"] = date.msecsTo(QDateTime::currentDateTime());

        nodeStats["Connection Stats"] = connectionStats;

        using Events = udt::ConnectionStats::Stats::Event;
        const auto& events = stats.events;

        QJsonObject upstreamStats;
        upstreamStats["1. Sent (P/s)"] = stats.sendRate;
        upstreamStats["2. Sent Packets"] = (int)stats.sentPackets;
        upstreamStats["3. Recvd ACK"] = events[Events::ReceivedACK];
        upstreamStats["4. Procd ACK"] = events[Events::ProcessedACK];
        upstreamStats["5. Retransmitted"] = (int)stats.retransmittedPackets;
        nodeStats["Upstream Stats"] = upstreamStats;

        QJsonObject downstreamStats;
        downstreamStats["1. Recvd (P/s)"] = stats.receiveRate;
        downstreamStats["2. Recvd Packets"] = (int)stats.receivedPackets;
        downstreamStats["3. Sent ACK"] = events[Events::SentACK];
        downstreamStats["4. Duplicates"] = (int)stats.duplicatePackets;
        nodeStats["Downstream Stats"] = downstreamStats;

        QString uuid = uuidStringWithoutCurlyBraces(node->getUUID());
        nodeStats[USERNAME_UUID_REPLACEMENT_STATS_KEY] = uuid;

        serverStats[uuid] = nodeStats;
    });

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
                if (!jsonDocument.isObject()) {
                    qCWarning(asset_server) << "Failed to read mapping file, root value in" << mapFilePath << "is not an object";
                    return false;
                }

                //_fileMappings = jsonDocument.object().toVariantHash();
                auto root = jsonDocument.object();
                for (auto it = root.begin(); it != root.end(); ++it) {
                    auto key = it.key();
                    auto value = it.value();

                    if (!value.isString()) {
                        qCWarning(asset_server) << "Skipping" << key << ":" << value << "because it is not a string";
                        continue;
                    }

                    if (!AssetUtils::isValidFilePath(key)) {
                        qCWarning(asset_server) << "Will not keep mapping for" << key << "since it is not a valid path.";
                        continue;
                    }

                    if (!AssetUtils::isValidHash(value.toString())) {
                        qCWarning(asset_server) << "Will not keep mapping for" << key << "since it does not have a valid hash.";
                        continue;
                    }


                    qDebug() << "Added " << key << value.toString();
                    _fileMappings[key] = value.toString();
                }

                qCInfo(asset_server) << "Loaded" << _fileMappings.size() << "mappings from map file at" << mapFilePath;
                return true;
            }
        }

        qCCritical(asset_server) << "Failed to read mapping file at" << mapFilePath;
        return false;
    } else {
        qCInfo(asset_server) << "No existing mappings loaded from file since no file was found at" << mapFilePath;
    }

    return true;
}

bool AssetServer::writeMappingsToFile() {
    auto mapFilePath = _resourcesDirectory.absoluteFilePath(MAP_FILE_NAME);

    QSaveFile mapFile { mapFilePath };
    if (mapFile.open(QIODevice::WriteOnly)) {
        QJsonObject root;

        for (auto it : _fileMappings) {
            root[it.first] = it.second;
        }

        QJsonDocument jsonDocument { root };

        if (mapFile.write(jsonDocument.toJson()) != -1) {
            if (mapFile.commit()) {
                qCDebug(asset_server) << "Wrote JSON mappings to file at" << mapFilePath;
                return true;
            } else {
                qCWarning(asset_server) << "Failed to commit JSON mappings to file at" << mapFilePath;
            }
        } else {
            qCWarning(asset_server) << "Failed to write JSON mappings to file at" << mapFilePath;
        }
    } else {
        qCWarning(asset_server) << "Failed to open map file at" << mapFilePath;
    }

    return false;
}

bool AssetServer::setMapping(AssetUtils::AssetPath path, AssetUtils::AssetHash hash) {
    path = path.trimmed();

    if (!AssetUtils::isValidFilePath(path)) {
        qCWarning(asset_server) << "Cannot set a mapping for invalid path:" << path << "=>" << hash;
        return false;
    }

    if (!AssetUtils::isValidHash(hash)) {
        qCWarning(asset_server) << "Cannot set a mapping for invalid hash" << path << "=>" << hash;
        return false;
    }

    // remember what the old mapping was in case persistence fails
    auto it = _fileMappings.find(path);
    auto oldMapping = it != _fileMappings.end() ? it->second : "";

    // update the in memory QHash
    _fileMappings[path] = hash;

    // attempt to write to file
    if (writeMappingsToFile()) {
        // persistence succeeded, we are good to go
        qCDebug(asset_server) << "Set mapping:" << path << "=>" << hash;
        maybeBake(path, hash);
        return true;
    } else {
        // failed to persist this mapping to file - put back the old one in our in-memory representation
        if (oldMapping.isEmpty()) {
            _fileMappings.erase(_fileMappings.find(path));
        } else {
            _fileMappings[path] = oldMapping;
        }

        qCWarning(asset_server) << "Failed to persist mapping:" << path << "=>" << hash;

        return false;
    }
}

bool pathIsFolder(const AssetUtils::AssetPath& path) {
    return path.endsWith('/');
}

void AssetServer::removeBakedPathsForDeletedAsset(AssetUtils::AssetHash hash) {
    // we deleted the file with this hash

    // check if we had baked content for that file that should also now be removed
    // by calling deleteMappings for the hidden baked content folder for this hash
    AssetUtils::AssetPathList hiddenBakedFolder { AssetUtils::HIDDEN_BAKED_CONTENT_FOLDER + hash + "/" };

    qCDebug(asset_server) << "Deleting baked content below" << hiddenBakedFolder << "since" << hash << "was deleted";

    deleteMappings(hiddenBakedFolder);
}

bool AssetServer::deleteMappings(const AssetUtils::AssetPathList& paths) {
    // take a copy of the current mappings in case persistence of these deletes fails
    auto oldMappings = _fileMappings;

    QSet<QString> hashesToCheckForDeletion;

    // enumerate the paths to delete and remove them all
    for (const auto& rawPath : paths) {
        auto path = rawPath.trimmed();

        // figure out if this path will delete a file or folder
        if (pathIsFolder(path)) {
            // enumerate the in memory file mappings and remove anything that matches
            auto it = _fileMappings.begin();
            auto sizeBefore = _fileMappings.size();

            while (it != _fileMappings.end()) {
                if (it->first.startsWith(path)) {
                    // add this hash to the list we need to check for asset removal from the server
                    hashesToCheckForDeletion << it->second;

                    it = _fileMappings.erase(it);
                } else {
                    ++it;
                }
            }

            auto sizeNow = _fileMappings.size();
            if (sizeBefore != sizeNow) {
                qCDebug(asset_server) << "Deleted" << sizeBefore - sizeNow << "mappings in folder: " << path;
            } else {
                qCDebug(asset_server) << "Did not find any mappings to delete in folder:" << path;
            }

        } else {
            auto it = _fileMappings.find(path);
            if (it != _fileMappings.end()) {
                // add this hash to the list we need to check for asset removal from server
                hashesToCheckForDeletion << it->second;

                qCDebug(asset_server) << "Deleted a mapping:" << path << "=>" << it->second;

                _fileMappings.erase(it);
            } else {
                qCDebug(asset_server) << "Unable to delete a mapping that was not found:" << path;
            }
        }
    }

    // deleted the old mappings, attempt to persist to file
    if (writeMappingsToFile()) {
        // persistence succeeded we are good to go

        // TODO iterate through hashesToCheckForDeletion instead
        for (auto& pair : _fileMappings) {
            auto it = hashesToCheckForDeletion.find(pair.second);
            if (it != hashesToCheckForDeletion.end()) {
                hashesToCheckForDeletion.erase(it);
            }
        }

        // we now have a set of hashes that are unmapped - we will delete those asset files
        for (auto& hash : hashesToCheckForDeletion) {
            // remove the unmapped file
            QFile removeableFile { _filesDirectory.absoluteFilePath(hash) };

            if (removeableFile.remove()) {
                qCDebug(asset_server) << "\tDeleted" << hash << "from asset files directory since it is now unmapped.";

                removeBakedPathsForDeletedAsset(hash);
            } else {
                qCDebug(asset_server) << "\tAttempt to delete unmapped file" << hash << "failed";
            }
        }

        return true;
    } else {
        qCWarning(asset_server) << "Failed to persist deleted mappings, rolling back";

        // we didn't delete the previous mapping, put it back in our in-memory representation
        _fileMappings = oldMappings;

        return false;
    }
}

bool AssetServer::renameMapping(AssetUtils::AssetPath oldPath, AssetUtils::AssetPath newPath) {
    oldPath = oldPath.trimmed();
    newPath = newPath.trimmed();

    if (!AssetUtils::isValidFilePath(oldPath) || !AssetUtils::isValidFilePath(newPath)) {
        qCWarning(asset_server) << "Cannot perform rename with invalid paths - both should have leading forward and no ending slashes:"
            << oldPath << "=>" << newPath;

        return false;
    }

    // figure out if this rename is for a file or folder
    if (pathIsFolder(oldPath)) {
        if (!pathIsFolder(newPath)) {
            // we were asked to rename a path to a folder to a path that isn't a folder, this is a fail
            qCWarning(asset_server) << "Cannot rename mapping from folder path" << oldPath << "to file path" << newPath;

            return false;
        }

        // take a copy of the old mappings
        auto oldMappings = _fileMappings;

        // iterate the current mappings and adjust any that matches the renamed folder
        auto it = oldMappings.begin();

        while (it != oldMappings.end()) {
            auto& oldKey = it->first;
            if (oldKey.startsWith(oldPath)) {
                auto newKey = oldKey;
                newKey.replace(0, oldPath.size(), newPath);

                // remove the old version from the in memory file mappings
                _fileMappings.erase(_fileMappings.find(oldKey));
                _fileMappings[newKey] = it->second;
            }

            ++it;
        }

        if (writeMappingsToFile()) {
            // persisted the changed mappings, return success
            qCDebug(asset_server) << "Renamed folder mapping:" << oldPath << "=>" << newPath;

            return true;
        } else {
            // couldn't persist the renamed paths, rollback and return failure
            _fileMappings = oldMappings;

            qCWarning(asset_server) << "Failed to persist renamed folder mapping:" << oldPath << "=>" << newPath;

            return false;
        }
    } else {
        if (pathIsFolder(newPath)) {
            // we were asked to rename a path to a file to a path that is a folder, this is a fail
            qCWarning(asset_server) << "Cannot rename mapping from file path" << oldPath << "to folder path" << newPath;

            return false;
        }

        // take the old hash to remove the old mapping
        auto it = _fileMappings.find(oldPath);
        auto oldSourceMapping = it->second;
        _fileMappings.erase(it);

        // in case we're overwriting, keep the current destination mapping for potential rollback
        auto oldDestinationIt = _fileMappings.find(newPath);

        if (!oldSourceMapping.isEmpty()) {
            _fileMappings[newPath] = oldSourceMapping;

            if (writeMappingsToFile()) {
                // persisted the renamed mapping, return success
                qCDebug(asset_server) << "Renamed mapping:" << oldPath << "=>" << newPath;

                return true;
            } else {
                // we couldn't persist the renamed mapping, rollback and return failure
                _fileMappings[oldPath] = oldSourceMapping;

                if (oldDestinationIt != _fileMappings.end()) {
                    // put back the overwritten mapping for the destination path
                    _fileMappings[newPath] = oldDestinationIt->second;
                } else {
                    // clear the new mapping
                    _fileMappings.erase(_fileMappings.find(newPath));
                }

                qCDebug(asset_server) << "Failed to persist renamed mapping:" << oldPath << "=>" << newPath;

                return false;
            }
        } else {
            // failed to find a mapping that was to be renamed, return failure
            return false;
        }
    }
}

void AssetServer::handleFailedBake(QString originalAssetHash, QString assetPath, QString errors) {
    qDebug() << "Failed to bake: " << originalAssetHash << assetPath << "(" << errors << ")";

    bool loaded;
    AssetMeta meta;

    std::tie(loaded, meta) = readMetaFile(originalAssetHash);

    auto type = assetTypeForFilename(assetPath);
    auto currentTypeVersion = currentBakeVersionForAssetType(type);

    meta.failedLastBake = true;
    meta.lastBakeErrors = errors;
    meta.bakeVersion = currentTypeVersion;

    writeMetaFile(originalAssetHash, meta);

    _pendingBakes.remove(originalAssetHash);
}

void AssetServer::handleCompletedBake(QString originalAssetHash, QString originalAssetPath,
                                      QString bakedTempOutputDir) {
    auto reportCompletion = [this, originalAssetPath, originalAssetHash](bool errorCompletingBake,
                                                                         QString errorReason,
                                                                         QString redirectTarget) {
        auto type = assetTypeForFilename(originalAssetPath);
        auto currentTypeVersion = currentBakeVersionForAssetType(type);

        AssetMeta meta;
        meta.bakeVersion = currentTypeVersion;
        meta.failedLastBake = errorCompletingBake;
        meta.redirectTarget = redirectTarget;

        if (errorCompletingBake) {
            qWarning() << "Could not complete bake for" << originalAssetHash;
            meta.lastBakeErrors = errorReason;
        }

        writeMetaFile(originalAssetHash, meta);

        _pendingBakes.remove(originalAssetHash);
    };

    bool errorCompletingBake { false };
    QString errorReason;
    QString redirectTarget;

    qDebug() << "Completing bake for " << originalAssetHash;

    // Find the directory containing the baked content
    QDir outputDir(bakedTempOutputDir);
    QString outputDirName = outputDir.dirName();
    auto directories = outputDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    QString bakedDirectoryPath;
    for (const auto& dirName : directories) {
        outputDir.cd(dirName);
        if (outputDir.exists("baked") && outputDir.exists("original")) {
            bakedDirectoryPath = outputDir.filePath("baked");
            break;
        }
        outputDir.cdUp();
    }
    if (bakedDirectoryPath.isEmpty()) {
        errorCompletingBake = true;
        errorReason = "Failed to find baking output";

        // Cleanup temporary output directory
        PathUtils::deleteMyTemporaryDir(outputDirName);
        reportCompletion(errorCompletingBake, errorReason, redirectTarget);
        return;
    }

    // Compile list of all the baked files
    QDirIterator it(bakedDirectoryPath, QDirIterator::Subdirectories);
    QVector<QString> bakedFilePaths;
    while (it.hasNext()) {
        it.next();
        if (it.fileInfo().isFile()) {
            bakedFilePaths.push_back(it.filePath());
        }
    }
    if (bakedFilePaths.isEmpty()) {
        errorCompletingBake = true;
        errorReason = "Baking output has no files";

        // Cleanup temporary output directory
        PathUtils::deleteMyTemporaryDir(outputDirName);
        reportCompletion(errorCompletingBake, errorReason, redirectTarget);
        return;
    }

    QDir bakedDirectory(bakedDirectoryPath);

    for (auto& filePath : bakedFilePaths) {
        // figure out the hash for the contents of this file
        QFile file(filePath);

        qDebug() << "File path: " << filePath;

        AssetUtils::AssetHash bakedFileHash;

        if (!file.open(QIODevice::ReadOnly)) {
            qDebug() << "Failed to open baked file: " << filePath;
            // stop handling this bake, we couldn't open one of the files for reading
            errorCompletingBake = true;
            errorReason = "Could not open baked file " + file.fileName();
            break;
        }

        QCryptographicHash hasher(QCryptographicHash::Sha256);

        if (!hasher.addData(&file)) {
            // stop handling this bake, couldn't hash the contents of the file
            errorCompletingBake = true;
            errorReason = "Could not hash data for " + file.fileName();
            break;
        }

        bakedFileHash = hasher.result().toHex();

        // first check that we don't already have this bake file in our list
        auto bakeFileDestination = _filesDirectory.absoluteFilePath(bakedFileHash);
        if (!QFile::exists(bakeFileDestination)) {
            // copy each to our files folder (with the hash as their filename)
            if (!file.copy(_filesDirectory.absoluteFilePath(bakedFileHash))) {
                // stop handling this bake, couldn't copy the bake file into our files directory
                errorCompletingBake = true;
                errorReason = "Failed to copy baked assets to asset server";
                break;
            }
        }

        // setup the mapping for this bake file
        auto relativeFilePath = bakedDirectory.relativeFilePath(filePath);

        QString bakeMapping = getBakeMapping(originalAssetHash, relativeFilePath);

        // Check if this is the file we should redirect to when someone asks for the original asset
        if ((relativeFilePath.endsWith(".baked.fst", Qt::CaseInsensitive) && originalAssetPath.endsWith(".fbx")) ||
            (relativeFilePath.endsWith(".texmeta.json", Qt::CaseInsensitive) && !originalAssetPath.endsWith(".fbx"))) {
            if (!redirectTarget.isEmpty()) {
                qWarning() << "Found multiple baked redirect target for" << originalAssetPath;
            }
            redirectTarget = bakeMapping;
        }

        // add a mapping (under the hidden baked folder) for this file resulting from the bake
        if (!setMapping(bakeMapping, bakedFileHash)) {
            qDebug() << "Failed to set mapping";
            // stop handling this bake, couldn't add a mapping for this bake file
            errorCompletingBake = true;
            errorReason = "Failed to set mapping for baked file " + file.fileName();
            break;
        }

        qDebug() << "Added" << bakeMapping << "for bake file" << bakedFileHash << "from bake of" << originalAssetHash;
    }


    if (redirectTarget.isEmpty()) {
        errorCompletingBake = true;
        errorReason = "Could not find root file for baked output";
    }

    // Cleanup temporary output directory
    PathUtils::deleteMyTemporaryDir(outputDirName);
    reportCompletion(errorCompletingBake, errorReason, redirectTarget);
}

void AssetServer::handleAbortedBake(QString originalAssetHash, QString assetPath) {
    qDebug() << "Aborted bake:" << originalAssetHash;

    // for an aborted bake we don't do anything but remove the BakeAssetTask from our pending bakes
    _pendingBakes.remove(originalAssetHash);
}

static const QString BAKE_VERSION_KEY = "bake_version";
static const QString FAILED_LAST_BAKE_KEY = "failed_last_bake";
static const QString LAST_BAKE_ERRORS_KEY = "last_bake_errors";
static const QString REDIRECT_TARGET_KEY = "redirect_target";

std::pair<bool, AssetMeta> AssetServer::readMetaFile(AssetUtils::AssetHash hash) {
    auto metaFilePath = AssetUtils::HIDDEN_BAKED_CONTENT_FOLDER + hash + "/" + "meta.json";

    auto it = _fileMappings.find(metaFilePath);
    if (it == _fileMappings.end()) {
        return { false, {} };
    }

    auto metaFileHash = it->second;

    QFile metaFile(_filesDirectory.absoluteFilePath(metaFileHash));

    if (metaFile.open(QIODevice::ReadOnly)) {
        auto data = metaFile.readAll();
        metaFile.close();

        QJsonParseError error;
        auto doc = QJsonDocument::fromJson(data, &error);

        if (error.error == QJsonParseError::NoError && doc.isObject()) {
            auto root = doc.object();

            auto bakeVersion = root[BAKE_VERSION_KEY];
            auto failedLastBake = root[FAILED_LAST_BAKE_KEY];
            auto lastBakeErrors = root[LAST_BAKE_ERRORS_KEY];
            auto redirectTarget = root[REDIRECT_TARGET_KEY];

            if (bakeVersion.isDouble()
                && failedLastBake.isBool()
                && lastBakeErrors.isString()) {

                AssetMeta meta;
                meta.bakeVersion = bakeVersion.toInt();
                meta.failedLastBake = failedLastBake.toBool();
                meta.lastBakeErrors = lastBakeErrors.toString();
                meta.redirectTarget = redirectTarget.toString();

                return { true, meta };
            } else {
                qCWarning(asset_server) << "Metafile for" << hash << "has either missing or malformed data.";
            }
        }
    }

    return { false, {} };
}

bool AssetServer::writeMetaFile(AssetUtils::AssetHash originalAssetHash, const AssetMeta& meta) {
    // construct the JSON that will be in the meta file
    QJsonObject metaFileObject;

    metaFileObject[BAKE_VERSION_KEY] = (int)meta.bakeVersion;
    metaFileObject[FAILED_LAST_BAKE_KEY] = meta.failedLastBake;
    metaFileObject[LAST_BAKE_ERRORS_KEY] = meta.lastBakeErrors;
    metaFileObject[REDIRECT_TARGET_KEY] = meta.redirectTarget;

    QJsonDocument metaFileDoc;
    metaFileDoc.setObject(metaFileObject);

    auto metaFileJSON = metaFileDoc.toJson();

    // get a hash for the contents of the meta-file
    AssetUtils::AssetHash metaFileHash = QCryptographicHash::hash(metaFileJSON, QCryptographicHash::Sha256).toHex();

    // create the meta file in our files folder, named by the hash of its contents
    QFile metaFile(_filesDirectory.absoluteFilePath(metaFileHash));

    if (metaFile.open(QIODevice::WriteOnly)) {
        metaFile.write(metaFileJSON);
        metaFile.close();

        // add a mapping to the meta file so it doesn't get deleted because it is unmapped
        auto metaFileMapping = AssetUtils::HIDDEN_BAKED_CONTENT_FOLDER + originalAssetHash + "/" + "meta.json";

        return setMapping(metaFileMapping, metaFileHash);
    } else {
        return false;
    }
}

bool AssetServer::setBakingEnabled(const AssetUtils::AssetPathList& paths, bool enabled) {
    for (const auto& path : paths) {
        auto it = _fileMappings.find(path);
        if (it != _fileMappings.end()) {
            auto type = assetTypeForFilename(path);
            if (type == BakedAssetType::Undefined) {
                continue;
            }

            auto hash = it->second;

            bool loaded;
            AssetMeta meta;
            std::tie(loaded, meta) = readMetaFile(hash);

            QString bakedFilename = bakedFilenameForAssetType(type);
            auto bakedMapping = getBakeMapping(hash, bakedFilename);
            if (loaded && !meta.redirectTarget.isEmpty()) {
                bakedMapping = meta.redirectTarget;
            }

            auto it = _fileMappings.find(bakedMapping);
            bool currentlyDisabled = (it != _fileMappings.end() && it->second == hash);

            if (enabled && currentlyDisabled) {
                QStringList bakedMappings{ bakedMapping };
                deleteMappings(bakedMappings);
                maybeBake(path, hash);
                qDebug() << "Enabled baking for" << path;
            } else if (!enabled && !currentlyDisabled) {
                removeBakedPathsForDeletedAsset(hash);
                setMapping(bakedMapping, hash);
                qDebug() << "Disabled baking for" << path;
            }
        }
    }
    return true;
}
