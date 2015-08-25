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

#include <DependencyManager.h>

#include "AssetUtils.h"
#include "LimitedNodeList.h"
#include "NLPacket.h"

class AssetRequest;

struct AssetInfo {
    QString hash;
    int64_t size;
};

using ReceivedAssetCallback = std::function<void(bool result, QByteArray data)>;
using GetInfoCallback = std::function<void(bool result, AssetInfo info)>;
using UploadResultCallback = std::function<void(bool result, QString hash)>;

class AssetClient : public QObject, public Dependency {
    Q_OBJECT
public:
    AssetClient();

    Q_INVOKABLE AssetRequest* create(QString hash);
    bool getAssetInfo(QString hash, GetInfoCallback callback);
    bool getAsset(QString hash, DataOffset start, DataOffset end, ReceivedAssetCallback callback);
    bool uploadAsset(QByteArray data, QString extension, UploadResultCallback callback);
    bool abortDataRequest(MessageID messageID);

private slots:
    void handleAssetGetInfoReply(QSharedPointer<NLPacket> packet, SharedNodePointer senderNode);
    void handleAssetGetReply(QSharedPointer<NLPacketList> packetList, SharedNodePointer senderNode);
    void handleAssetUploadReply(QSharedPointer<NLPacket> packet, SharedNodePointer senderNode);

private:
    static MessageID _currentID;
    QHash<MessageID, ReceivedAssetCallback> _pendingRequests;
    QHash<MessageID, GetInfoCallback> _pendingInfoRequests;
    QHash<MessageID, UploadResultCallback> _pendingUploads;
};

#endif
