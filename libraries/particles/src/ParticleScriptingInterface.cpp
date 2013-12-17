//
//  ParticleScriptingInterface.cpp
//  hifi
//
//  Created by Brad Hefta-Gaub on 12/6/13
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include "ParticleScriptingInterface.h"



void ParticleScriptingInterface::queueParticleAdd(PACKET_TYPE addPacketType, ParticleDetail& addParticleDetails) {
    getParticlePacketSender()->queueParticleEditMessages(addPacketType, 1, &addParticleDetails);
}

unsigned int ParticleScriptingInterface::queueParticleAdd(glm::vec3 position, float radius, 
            xColor color, glm::vec3 velocity, glm::vec3 gravity, float damping, bool inHand, QString updateScript) {

    // The application will keep track of creatorTokenID
    uint32_t creatorTokenID = _nextCreatorTokenID;
    _nextCreatorTokenID++;
                                                    
    // setup a ParticleDetail struct with the data
    ParticleDetail addParticleDetail = { NEW_PARTICLE, usecTimestampNow(), 
                                        position, radius, {color.red, color.green, color.blue }, velocity, 
                                        gravity, damping, inHand, updateScript, creatorTokenID };
    
    // queue the packet
    queueParticleAdd(PACKET_TYPE_PARTICLE_ADD_OR_EDIT, addParticleDetail);
    
    return creatorTokenID;
}
