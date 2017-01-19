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

#include "LimitedNodeList.h"
#include "ReceivedMessage.h"
#include "AssetUtils.h"

#include <DependencyManager.h>
#include <cstdint>
#include <unordered_map>

using MessageID = uint32_t;

using GetScriptStatusCallback = std::function<void(bool responseReceived)>;

class GetScriptStatusRequest : public QObject {
    Q_OBJECT
public:
    GetScriptStatusRequest(QUuid);
    ~GetScriptStatusRequest();

    Q_INVOKABLE void start();

signals:
    void finished(GetScriptStatusRequest* request);

private:
    QUuid _entityID;
    MessageID _messageID;
    QString status;
};

class EntityScriptClient : public QObject, public Dependency {
    Q_OBJECT
public:
    EntityScriptClient();

    Q_INVOKABLE GetScriptStatusRequest* createScriptStatusRequest(QUuid entityID);

    bool reloadServerScript(QUuid entityID);
    MessageID getEntityServerScriptStatus(QUuid entityID, GetScriptStatusCallback callback);

private slots:
    void handleNodeKilled(SharedNodePointer node);
    void handleNodeClientConnectionReset(SharedNodePointer node);

    void handleGetScriptStatusReply(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode);

private:
    static MessageID _currentID;
    std::unordered_map<SharedNodePointer, std::unordered_map<MessageID, GetScriptStatusCallback>> _pendingEntityScriptStatusRequests;

    void forceFailureOfPendingRequests(SharedNodePointer node);
};

#endif