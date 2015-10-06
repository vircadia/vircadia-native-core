//
//  EntityServer.h
//  assignment-client/src/entities
//
//  Created by Brad Hefta-Gaub on 4/29/14
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_EntityServer_h
#define hifi_EntityServer_h

#include "../octree/OctreeServer.h"

#include "EntityItem.h"
#include "EntityServerConsts.h"
#include "EntityTree.h"

/// Handles assignments of type EntityServer - sending entities to various clients.
class EntityServer : public OctreeServer, public NewlyCreatedEntityHook {
    Q_OBJECT
public:
    EntityServer(NLPacket& packet);
    ~EntityServer();

    // Subclasses must implement these methods
    virtual OctreeQueryNode* createOctreeQueryNode() override ;
    virtual char getMyNodeType() const override { return NodeType::EntityServer; }
    virtual PacketType getMyQueryMessageType() const override { return PacketType::EntityQuery; }
    virtual const char* getMyServerName() const override { return MODEL_SERVER_NAME; }
    virtual const char* getMyLoggingServerTargetName() const override { return MODEL_SERVER_LOGGING_TARGET_NAME; }
    virtual const char* getMyDefaultPersistFilename() const override { return LOCAL_MODELS_PERSIST_FILE; }
    virtual PacketType getMyEditNackType() const override { return PacketType::EntityEditNack; }
    virtual QString getMyDomainSettingsKey() const override { return QString("entity_server_settings"); }

    // subclass may implement these method
    virtual void beforeRun() override;
    virtual bool hasSpecialPacketsToSend(const SharedNodePointer& node) override;
    virtual int sendSpecialPackets(const SharedNodePointer& node, OctreeQueryNode* queryNode, int& packetsSent) override;

    virtual void entityCreated(const EntityItem& newEntity, const SharedNodePointer& senderNode) override;
    virtual bool readAdditionalConfiguration(const QJsonObject& settingsSectionObject) override;

public slots:
    void pruneDeletedEntities();

protected:
    virtual OctreePointer createTree() override;

private slots:
    void handleEntityPacket(QSharedPointer<NLPacket> packet, SharedNodePointer senderNode);

private:
    EntitySimulation* _entitySimulation;
    QTimer* _pruneDeletedEntitiesTimer = nullptr;
};

#endif // hifi_EntityServer_h
