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

#include <QDir>

#include <ThreadedAssignment.h>
#include <QThreadPool>

#include "AssetUtils.h"
#include "ReceivedMessage.h"

class AssetServer : public ThreadedAssignment {
    Q_OBJECT
public:
    AssetServer(ReceivedMessage& message);

public slots:
    void run();

private slots:
    void completeSetup();

    void handleAssetGetInfo(QSharedPointer<ReceivedMessage> packet, SharedNodePointer senderNode);
    void handleAssetGet(QSharedPointer<ReceivedMessage> packet, SharedNodePointer senderNode);
    void handleAssetUpload(QSharedPointer<ReceivedMessage> packetList, SharedNodePointer senderNode);
    void handleAssetMappingOperation(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode);
    
    void sendStatsPacket();
    
private:
    using Mappings = QVariantHash;

    void handleGetMappingOperation(ReceivedMessage& message, SharedNodePointer senderNode, NLPacketList& replyPacket);
    void handleSetMappingOperation(ReceivedMessage& message, SharedNodePointer senderNode, NLPacketList& replyPacket);
    void handleDeleteMappingOperation(ReceivedMessage& message, SharedNodePointer senderNode, NLPacketList& replyPacket);

    // Mapping file operations must be called from main assignment thread only
    void loadMappingsFromFile();
    bool writeMappingsToFile();

    /// Return the hash mapping for AssetPath `path`
    AssetHash getMapping(AssetPath path);

    /// Set the mapping for path to hash
    bool setMapping(AssetPath path, AssetHash hash);

    /// Delete mapping `path`. Return `true` if mapping existed, else `false`.
    bool deleteMapping(AssetPath path);

    static void writeError(NLPacketList* packetList, AssetServerError error);

    void performMappingMigration();

    Mappings _fileMappings;

    QDir _resourcesDirectory;
    QDir _filesDirectory;
    QThreadPool _taskPool;
};

inline void writeError(NLPacketList* packetList, AssetServerError error) {
    packetList->writePrimitive(error);
}

#endif
