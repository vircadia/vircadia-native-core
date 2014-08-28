//
//  ModelServer.cpp
//  assignment-client/src/models
//
//  Created by Brad Hefta-Gaub on 4/29/14
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QTimer>
#include <ModelTree.h>

#include "ModelServer.h"
#include "ModelServerConsts.h"
#include "ModelNodeData.h"

const char* MODEL_SERVER_NAME = "Model";
const char* MODEL_SERVER_LOGGING_TARGET_NAME = "model-server";
const char* LOCAL_MODELS_PERSIST_FILE = "resources/models.svo";

ModelServer::ModelServer(const QByteArray& packet) : OctreeServer(packet) {
    // nothing special to do here...
}

ModelServer::~ModelServer() {
    ModelTree* tree = (ModelTree*)_tree;
    tree->removeNewlyCreatedHook(this);
}

OctreeQueryNode* ModelServer::createOctreeQueryNode() {
    return new ModelNodeData();
}

Octree* ModelServer::createTree() {
    ModelTree* tree = new ModelTree(true);
    tree->addNewlyCreatedHook(this);
    return tree;
}

void ModelServer::beforeRun() {
    QTimer* pruneDeletedModelsTimer = new QTimer(this);
    connect(pruneDeletedModelsTimer, SIGNAL(timeout()), this, SLOT(pruneDeletedModels()));
    const int PRUNE_DELETED_MODELS_INTERVAL_MSECS = 1 * 1000; // once every second
    pruneDeletedModelsTimer->start(PRUNE_DELETED_MODELS_INTERVAL_MSECS);
}

void ModelServer::modelCreated(const ModelItem& newModel, const SharedNodePointer& senderNode) {
    unsigned char outputBuffer[MAX_PACKET_SIZE];
    unsigned char* copyAt = outputBuffer;

    int numBytesPacketHeader = populatePacketHeader(reinterpret_cast<char*>(outputBuffer), PacketTypeModelAddResponse);
    int packetLength = numBytesPacketHeader;
    copyAt += numBytesPacketHeader;

    // encode the creatorTokenID
    uint32_t creatorTokenID = newModel.getCreatorTokenID();
    memcpy(copyAt, &creatorTokenID, sizeof(creatorTokenID));
    copyAt += sizeof(creatorTokenID);
    packetLength += sizeof(creatorTokenID);

    // encode the model ID
    uint32_t modelID = newModel.getID();
    memcpy(copyAt, &modelID, sizeof(modelID));
    copyAt += sizeof(modelID);
    packetLength += sizeof(modelID);

    NodeList::getInstance()->writeDatagram((char*) outputBuffer, packetLength, senderNode);
}


// ModelServer will use the "special packets" to send list of recently deleted models
bool ModelServer::hasSpecialPacketToSend(const SharedNodePointer& node) {
    bool shouldSendDeletedModels = false;

    // check to see if any new models have been added since we last sent to this node...
    ModelNodeData* nodeData = static_cast<ModelNodeData*>(node->getLinkedData());
    if (nodeData) {
        quint64 deletedModelsSentAt = nodeData->getLastDeletedModelsSentAt();

        ModelTree* tree = static_cast<ModelTree*>(_tree);
        shouldSendDeletedModels = tree->hasModelsDeletedSince(deletedModelsSentAt);
    }

    return shouldSendDeletedModels;
}

int ModelServer::sendSpecialPacket(const SharedNodePointer& node, OctreeQueryNode* queryNode, int& packetsSent) {
    unsigned char outputBuffer[MAX_PACKET_SIZE];
    size_t packetLength = 0;

    ModelNodeData* nodeData = static_cast<ModelNodeData*>(node->getLinkedData());
    if (nodeData) {
        quint64 deletedModelsSentAt = nodeData->getLastDeletedModelsSentAt();
        quint64 deletePacketSentAt = usecTimestampNow();

        ModelTree* tree = static_cast<ModelTree*>(_tree);
        bool hasMoreToSend = true;

        // TODO: is it possible to send too many of these packets? what if you deleted 1,000,000 models?
        packetsSent = 0;
        while (hasMoreToSend) {
            hasMoreToSend = tree->encodeModelsDeletedSince(queryNode->getSequenceNumber(), deletedModelsSentAt,
                                                outputBuffer, MAX_PACKET_SIZE, packetLength);

            //qDebug() << "sending PacketType_MODEL_ERASE packetLength:" << packetLength;

            NodeList::getInstance()->writeDatagram((char*) outputBuffer, packetLength, SharedNodePointer(node));
            queryNode->packetSent(outputBuffer, packetLength);
            packetsSent++;
        }

        nodeData->setLastDeletedModelsSentAt(deletePacketSentAt);
    }

    // TODO: caller is expecting a packetLength, what if we send more than one packet??
    return packetLength;
}

void ModelServer::pruneDeletedModels() {
    ModelTree* tree = static_cast<ModelTree*>(_tree);
    if (tree->hasAnyDeletedModels()) {

        //qDebug() << "there are some deleted models to consider...";
        quint64 earliestLastDeletedModelsSent = usecTimestampNow() + 1; // in the future
        foreach (const SharedNodePointer& otherNode, NodeList::getInstance()->getNodeHash()) {
            if (otherNode->getLinkedData()) {
                ModelNodeData* nodeData = static_cast<ModelNodeData*>(otherNode->getLinkedData());
                quint64 nodeLastDeletedModelsSentAt = nodeData->getLastDeletedModelsSentAt();
                if (nodeLastDeletedModelsSentAt < earliestLastDeletedModelsSent) {
                    earliestLastDeletedModelsSent = nodeLastDeletedModelsSentAt;
                }
            }
        }
        //qDebug() << "earliestLastDeletedModelsSent=" << earliestLastDeletedModelsSent;
        tree->forgetModelsDeletedBefore(earliestLastDeletedModelsSent);
    }
}

