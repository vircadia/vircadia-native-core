//
//  ParticleServer.cpp
//  assignment-client/src/particles
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QTimer>
#include <ParticleTree.h>

#include "ParticleServer.h"
#include "ParticleServerConsts.h"
#include "ParticleNodeData.h"

const char* PARTICLE_SERVER_NAME = "Particle";
const char* PARTICLE_SERVER_LOGGING_TARGET_NAME = "particle-server";
const char* LOCAL_PARTICLES_PERSIST_FILE = "resources/particles.svo";

ParticleServer::ParticleServer(const QByteArray& packet) : OctreeServer(packet) {
    // nothing special to do here...
}

ParticleServer::~ParticleServer() {
    ParticleTree* tree = (ParticleTree*)_tree;
    tree->removeNewlyCreatedHook(this);
}

OctreeQueryNode* ParticleServer::createOctreeQueryNode() {
    return new ParticleNodeData();
}

Octree* ParticleServer::createTree() {
    ParticleTree* tree = new ParticleTree(true);
    tree->addNewlyCreatedHook(this);
    return tree;
}

void ParticleServer::beforeRun() {
    QTimer* pruneDeletedParticlesTimer = new QTimer(this);
    connect(pruneDeletedParticlesTimer, SIGNAL(timeout()), this, SLOT(pruneDeletedParticles()));
    const int PRUNE_DELETED_PARTICLES_INTERVAL_MSECS = 1 * 1000; // once every second
    pruneDeletedParticlesTimer->start(PRUNE_DELETED_PARTICLES_INTERVAL_MSECS);
}

void ParticleServer::particleCreated(const Particle& newParticle, const SharedNodePointer& senderNode) {
    unsigned char outputBuffer[MAX_PACKET_SIZE];
    unsigned char* copyAt = outputBuffer;

    int numBytesPacketHeader = populatePacketHeader(reinterpret_cast<char*>(outputBuffer), PacketTypeParticleAddResponse);
    int packetLength = numBytesPacketHeader;
    copyAt += numBytesPacketHeader;

    // encode the creatorTokenID
    uint32_t creatorTokenID = newParticle.getCreatorTokenID();
    memcpy(copyAt, &creatorTokenID, sizeof(creatorTokenID));
    copyAt += sizeof(creatorTokenID);
    packetLength += sizeof(creatorTokenID);

    // encode the particle ID
    uint32_t particleID = newParticle.getID();
    memcpy(copyAt, &particleID, sizeof(particleID));
    copyAt += sizeof(particleID);
    packetLength += sizeof(particleID);

    NodeList::getInstance()->writeDatagram((char*) outputBuffer, packetLength, senderNode);
}


// ParticleServer will use the "special packets" to send list of recently deleted particles
bool ParticleServer::hasSpecialPacketToSend(const SharedNodePointer& node) {
    bool shouldSendDeletedParticles = false;

    // check to see if any new particles have been added since we last sent to this node...
    ParticleNodeData* nodeData = static_cast<ParticleNodeData*>(node->getLinkedData());
    if (nodeData) {
        quint64 deletedParticlesSentAt = nodeData->getLastDeletedParticlesSentAt();

        ParticleTree* tree = static_cast<ParticleTree*>(_tree);
        shouldSendDeletedParticles = tree->hasParticlesDeletedSince(deletedParticlesSentAt);
    }

    return shouldSendDeletedParticles;
}

int ParticleServer::sendSpecialPacket(const SharedNodePointer& node, OctreeQueryNode* queryNode, int& packetsSent) {
    unsigned char outputBuffer[MAX_PACKET_SIZE];
    size_t packetLength = 0;

    ParticleNodeData* nodeData = static_cast<ParticleNodeData*>(node->getLinkedData());
    if (nodeData) {
        quint64 deletedParticlesSentAt = nodeData->getLastDeletedParticlesSentAt();
        quint64 deletePacketSentAt = usecTimestampNow();

        ParticleTree* tree = static_cast<ParticleTree*>(_tree);
        bool hasMoreToSend = true;

        // TODO: is it possible to send too many of these packets? what if you deleted 1,000,000 particles?
        packetsSent = 0;
        while (hasMoreToSend) {
            hasMoreToSend = tree->encodeParticlesDeletedSince(queryNode->getSequenceNumber(), deletedParticlesSentAt,
                                                outputBuffer, MAX_PACKET_SIZE, packetLength);

            //qDebug() << "sending PacketType_PARTICLE_ERASE packetLength:" << packetLength;

            NodeList::getInstance()->writeDatagram((char*) outputBuffer, packetLength, SharedNodePointer(node));
            queryNode->packetSent(outputBuffer, packetLength);
            packetsSent++;
        }

        nodeData->setLastDeletedParticlesSentAt(deletePacketSentAt);
    }

    // TODO: caller is expecting a packetLength, what if we send more than one packet??
    return packetLength;
}

void ParticleServer::pruneDeletedParticles() {
    ParticleTree* tree = static_cast<ParticleTree*>(_tree);
    if (tree->hasAnyDeletedParticles()) {

        //qDebug() << "there are some deleted particles to consider...";
        quint64 earliestLastDeletedParticlesSent = usecTimestampNow() + 1; // in the future
        foreach (const SharedNodePointer& otherNode, NodeList::getInstance()->getNodeHash()) {
            if (otherNode->getLinkedData()) {
                ParticleNodeData* nodeData = static_cast<ParticleNodeData*>(otherNode->getLinkedData());
                quint64 nodeLastDeletedParticlesSentAt = nodeData->getLastDeletedParticlesSentAt();
                if (nodeLastDeletedParticlesSentAt < earliestLastDeletedParticlesSent) {
                    earliestLastDeletedParticlesSent = nodeLastDeletedParticlesSentAt;
                }
            }
        }
        //qDebug() << "earliestLastDeletedParticlesSent=" << earliestLastDeletedParticlesSent;
        tree->forgetParticlesDeletedBefore(earliestLastDeletedParticlesSent);
    }
}

