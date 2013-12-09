//
//  ParticleScriptingInterface.cpp
//  hifi
//
//  Created by Brad Hefta-Gaub on 12/6/13
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include "ParticleScriptingInterface.h"

ParticleScriptingInterface::ParticleScriptingInterface() :
    _jurisdictionListener(NODE_TYPE_PARTICLE_SERVER)
{
    _jurisdictionListener.initialize(true);
    _particlePacketSender.setServerJurisdictions(_jurisdictionListener.getJurisdictions());
}

void ParticleScriptingInterface::queueParticleAdd(PACKET_TYPE addPacketType, ParticleDetail& addParticleDetails) {
    _particlePacketSender.queueParticleEditMessages(addPacketType, 1, &addParticleDetails);
}

void ParticleScriptingInterface::queueParticleAdd(float x, float y, float z, float scale, uchar red, uchar green, uchar blue, 
                                                    float vx, float vy, float vz) {
                                                    
    // setup a ParticleDetail struct with the data
    ParticleDetail addParticleDetail = { glm::vec3(x, y, z), scale, {red, green, blue} , glm::vec3(vx, vy, vz) };
    
    // queue the packet
    queueParticleAdd(PACKET_TYPE_PARTICLE_ADD, addParticleDetail);
}

void ParticleScriptingInterface::queueParticleDelete(float x, float y, float z, float scale) {
    // not yet supported
}

