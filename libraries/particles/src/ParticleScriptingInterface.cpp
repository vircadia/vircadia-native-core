//
//  ParticleScriptingInterface.cpp
//  hifi
//
//  Created by Brad Hefta-Gaub on 12/6/13
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include "ParticleScriptingInterface.h"

ParticleScriptingInterface::ParticleScriptingInterface(ParticleEditPacketSender* particleSender, 
                JurisdictionListener* particleJurisdictionListener) :
    _nextCreatorTokenID(0)
{
    setParticlePacketSender(particleSender);
    setJurisdictionListener(particleJurisdictionListener);
}

ParticleScriptingInterface::~ParticleScriptingInterface() {
    if (_managedJuridiciontListerner) {
        delete _jurisdictionListener;
    }
    if (_managedPacketSender) {
        delete _particlePacketSender;
    }
}


void ParticleScriptingInterface::setParticlePacketSender(ParticleEditPacketSender* particlePacketSender) {
    _particlePacketSender = particlePacketSender;
}

void ParticleScriptingInterface::setJurisdictionListener(JurisdictionListener* jurisdictionListener) {
    _jurisdictionListener = jurisdictionListener;
}

void ParticleScriptingInterface::init() {
    if (_jurisdictionListener) {
        _managedJuridiciontListerner = false;
    } else {
        _managedJuridiciontListerner = true;
        _jurisdictionListener = new JurisdictionListener(NODE_TYPE_PARTICLE_SERVER);
        _jurisdictionListener->initialize(true);
    }
    
    if (_particlePacketSender) {
        _managedPacketSender = false;
    } else {
        _managedPacketSender = true;
        _particlePacketSender = new ParticleEditPacketSender();
        _particlePacketSender->setServerJurisdictions(_jurisdictionListener->getJurisdictions());
    }
}


void ParticleScriptingInterface::queueParticleAdd(PACKET_TYPE addPacketType, ParticleDetail& addParticleDetails) {
    _particlePacketSender->queueParticleEditMessages(addPacketType, 1, &addParticleDetails);
}

unsigned int ParticleScriptingInterface::queueParticleAdd(glm::vec3 position, float radius, 
            xColor color, glm::vec3 velocity, glm::vec3 gravity, float damping, QString updateScript) {

    // The application will keep track of creatorTokenID
    uint32_t creatorTokenID = _nextCreatorTokenID;
    _nextCreatorTokenID++;
                                                    
    // setup a ParticleDetail struct with the data
    ParticleDetail addParticleDetail = { NEW_PARTICLE, usecTimestampNow(), 
                                        position, radius, {color.red, color.green, color.blue }, velocity, 
                                        gravity, damping, updateScript, creatorTokenID };
    
    // queue the packet
    queueParticleAdd(PACKET_TYPE_PARTICLE_ADD_OR_EDIT, addParticleDetail);
    
    return creatorTokenID;
}
