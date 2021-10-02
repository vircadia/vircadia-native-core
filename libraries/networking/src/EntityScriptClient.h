//
//  EntityScriptClient.h
//  libraries/networking/src
//
//  Created by Ryan Huffman on 2017/01/13
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_EntityScriptClient_h
#define hifi_EntityScriptClient_h

#include <QtCore/QSharedPointer>

#include "ClientServerUtils.h"
#include "LimitedNodeList.h"
#include "ReceivedMessage.h"
#include "AssetUtils.h"
#include "EntityScriptUtils.h"

#include <DependencyManager.h>
#include <unordered_map>

using GetScriptStatusCallback = std::function<void(bool responseReceived, bool isRunning, EntityScriptStatus status, QString errorInfo)>;

class GetScriptStatusRequest : public QObject {
    Q_OBJECT
public:
    GetScriptStatusRequest(QUuid);
    ~GetScriptStatusRequest();

    Q_INVOKABLE void start();

    bool getResponseReceived() const { return _responseReceived; }
    bool getIsRunning() const { return _isRunning; }
    EntityScriptStatus getStatus() const { return _status; }
    QString getErrorInfo() const { return _errorInfo;  }

signals:
    void finished(GetScriptStatusRequest* request);

private:
    QUuid _entityID;
    MessageID _messageID;

    bool _responseReceived;
    bool _isRunning;
    EntityScriptStatus _status;
    QString _errorInfo;
};

class EntityScriptClient : public QObject, public Dependency {
    Q_OBJECT
public:
    EntityScriptClient();

    Q_INVOKABLE GetScriptStatusRequest* createScriptStatusRequest(QUuid entityID);

    bool reloadServerScript(QUuid entityID);
    MessageID getEntityServerScriptStatus(QUuid entityID, GetScriptStatusCallback callback);
    void callEntityServerMethod(QUuid id, const QString& method, const QStringList& params);


private slots:
    void handleNodeKilled(SharedNodePointer node);
    void handleNodeClientConnectionReset(SharedNodePointer node);

    void handleGetScriptStatusReply(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode);

private:
    static MessageID _currentID;
    std::unordered_map<SharedNodePointer, std::unordered_map<MessageID, GetScriptStatusCallback>> _pendingEntityScriptStatusRequests;

    void forceFailureOfPendingRequests(SharedNodePointer node);
};


class EntityScriptServerServices : public QObject, public Dependency {
    Q_OBJECT
public:
    void callEntityClientMethod(QUuid clientSessionID, QUuid entityID, const QString& method, const QStringList& params);
};

#endif
