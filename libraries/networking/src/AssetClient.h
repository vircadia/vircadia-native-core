//
//  AssetClient.h
//  libraries/networking/src
//
//  Created by Ryan Huffman on 2015/07/21
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AssetClient_h
#define hifi_AssetClient_h

#include <QStandardItemModel>
#include <QtQml/QJSEngine>
#include <QString>

#include <map>

#include <DependencyManager.h>

#include "AssetUtils.h"
#include "LimitedNodeList.h"
#include "NLPacket.h"
#include "Node.h"
#include "ReceivedMessage.h"
#include "ResourceCache.h"

class GetMappingRequest;
class SetMappingRequest;
class GetAllMappingsRequest;
class DeleteMappingsRequest;
class RenameMappingRequest;
class AssetRequest;
class AssetUpload;

struct AssetInfo {
    QString hash;
    int64_t size;
};

using MappingOperationCallback = std::function<void(bool responseReceived, AssetServerError serverError, QSharedPointer<ReceivedMessage> message)>;
using ReceivedAssetCallback = std::function<void(bool responseReceived, AssetServerError serverError, const QByteArray& data)>;
using GetInfoCallback = std::function<void(bool responseReceived, AssetServerError serverError, AssetInfo info)>;
using UploadResultCallback = std::function<void(bool responseReceived, AssetServerError serverError, const QString& hash)>;
using ProgressCallback = std::function<void(qint64 totalReceived, qint64 total)>;

class AssetClient : public QObject, public Dependency {
    Q_OBJECT
public:
    AssetClient();

    Q_INVOKABLE GetMappingRequest* createGetMappingRequest(const AssetPath& path);
    Q_INVOKABLE GetAllMappingsRequest* createGetAllMappingsRequest();
    Q_INVOKABLE DeleteMappingsRequest* createDeleteMappingsRequest(const AssetPathList& paths);
    Q_INVOKABLE SetMappingRequest* createSetMappingRequest(const AssetPath& path, const AssetHash& hash);
    Q_INVOKABLE RenameMappingRequest* createRenameMappingRequest(const AssetPath& oldPath, const AssetPath& newPath);
    Q_INVOKABLE AssetRequest* createRequest(const AssetHash& hash);
    Q_INVOKABLE AssetUpload* createUpload(const QString& filename);
    Q_INVOKABLE AssetUpload* createUpload(const QByteArray& data);

    static const MessageID INVALID_MESSAGE_ID = 0;

public slots:
    void init();

    void cacheInfoRequest(QObject* reciever, QString slot);
    void clearCache();

private slots:
    void handleAssetMappingOperationReply(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode);
    void handleAssetGetInfoReply(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode);
    void handleAssetGetReply(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode);
    void handleAssetUploadReply(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode);

    void handleNodeKilled(SharedNodePointer node);

private:
    MessageID getAssetMapping(const AssetHash& hash, MappingOperationCallback callback);
    MessageID getAllAssetMappings(MappingOperationCallback callback);
    MessageID setAssetMapping(const QString& path, const AssetHash& hash, MappingOperationCallback callback);
    MessageID deleteAssetMappings(const AssetPathList& paths, MappingOperationCallback callback);
    MessageID renameAssetMapping(const AssetPath& oldPath, const AssetPath& newPath, MappingOperationCallback callback);

    MessageID getAssetInfo(const QString& hash, GetInfoCallback callback);
    MessageID getAsset(const QString& hash, DataOffset start, DataOffset end,
                  ReceivedAssetCallback callback, ProgressCallback progressCallback);
    MessageID uploadAsset(const QByteArray& data, UploadResultCallback callback);

    bool cancelMappingRequest(MessageID id);
    bool cancelGetAssetInfoRequest(MessageID id);
    bool cancelGetAssetRequest(MessageID id);
    bool cancelUploadAssetRequest(MessageID id);

    void handleProgressCallback(const QWeakPointer<Node>& node, MessageID messageID, qint64 size, DataOffset length);
    void handleCompleteCallback(const QWeakPointer<Node>& node, MessageID messageID);

    struct GetAssetRequestData {
        QSharedPointer<ReceivedMessage> message;
        ReceivedAssetCallback completeCallback;
        ProgressCallback progressCallback;
    };

    static MessageID _currentID;
    std::unordered_map<SharedNodePointer, std::unordered_map<MessageID, MappingOperationCallback>> _pendingMappingRequests;
    std::unordered_map<SharedNodePointer, std::unordered_map<MessageID, GetAssetRequestData>> _pendingRequests;
    std::unordered_map<SharedNodePointer, std::unordered_map<MessageID, GetInfoCallback>> _pendingInfoRequests;
    std::unordered_map<SharedNodePointer, std::unordered_map<MessageID, UploadResultCallback>> _pendingUploads;

    friend class AssetRequest;
    friend class AssetUpload;
    friend class MappingRequest;
    friend class GetMappingRequest;
    friend class GetAllMappingsRequest;
    friend class SetMappingRequest;
    friend class DeleteMappingsRequest;
    friend class RenameMappingRequest;
};

#endif
