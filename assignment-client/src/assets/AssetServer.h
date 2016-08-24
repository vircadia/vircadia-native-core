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

#include <ThreadedAssignment.h>

#include "AssetUtils.h"
#include "ReceivedMessage.h"

class AssetServer : public ThreadedAssignment {
    Q_OBJECT
public:
    AssetServer(ReceivedMessage& message);

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
    using Mappings = QVariantHash;

    void handleGetMappingOperation(ReceivedMessage& message, SharedNodePointer senderNode, NLPacketList& replyPacket);
    void handleGetAllMappingOperation(ReceivedMessage& message, SharedNodePointer senderNode, NLPacketList& replyPacket);
    void handleSetMappingOperation(ReceivedMessage& message, SharedNodePointer senderNode, NLPacketList& replyPacket);
    void handleDeleteMappingsOperation(ReceivedMessage& message, SharedNodePointer senderNode, NLPacketList& replyPacket);
    void handleRenameMappingOperation(ReceivedMessage& message, SharedNodePointer senderNode, NLPacketList& replyPacket);

    // Mapping file operations must be called from main assignment thread only
    bool loadMappingsFromFile();
    bool writeMappingsToFile();

    /// Set the mapping for path to hash
    bool setMapping(AssetPath path, AssetHash hash);

    /// Delete mapping `path`. Returns `true` if deletion of mappings succeeds, else `false`.
    bool deleteMappings(AssetPathList& paths);

    /// Rename mapping from `oldPath` to `newPath`. Returns true if successful
    bool renameMapping(AssetPath oldPath, AssetPath newPath);

    // deletes any unmapped files from the local asset directory
    void cleanupUnmappedFiles();

    Mappings _fileMappings;

    QDir _resourcesDirectory;
    QDir _filesDirectory;
    QThreadPool _taskPool;
};

#endif
