//
//  ParticleEditHandle.cpp
//  hifi
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//

#include "Particle.h"
#include "ParticleEditHandle.h"
#include "ParticleEditPacketSender.h"
#include "ParticleTree.h"

std::map<uint32_t,ParticleEditHandle*> ParticleEditHandle::_allHandles;
uint32_t ParticleEditHandle::_nextCreatorTokenID = 0;

ParticleEditHandle::ParticleEditHandle(ParticleEditPacketSender* packetSender, ParticleTree* localTree, uint32_t id) {
    if (id == NEW_PARTICLE) {
        _creatorTokenID = _nextCreatorTokenID;
        _nextCreatorTokenID++;
        _id = NEW_PARTICLE;
        _isKnownID = false;
        _allHandles[_creatorTokenID] = this;
    } else {
        _creatorTokenID = UNKNOWN_TOKEN; 
        _id = id;
        _isKnownID = true;
        // don't add to _allHandles because we already know it...
    }
    _packetSender = packetSender;
    _localTree = localTree;
    
}

ParticleEditHandle::~ParticleEditHandle() {
    // remove us from our _allHandles map
    if (_creatorTokenID != UNKNOWN_TOKEN) {
        _allHandles.erase(_allHandles.find(_creatorTokenID));
    }
}

void ParticleEditHandle::createParticle(glm::vec3 position, float radius, xColor color, glm::vec3 velocity, 
                           glm::vec3 gravity, float damping, QString updateScript) {

    // setup a ParticleDetail struct with the data
    ParticleDetail addParticleDetail = { NEW_PARTICLE, usecTimestampNow(), 
            position, radius, {color.red, color.green, color.blue }, 
            velocity, gravity, damping, updateScript, _creatorTokenID };
    
    // queue the packet
    _packetSender->queueParticleEditMessages(PACKET_TYPE_PARTICLE_ADD_OR_EDIT, 1, &addParticleDetail);
    
    // release them
    _packetSender->releaseQueuedMessages();
    
    // if we have a local tree, also update it...
    if (_localTree) {
        // we can't really do this here, because if we create a particle locally, we'll get a ghost particle
        // because we can't really handle updating/deleting it locally
    }
}

bool ParticleEditHandle::updateParticle(glm::vec3 position, float radius, xColor color, glm::vec3 velocity, 
                           glm::vec3 gravity, float damping, QString updateScript) {

    if (!isKnownID()) {
        return false; // not allowed until we know the id
    }
    
    // setup a ParticleDetail struct with the data
    ParticleDetail newParticleDetail = { _id, usecTimestampNow(), 
            position, radius, {color.red, color.green, color.blue }, 
            velocity, gravity, damping, updateScript, _creatorTokenID };

    // queue the packet
    _packetSender->queueParticleEditMessages(PACKET_TYPE_PARTICLE_ADD_OR_EDIT, 1, &newParticleDetail);
    
    // release them
    _packetSender->releaseQueuedMessages();

    // if we have a local tree, also update it...
    if (_localTree) {
        rgbColor rcolor = {color.red, color.green, color.blue };
        Particle tempParticle(position, radius, rcolor, velocity, damping, gravity, updateScript, _id);
        _localTree->storeParticle(tempParticle);
    }
    
    return true;
}

void ParticleEditHandle::handleAddResponse(unsigned char* packetData , int packetLength) {
    unsigned char* dataAt = packetData;
    int numBytesPacketHeader = numBytesForPacketHeader(packetData);
    dataAt += numBytesPacketHeader;
    
    uint32_t creatorTokenID;
    memcpy(&creatorTokenID, dataAt, sizeof(creatorTokenID));
    dataAt += sizeof(creatorTokenID);

    uint32_t particleID;
    memcpy(&particleID, dataAt, sizeof(particleID));
    dataAt += sizeof(particleID);

    // find this particle in the _allHandles map
    if (_allHandles.find(creatorTokenID) != _allHandles.end()) {
        ParticleEditHandle* theHandle = _allHandles[creatorTokenID];
        theHandle->_id = particleID;
        theHandle->_isKnownID = true;
    }
}
