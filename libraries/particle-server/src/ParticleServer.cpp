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

void ParticleServer::particleCreated(const Particle& newParticle, Node* senderNode) {
    printf("ParticleServer::particleCreated(newParticle.creatorTokenID=%u, senderNode)\n",newParticle.getCreatorTokenID());
}
