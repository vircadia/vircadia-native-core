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

class MappingRequest : public QObject {
    Q_OBJECT
public:
    enum Error {
        NoError,
        NotFound,
        NetworkError,
        PermissionDenied,
        UnknownError
    };

    Q_INVOKABLE void start();
    Error getError() const { return _error; }

protected:
    Error _error { NoError };

private:
    virtual void doStart() = 0;
};


class GetMappingRequest : public MappingRequest {
    Q_OBJECT
public:
    GetMappingRequest(const AssetPath& path);

    AssetHash getHash() const { return _hash;  }

signals:
    void finished(GetMappingRequest* thisRequest);

private:
    virtual void doStart() override;

    AssetPath _path;
    AssetHash _hash;
};

class SetMappingRequest : public MappingRequest {
    Q_OBJECT
public:
    SetMappingRequest(const AssetPath& path, const AssetHash& hash);

    AssetHash getHash() const { return _hash;  }

signals:
    void finished(SetMappingRequest* thisRequest);

private:
    virtual void doStart() override;

    AssetPath _path;
    AssetHash _hash;
};

class DeleteMappingsRequest : public MappingRequest {
    Q_OBJECT
public:
    DeleteMappingsRequest(const AssetPathList& path);

signals:
    void finished(DeleteMappingsRequest* thisRequest);

private:
    virtual void doStart() override;

    AssetPathList _paths;
};

class RenameMappingRequest : public MappingRequest {
    Q_OBJECT
public:
    RenameMappingRequest(const AssetPath& oldPath, const AssetPath& newPath);

signals:
    void finished(RenameMappingRequest* thisRequest);

private:
    virtual void doStart() override;

    AssetPath _oldPath;
    AssetPath _newPath;
};

class GetAllMappingsRequest : public MappingRequest {
    Q_OBJECT
public:
    GetAllMappingsRequest();

    AssetMapping getMappings() const { return _mappings;  }

signals:
    void finished(GetAllMappingsRequest* thisRequest);

private:
    virtual void doStart() override;

    std::map<AssetPath, AssetHash> _mappings;
};

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
    bool getAssetMapping(const AssetHash& hash, MappingOperationCallback callback);
    bool getAllAssetMappings(MappingOperationCallback callback);
    bool setAssetMapping(const QString& path, const AssetHash& hash, MappingOperationCallback callback);
    bool deleteAssetMappings(const AssetPathList& paths, MappingOperationCallback callback);
    bool renameAssetMapping(const AssetPath& oldPath, const AssetPath& newPath, MappingOperationCallback callback);

    bool getAssetInfo(const QString& hash, GetInfoCallback callback);
    bool getAsset(const QString& hash, DataOffset start, DataOffset end,
                  ReceivedAssetCallback callback, ProgressCallback progressCallback);
    bool uploadAsset(const QByteArray& data, UploadResultCallback callback);

    struct GetAssetCallbacks {
        ReceivedAssetCallback completeCallback;
        ProgressCallback progressCallback;
    };

    static MessageID _currentID;
    std::unordered_map<SharedNodePointer, std::unordered_map<MessageID, MappingOperationCallback>> _pendingMappingRequests;
    std::unordered_map<SharedNodePointer, std::unordered_map<MessageID, GetAssetCallbacks>> _pendingRequests;
    std::unordered_map<SharedNodePointer, std::unordered_map<MessageID, GetInfoCallback>> _pendingInfoRequests;
    std::unordered_map<SharedNodePointer, std::unordered_map<MessageID, UploadResultCallback>> _pendingUploads;

    QHash<QString, QString> _mappingCache;
    
    friend class AssetRequest;
    friend class AssetUpload;
    friend class GetMappingRequest;
    friend class GetAllMappingsRequest;
    friend class SetMappingRequest;
    friend class DeleteMappingsRequest;
    friend class RenameMappingRequest;
};


class AssetScriptingInterface : public QObject {
    Q_OBJECT
public:
    AssetScriptingInterface(QScriptEngine* engine);

    Q_INVOKABLE void uploadData(QString data, QScriptValue callback);
    Q_INVOKABLE void downloadData(QString url, QScriptValue downloadComplete);
    Q_INVOKABLE void setMapping(QString path, QString hash, QScriptValue callback);
    Q_INVOKABLE void getMapping(QString path, QScriptValue callback);
    Q_INVOKABLE void deleteMappings(QStringList paths, QScriptValue callback);
    Q_INVOKABLE void getAllMappings(QScriptValue callback);
    Q_INVOKABLE void renameMapping(QString oldPath, QString newPath, QScriptValue callback);
protected:
    QSet<AssetRequest*> _pendingRequests;
    QScriptEngine* _engine;
};


#endif
