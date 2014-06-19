//
//  ModelServer.h
//  assignment-client/src/models
//
//  Created by Brad Hefta-Gaub on 4/29/14
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ModelServer_h
#define hifi_ModelServer_h

#include "../octree/OctreeServer.h"

#include "ModelItem.h"
#include "ModelServerConsts.h"
#include "ModelTree.h"

/// Handles assignments of type ModelServer - sending models to various clients.
class ModelServer : public OctreeServer, public NewlyCreatedModelHook {
    Q_OBJECT
public:
    ModelServer(const QByteArray& packet);
    ~ModelServer();

    // Subclasses must implement these methods
    virtual OctreeQueryNode* createOctreeQueryNode();
    virtual Octree* createTree();
    virtual char getMyNodeType() const { return NodeType::ModelServer; }
    virtual PacketType getMyQueryMessageType() const { return PacketTypeModelQuery; }
    virtual const char* getMyServerName() const { return MODEL_SERVER_NAME; }
    virtual const char* getMyLoggingServerTargetName() const { return MODEL_SERVER_LOGGING_TARGET_NAME; }
    virtual const char* getMyDefaultPersistFilename() const { return LOCAL_MODELS_PERSIST_FILE; }
    virtual PacketType getMyEditNackType() const { return PacketTypeModelEditNack; }

    // subclass may implement these method
    virtual void beforeRun();
    virtual bool hasSpecialPacketToSend(const SharedNodePointer& node);
    virtual int sendSpecialPacket(const SharedNodePointer& node, OctreeQueryNode* queryNode, int& packetsSent);

    virtual void modelCreated(const ModelItem& newModel, const SharedNodePointer& senderNode);

public slots:
    void pruneDeletedModels();

private:
};

#endif // hifi_ModelServer_h
