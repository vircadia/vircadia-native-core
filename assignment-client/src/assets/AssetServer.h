//
//  AssetServer.h
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

class AssetServer : public ThreadedAssignment {
    Q_OBJECT
public:
    AssetServer(NLPacket& packet);
    ~AssetServer();

    enum ServerResponse {
        ASSET_NOT_FOUND = 0,
        ASSET_UPLOADED = 1,
    };

public slots:
    void run();

private slots:
    void handleAssetGet(QSharedPointer<NLPacket> packet, SharedNodePointer senderNode);
    void handleAssetUpload(QSharedPointer<NLPacket> packet, SharedNodePointer senderNode);

private:
    QDir _resourcesDirectory;
};

#endif
