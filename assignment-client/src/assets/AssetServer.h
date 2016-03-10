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
    
    void sendStatsPacket();
    
private:
    static void writeError(NLPacketList* packetList, AssetServerError error);
    QDir _resourcesDirectory;
    QThreadPool _taskPool;
};

inline void writeError(NLPacketList* packetList, AssetServerError error) {
    packetList->writePrimitive(error);
}

#endif
