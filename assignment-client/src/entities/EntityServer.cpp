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

#include "EntityServer.h"
#include "EntityServerConsts.h"
#include "EntityNodeData.h"

const char* MODEL_SERVER_NAME = "Entity";
const char* MODEL_SERVER_LOGGING_TARGET_NAME = "entity-server";
const char* LOCAL_MODELS_PERSIST_FILE = "resources/models.svo";

EntityServer::EntityServer(const QByteArray& packet) : OctreeServer(packet) {
    // nothing special to do here...
}

EntityServer::~EntityServer() {
    EntityTree* tree = (EntityTree*)_tree;
    tree->removeNewlyCreatedHook(this);
}

OctreeQueryNode* EntityServer::createOctreeQueryNode() {
    return new EntityNodeData();
}

Octree* EntityServer::createTree() {
    EntityTree* tree = new EntityTree(true);
    tree->addNewlyCreatedHook(this);
    return tree;
}

void EntityServer::beforeRun() {
    QTimer* pruneDeletedEntitiesTimer = new QTimer(this);
    connect(pruneDeletedEntitiesTimer, SIGNAL(timeout()), this, SLOT(pruneDeletedEntities()));
    const int PRUNE_DELETED_MODELS_INTERVAL_MSECS = 1 * 1000; // once every second
    pruneDeletedEntitiesTimer->start(PRUNE_DELETED_MODELS_INTERVAL_MSECS);
}

void EntityServer::entityCreated(const EntityItem& newEntity, const SharedNodePointer& senderNode) {

    unsigned char outputBuffer[MAX_PACKET_SIZE];
    unsigned char* copyAt = outputBuffer;

    int numBytesPacketHeader = populatePacketHeader(reinterpret_cast<char*>(outputBuffer), PacketTypeEntityAddResponse);
    int packetLength = numBytesPacketHeader;
    copyAt += numBytesPacketHeader;

    // encode the creatorTokenID
    uint32_t creatorTokenID = newEntity.getCreatorTokenID();
    memcpy(copyAt, &creatorTokenID, sizeof(creatorTokenID));
    copyAt += sizeof(creatorTokenID);
    packetLength += sizeof(creatorTokenID);

    // encode the entity ID
    QUuid entityID = newEntity.getID();
    QByteArray encodedID = entityID.toRfc4122();
    memcpy(copyAt, encodedID.constData(), encodedID.size());
    copyAt += sizeof(entityID);
    packetLength += sizeof(entityID);

    NodeList::getInstance()->writeDatagram((char*) outputBuffer, packetLength, senderNode);
}


// EntityServer will use the "special packets" to send list of recently deleted entities
bool EntityServer::hasSpecialPacketToSend(const SharedNodePointer& node) {
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

int EntityServer::sendSpecialPacket(const SharedNodePointer& node, OctreeQueryNode* queryNode, int& packetsSent) {
    unsigned char outputBuffer[MAX_PACKET_SIZE];
    size_t packetLength = 0;

    EntityNodeData* nodeData = static_cast<EntityNodeData*>(node->getLinkedData());
    if (nodeData) {
        quint64 deletedEntitiesSentAt = nodeData->getLastDeletedEntitiesSentAt();
        quint64 deletePacketSentAt = usecTimestampNow();

        EntityTree* tree = static_cast<EntityTree*>(_tree);
        bool hasMoreToSend = true;

        // TODO: is it possible to send too many of these packets? what if you deleted 1,000,000 entities?
        packetsSent = 0;
        while (hasMoreToSend) {
            hasMoreToSend = tree->encodeEntitiesDeletedSince(queryNode->getSequenceNumber(), deletedEntitiesSentAt,
                                                outputBuffer, MAX_PACKET_SIZE, packetLength);

            NodeList::getInstance()->writeDatagram((char*) outputBuffer, packetLength, SharedNodePointer(node));
            queryNode->packetSent(outputBuffer, packetLength);
            packetsSent++;
        }

        nodeData->setLastDeletedEntitiesSentAt(deletePacketSentAt);
    }

    // TODO: caller is expecting a packetLength, what if we send more than one packet??
    return packetLength;
}

void EntityServer::pruneDeletedEntities() {
    EntityTree* tree = static_cast<EntityTree*>(_tree);
    if (tree->hasAnyDeletedEntities()) {

        quint64 earliestLastDeletedEntitiesSent = usecTimestampNow() + 1; // in the future
        
        NodeHashSnapshot snapshotHash =  NodeList::getInstance()->getNodeHash().snapshot_table();
        
        for (auto it = snapshotHash.begin(); it != snapshotHash.end(); it++) {
            SharedNodePointer node = it->second;
            
            if (node->getLinkedData()) {
                EntityNodeData* nodeData = static_cast<EntityNodeData*>(node->getLinkedData());
                quint64 nodeLastDeletedEntitiesSentAt = nodeData->getLastDeletedEntitiesSentAt();
                if (nodeLastDeletedEntitiesSentAt < earliestLastDeletedEntitiesSent) {
                    earliestLastDeletedEntitiesSent = nodeLastDeletedEntitiesSentAt;
                }
            }
        }
        tree->forgetEntitiesDeletedBefore(earliestLastDeletedEntitiesSent);
    }
}

