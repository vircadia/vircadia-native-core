//
//  ParticlesScriptingInterface.cpp
//  hifi
//
//  Created by Brad Hefta-Gaub on 12/6/13
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include "ParticlesScriptingInterface.h"



void ParticlesScriptingInterface::queueParticleMessage(PACKET_TYPE packetType, ParticleDetail& particleDetails) {
    getParticlePacketSender()->queueParticleEditMessages(packetType, 1, &particleDetails);
}

unsigned int ParticlesScriptingInterface::queueParticleAdd(glm::vec3 position, float radius, 
            xColor color, glm::vec3 velocity, glm::vec3 gravity, float damping, bool inHand, QString script) {

    // The application will keep track of creatorTokenID
    uint32_t creatorTokenID = _nextCreatorTokenID;
    _nextCreatorTokenID++;
                                                    
    // setup a ParticleDetail struct with the data
    uint64_t now = usecTimestampNow();
    ParticleDetail addParticleDetail = { NEW_PARTICLE, now,
                                        position, radius, {color.red, color.green, color.blue }, velocity, 
                                        gravity, damping, inHand, script, creatorTokenID };
    
    // queue the packet
    queueParticleMessage(PACKET_TYPE_PARTICLE_ADD_OR_EDIT, addParticleDetail);
    
    return creatorTokenID;
}


void ParticlesScriptingInterface::queueParticleEdit(unsigned int particleID, glm::vec3 position, float radius, 
            xColor color, glm::vec3 velocity, glm::vec3 gravity, float damping, bool inHand, QString script) {

    // setup a ParticleDetail struct with the data
    uint64_t now = usecTimestampNow();
    ParticleDetail editParticleDetail = { particleID, now,
                                        position, radius, {color.red, color.green, color.blue }, velocity, 
                                        gravity, damping, inHand, script, UNKNOWN_TOKEN };
    
    // queue the packet
    queueParticleMessage(PACKET_TYPE_PARTICLE_ADD_OR_EDIT, editParticleDetail);
}
