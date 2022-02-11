//
//  AssetServer.h
//  assignment-client/src/assets
//
//  Created by Ryan Huffman on 2015/07/21
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AssetServer_h
#define hifi_AssetServer_h

#include <QtCore/QDir>
#include <QtCore/QSharedPointer>
#include <QtCore/QThreadPool>
#include <QRunnable>

#include <ThreadedAssignment.h>

#include "AssetUtils.h"
#include "ReceivedMessage.h"

#include "RegisteredMetaTypes.h"

using BakeVersion = int;
static const BakeVersion INITIAL_BAKE_VERSION = 0;
static const BakeVersion NEEDS_BAKING_BAKE_VERSION = -1;

enum class BakedAssetType : int {
    Model = 0,
    Texture,
    Script,

    NUM_ASSET_TYPES,
    Undefined
};

// ATTENTION! Do not remove baking versions, and do not reorder them. If you add
// a new value, it will immediately become the "current" version.
enum class ModelBakeVersion : BakeVersion {
    Initial = INITIAL_BAKE_VERSION,
    MetaTextureJson,

    COUNT
};

// ATTENTION! See above.
enum class TextureBakeVersion : BakeVersion {
    Initial = INITIAL_BAKE_VERSION,
    MetaTextureJson,

    COUNT
};

// ATTENTION! See above.
enum class ScriptBakeVersion : BakeVersion {
    Initial = INITIAL_BAKE_VERSION,
    FixEmptyScripts,

    COUNT
};

struct AssetMeta {
    BakeVersion bakeVersion { INITIAL_BAKE_VERSION };
    bool failedLastBake { false };
    QString lastBakeErrors;
    QString redirectTarget;
};

class BakeAssetTask;

class AssetServer : public ThreadedAssignment {
    Q_OBJECT
public:
    AssetServer(ReceivedMessage& message);

    void aboutToFinish() override;

public slots:
    void run() override;

private slots:
    void completeSetup();

    void queueRequests(QSharedPointer<ReceivedMessage> packet, SharedNodePointer senderNode);
    void handleAssetGetInfo(QSharedPointer<ReceivedMessage> packet, SharedNodePointer senderNode);
    void handleAssetGet(QSharedPointer<ReceivedMessage> packet, SharedNodePointer senderNode);
    void handleAssetUpload(QSharedPointer<ReceivedMessage> packetList, SharedNodePointer senderNode);
    void handleAssetMappingOperation(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode);

    void sendStatsPacket() override;

private:
    void replayRequests();

    void handleGetMappingOperation(ReceivedMessage& message, NLPacketList& replyPacket);
    void handleGetAllMappingOperation(NLPacketList& replyPacket);
    void handleSetMappingOperation(ReceivedMessage& message, bool hasWriteAccess, NLPacketList& replyPacket);
    void handleDeleteMappingsOperation(ReceivedMessage& message, bool hasWriteAccess, NLPacketList& replyPacket);
    void handleRenameMappingOperation(ReceivedMessage& message, bool hasWriteAccess, NLPacketList& replyPacket);
    void handleSetBakingEnabledOperation(ReceivedMessage& message, bool hasWriteAccess, NLPacketList& replyPacket);

    void handleAssetServerBackup(ReceivedMessage& message, NLPacketList& replyPacket);
    void handleAssetServerRestore(ReceivedMessage& message, NLPacketList& replyPacket);

    // Mapping file operations must be called from main assignment thread only
    bool loadMappingsFromFile();
    bool writeMappingsToFile();

    /// Set the mapping for path to hash
    bool setMapping(AssetUtils::AssetPath path, AssetUtils::AssetHash hash);

    /// Delete mapping `path`. Returns `true` if deletion of mappings succeeds, else `false`.
    bool deleteMappings(const AssetUtils::AssetPathList& paths);

    /// Rename mapping from `oldPath` to `newPath`. Returns true if successful
    bool renameMapping(AssetUtils::AssetPath oldPath, AssetUtils::AssetPath newPath);

    bool setBakingEnabled(const AssetUtils::AssetPathList& paths, bool enabled);

    /// Delete any unmapped files from the local asset directory
    void cleanupUnmappedFiles();

    /// Delete any baked files for assets removed from the local asset directory
    void cleanupBakedFilesForDeletedAssets();

    QString getPathToAssetHash(const AssetUtils::AssetHash& assetHash);

    std::pair<AssetUtils::BakingStatus, QString> getAssetStatus(const AssetUtils::AssetPath& path, const AssetUtils::AssetHash& hash);

    void bakeAssets();
    void maybeBake(const AssetUtils::AssetPath& path, const AssetUtils::AssetHash& hash);
    void createEmptyMetaFile(const AssetUtils::AssetHash& hash);
    bool hasMetaFile(const AssetUtils::AssetHash& hash);
    bool needsToBeBaked(const AssetUtils::AssetPath& path, const AssetUtils::AssetHash& assetHash);
    void bakeAsset(const AssetUtils::AssetHash& assetHash, const AssetUtils::AssetPath& assetPath, const QString& filePath);

    /// Move baked content for asset to baked directory and update baked status
    void handleCompletedBake(QString originalAssetHash, QString assetPath, QString bakedTempOutputDir);
    void handleFailedBake(QString originalAssetHash, QString assetPath, QString errors);
    void handleAbortedBake(QString originalAssetHash, QString assetPath);

    /// Create meta file to describe baked content for original asset
    std::pair<bool, AssetMeta> readMetaFile(AssetUtils::AssetHash hash);
    bool writeMetaFile(AssetUtils::AssetHash originalAssetHash, const AssetMeta& meta = AssetMeta());

    /// Remove baked paths when the original asset is deleteds
    void removeBakedPathsForDeletedAsset(AssetUtils::AssetHash originalAssetHash);

    AssetUtils::Mappings _fileMappings;

    QDir _resourcesDirectory;
    QDir _filesDirectory;

    /// Task pool for handling uploads and downloads of assets
    QThreadPool _transferTaskPool;

    QHash<AssetUtils::AssetHash, std::shared_ptr<BakeAssetTask>> _pendingBakes;
    QThreadPool _bakingTaskPool;

    QMutex _queuedRequestsMutex;
    bool _isQueueingRequests { true };
    using RequestQueue = QVector<QPair<QSharedPointer<ReceivedMessage>, SharedNodePointer>>;
    RequestQueue _queuedRequests;

    uint64_t _filesizeLimit;
};

#endif
