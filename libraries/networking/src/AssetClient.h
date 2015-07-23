//
//  AssetClient.h
//
//  Created by Ryan Huffman on 2015/07/21
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#ifndef hifi_AssetClient_h
#define hifi_AssetClient_h

#include <QString>
#include <QScriptValue>

#include <DependencyManager.h>

#include "LimitedNodeList.h"
#include "NLPacket.h"

using ReceivedAssetCallback = std::function<void(bool result, QByteArray data)>;
using UploadResultCallback = std::function<void(bool result, QString hash)>;
using MessageID = int;

class AssetClient : public QObject, public Dependency {
    Q_OBJECT
public:
    AssetClient();

    enum RequestResult {
        SUCCESS = 0,
        FAILURE,
        TIMEOUT
    };

    bool getAsset(QString hash, ReceivedAssetCallback callback);
    bool uploadAsset(QByteArray data, QString extension, UploadResultCallback callback);

private slots:
    void handleAssetGetReply(QSharedPointer<NLPacket> packet, SharedNodePointer senderNode);
    void handleAssetUploadReply(QSharedPointer<NLPacket> packet, SharedNodePointer senderNode);

private:
    static MessageID _currentID;
    QMap<QString, ReceivedAssetCallback> _pendingRequests;
    QMap<MessageID, UploadResultCallback> _pendingUploads;
};

#endif
