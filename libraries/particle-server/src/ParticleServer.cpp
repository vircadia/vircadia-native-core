//
//  ParticleServer.cpp
//  hifi
//
//  Created by Brad Hefta-Gaub on 12/4/13
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include <ParticleTree.h>

#include "ParticleServer.h"
#include "ParticleServerConsts.h"
#include "ParticleNodeData.h"

const char* PARTICLE_SERVER_NAME = "Particle";
const char* PARTICLE_SERVER_LOGGING_TARGET_NAME = "particle-server";
const char* LOCAL_PARTICLES_PERSIST_FILE = "resources/particles.svo";

ParticleServer::ParticleServer(const unsigned char* dataBuffer, int numBytes) : OctreeServer(dataBuffer, numBytes) {
    // nothing special to do here...
}

ParticleServer::~ParticleServer() {
    ParticleTree* tree = (ParticleTree*)_tree;
    tree->removeNewlyCreatedHook(this);
}

OctreeQueryNode* ParticleServer::createOctreeQueryNode(Node* newNode) {
    return new ParticleNodeData(newNode);
}

Octree* ParticleServer::createTree() {
    ParticleTree* tree = new ParticleTree(true);
    tree->addNewlyCreatedHook(this);
    return tree;
}

void ParticleServer::beforeRun() {
    // nothing special to do...
}

void ParticleServer::particleCreated(const Particle& newParticle, Node* node) {
    unsigned char outputBuffer[MAX_PACKET_SIZE];
    unsigned char* copyAt = outputBuffer;

    int numBytesPacketHeader = populateTypeAndVersion(outputBuffer, PACKET_TYPE_PARTICLE_ADD_RESPONSE);
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
    
    NodeList::getInstance()->getNodeSocket().writeDatagram((char*) outputBuffer, packetLength,
                                                           node->getActiveSocket()->getAddress(),
                                                           node->getActiveSocket()->getPort());
}
