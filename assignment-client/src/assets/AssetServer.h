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
#include <QtCore/QThreadPool>
#include <QRunnable>

#include <ThreadedAssignment.h>

#include "AssetUtils.h"
#include "ReceivedMessage.h"

namespace std {
    template <>
    struct hash<QString> {
        size_t operator()(const QString& v) const { return qHash(v); }
    };
}

struct AssetMeta {
    AssetMeta() {
    }

    int bakeVersion { 0 };
    bool failedLastBake { false };
    QString lastBakeErrors;
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

    void handleAssetGetInfo(QSharedPointer<ReceivedMessage> packet, SharedNodePointer senderNode);
    void handleAssetGet(QSharedPointer<ReceivedMessage> packet, SharedNodePointer senderNode);
    void handleAssetUpload(QSharedPointer<ReceivedMessage> packetList, SharedNodePointer senderNode);
    void handleAssetMappingOperation(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode);

    void sendStatsPacket() override;

private:
    using Mappings = std::unordered_map<QString, QString>;

    void handleGetMappingOperation(ReceivedMessage& message, SharedNodePointer senderNode, NLPacketList& replyPacket);
    void handleGetAllMappingOperation(ReceivedMessage& message, SharedNodePointer senderNode, NLPacketList& replyPacket);
    void handleSetMappingOperation(ReceivedMessage& message, SharedNodePointer senderNode, NLPacketList& replyPacket);
    void handleDeleteMappingsOperation(ReceivedMessage& message, SharedNodePointer senderNode, NLPacketList& replyPacket);
    void handleRenameMappingOperation(ReceivedMessage& message, SharedNodePointer senderNode, NLPacketList& replyPacket);
    void handleSetBakingEnabledOperation(ReceivedMessage& message, SharedNodePointer senderNode, NLPacketList& replyPacket);

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

    QString getPathToAssetHash(const AssetUtils::AssetHash& assetHash);

    std::pair<AssetUtils::BakingStatus, QString> getAssetStatus(const AssetUtils::AssetPath& path, const AssetUtils::AssetHash& hash);

    void bakeAssets();
    void maybeBake(const AssetUtils::AssetPath& path, const AssetUtils::AssetHash& hash);
    void createEmptyMetaFile(const AssetUtils::AssetHash& hash);
    bool hasMetaFile(const AssetUtils::AssetHash& hash);
    bool needsToBeBaked(const AssetUtils::AssetPath& path, const AssetUtils::AssetHash& assetHash);
    void bakeAsset(const AssetUtils::AssetHash& assetHash, const AssetUtils::AssetPath& assetPath, const QString& filePath);

    /// Move baked content for asset to baked directory and update baked status
    void handleCompletedBake(QString originalAssetHash, QString assetPath, QString bakedTempOutputDir,
                             QVector<QString> bakedFilePaths);
    void handleFailedBake(QString originalAssetHash, QString assetPath, QString errors);
    void handleAbortedBake(QString originalAssetHash, QString assetPath);

    /// Create meta file to describe baked content for original asset
    std::pair<bool, AssetMeta> readMetaFile(AssetUtils::AssetHash hash);
    bool writeMetaFile(AssetUtils::AssetHash originalAssetHash, const AssetMeta& meta = AssetMeta());

    /// Remove baked paths when the original asset is deleteds
    void removeBakedPathsForDeletedAsset(AssetUtils::AssetHash originalAssetHash);

    Mappings _fileMappings;

    QDir _resourcesDirectory;
    QDir _filesDirectory;

    /// Task pool for handling uploads and downloads of assets
    QThreadPool _transferTaskPool;

    QHash<AssetUtils::AssetHash, std::shared_ptr<BakeAssetTask>> _pendingBakes;
    QThreadPool _bakingTaskPool;

    bool _wasColorTextureCompressionEnabled { false };
    bool _wasGrayscaleTextureCompressionEnabled { false  };
    bool _wasNormalTextureCompressionEnabled { false };
    bool _wasCubeTextureCompressionEnabled { false };

    uint64_t _filesizeLimit;
};

#endif
