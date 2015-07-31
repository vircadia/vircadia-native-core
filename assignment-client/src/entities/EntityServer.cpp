//
//  EntityServer.cpp
//  assignment-client/src/entities
//
//  Created by Brad Hefta-Gaub on 4/29/14
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QTimer>
#include <EntityTree.h>
#include <SimpleEntitySimulation.h>

#include "EntityServer.h"
#include "EntityServerConsts.h"
#include "EntityNodeData.h"

const char* MODEL_SERVER_NAME = "Entity";
const char* MODEL_SERVER_LOGGING_TARGET_NAME = "entity-server";
const char* LOCAL_MODELS_PERSIST_FILE = "resources/models.svo";

EntityServer::EntityServer(NLPacket& packet) :
    OctreeServer(packet),
    _entitySimulation(NULL)
{
    auto& packetReceiver = DependencyManager::get<NodeList>()->getPacketReceiver();
    packetReceiver.registerListenerForTypes({ PacketType::EntityAdd, PacketType::EntityEdit, PacketType::EntityErase },
                                            this, "handleEntityPacket");
}

EntityServer::~EntityServer() {
    if (_pruneDeletedEntitiesTimer) {
        _pruneDeletedEntitiesTimer->stop();
        _pruneDeletedEntitiesTimer->deleteLater();
    }

    EntityTree* tree = (EntityTree*)_tree;
    tree->removeNewlyCreatedHook(this);
}

void EntityServer::handleEntityPacket(QSharedPointer<NLPacket> packet, SharedNodePointer senderNode) {
    if (_octreeInboundPacketProcessor) {
        _octreeInboundPacketProcessor->queueReceivedPacket(packet, senderNode);
    }
}

OctreeQueryNode* EntityServer::createOctreeQueryNode() {
    return new EntityNodeData();
}

Octree* EntityServer::createTree() {
    EntityTree* tree = new EntityTree(true);
    tree->addNewlyCreatedHook(this);
    if (!_entitySimulation) {
        SimpleEntitySimulation* simpleSimulation = new SimpleEntitySimulation();
        simpleSimulation->setEntityTree(tree);
        tree->setSimulation(simpleSimulation);
        _entitySimulation = simpleSimulation;
    }
    return tree;
}

void EntityServer::beforeRun() {
    _pruneDeletedEntitiesTimer = new QTimer();
    connect(_pruneDeletedEntitiesTimer, SIGNAL(timeout()), this, SLOT(pruneDeletedEntities()));
    const int PRUNE_DELETED_MODELS_INTERVAL_MSECS = 1 * 1000; // once every second
    _pruneDeletedEntitiesTimer->start(PRUNE_DELETED_MODELS_INTERVAL_MSECS);
}

void EntityServer::entityCreated(const EntityItem& newEntity, const SharedNodePointer& senderNode) {
}


// EntityServer will use the "special packets" to send list of recently deleted entities
bool EntityServer::hasSpecialPacketsToSend(const SharedNodePointer& node) {
    bool shouldSendDeletedEntities = false;

    // check to see if any new entities have been added since we last sent to this node...
    EntityNodeData* nodeData = static_cast<EntityNodeData*>(node->getLinkedData());
    if (nodeData) {
        quint64 deletedEntitiesSentAt = nodeData->getLastDeletedEntitiesSentAt();

        EntityTree* tree = static_cast<EntityTree*>(_tree);
        shouldSendDeletedEntities = tree->hasEntitiesDeletedSince(deletedEntitiesSentAt);
    }

    return shouldSendDeletedEntities;
}

int EntityServer::sendSpecialPackets(const SharedNodePointer& node, OctreeQueryNode* queryNode, int& packetsSent) {
    int totalBytes = 0;

    EntityNodeData* nodeData = static_cast<EntityNodeData*>(node->getLinkedData());
    if (nodeData) {
        quint64 deletedEntitiesSentAt = nodeData->getLastDeletedEntitiesSentAt();
        quint64 deletePacketSentAt = usecTimestampNow();

        EntityTree* tree = static_cast<EntityTree*>(_tree);
        bool hasMoreToSend = true;

        packetsSent = 0;

        while (hasMoreToSend) {
            auto specialPacket = tree->encodeEntitiesDeletedSince(queryNode->getSequenceNumber(), deletedEntitiesSentAt,
                                                                  hasMoreToSend);

            queryNode->packetSent(*specialPacket);

            totalBytes += specialPacket->getDataSize();
            packetsSent++;

            DependencyManager::get<NodeList>()->sendPacket(std::move(specialPacket), *node);
        }

        nodeData->setLastDeletedEntitiesSentAt(deletePacketSentAt);
    }

    // TODO: caller is expecting a packetLength, what if we send more than one packet??
    return totalBytes;
}

void EntityServer::pruneDeletedEntities() {
    EntityTree* tree = static_cast<EntityTree*>(_tree);
    if (tree->hasAnyDeletedEntities()) {

        quint64 earliestLastDeletedEntitiesSent = usecTimestampNow() + 1; // in the future

        DependencyManager::get<NodeList>()->eachNode([&earliestLastDeletedEntitiesSent](const SharedNodePointer& node) {
            if (node->getLinkedData()) {
                EntityNodeData* nodeData = static_cast<EntityNodeData*>(node->getLinkedData());
                quint64 nodeLastDeletedEntitiesSentAt = nodeData->getLastDeletedEntitiesSentAt();
                if (nodeLastDeletedEntitiesSentAt < earliestLastDeletedEntitiesSent) {
                    earliestLastDeletedEntitiesSent = nodeLastDeletedEntitiesSentAt;
                }
            }
        });

        tree->forgetEntitiesDeletedBefore(earliestLastDeletedEntitiesSent);
    }
}

void EntityServer::readAdditionalConfiguration(const QJsonObject& settingsSectionObject) {
    bool wantEditLogging = false;
    readOptionBool(QString("wantEditLogging"), settingsSectionObject, wantEditLogging);
    qDebug("wantEditLogging=%s", debug::valueOf(wantEditLogging));


    EntityTree* tree = static_cast<EntityTree*>(_tree);
    tree->setWantEditLogging(wantEditLogging);
}


