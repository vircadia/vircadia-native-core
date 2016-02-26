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

#include <QString>
#include <QScriptValue>

#include <DependencyManager.h>

#include "AssetUtils.h"
#include "LimitedNodeList.h"
#include "NLPacket.h"
#include "Node.h"
#include "ReceivedMessage.h"
#include "ResourceCache.h"

class AssetRequest;
class AssetUpload;

struct AssetInfo {
    QString hash;
    int64_t size;
};

using ReceivedAssetCallback = std::function<void(bool responseReceived, AssetServerError serverError, const QByteArray& data)>;
using GetInfoCallback = std::function<void(bool responseReceived, AssetServerError serverError, AssetInfo info)>;
using UploadResultCallback = std::function<void(bool responseReceived, AssetServerError serverError, const QString& hash)>;
using ProgressCallback = std::function<void(qint64 totalReceived, qint64 total)>;


class AssetClient : public QObject, public Dependency {
    Q_OBJECT
public:
    AssetClient();

    Q_INVOKABLE AssetRequest* createRequest(const QString& hash, const QString& extension);
    Q_INVOKABLE AssetUpload* createUpload(const QString& filename);
    Q_INVOKABLE AssetUpload* createUpload(const QByteArray& data, const QString& extension);

public slots:
    void init();

    void cacheInfoRequest(QObject* reciever, QString slot);
    void clearCache();

private slots:
    void handleAssetGetInfoReply(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode);
    void handleAssetGetReply(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode);
    void handleAssetUploadReply(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode);

    void handleNodeKilled(SharedNodePointer node);

private:
    bool getAssetInfo(const QString& hash, const QString& extension, GetInfoCallback callback);
    bool getAsset(const QString& hash, const QString& extension, DataOffset start, DataOffset end,
                  ReceivedAssetCallback callback, ProgressCallback progressCallback);
    bool uploadAsset(const QByteArray& data, const QString& extension, UploadResultCallback callback);

    struct GetAssetCallbacks {
        ReceivedAssetCallback completeCallback;
        ProgressCallback progressCallback;
    };

    static MessageID _currentID;
    std::unordered_map<SharedNodePointer, std::unordered_map<MessageID, GetAssetCallbacks>> _pendingRequests;
    std::unordered_map<SharedNodePointer, std::unordered_map<MessageID, GetInfoCallback>> _pendingInfoRequests;
    std::unordered_map<SharedNodePointer, std::unordered_map<MessageID, UploadResultCallback>> _pendingUploads;
    
    friend class AssetRequest;
    friend class AssetUpload;
};


class AssetScriptingInterface : public QObject {
    Q_OBJECT
public:
    AssetScriptingInterface(QScriptEngine* engine);

    Q_INVOKABLE void uploadData(QString data, QString extension, QScriptValue callback);
    Q_INVOKABLE void downloadData(QString url, QScriptValue downloadComplete);
protected:
    QSet<AssetRequest*> _pendingRequests;
    QScriptEngine* _engine;
};


#endif
