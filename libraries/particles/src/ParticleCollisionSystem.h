//
//  ParticleCollisionSystem.h
//  hifi
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//

#ifndef __hifi__ParticleCollisionSystem__
#define __hifi__ParticleCollisionSystem__

#include <glm/glm.hpp>
#include <stdint.h>

#include <QtScript/QScriptEngine>
#include <QtCore/QObject>

#include <SharedUtil.h>
#include <OctreePacketData.h>

#include "Particle.h"

class AbstractAudioInterface;
class ParticleEditPacketSender;
class ParticleTree;
class VoxelTree;

class ParticleCollisionSystem {
public:
    ParticleCollisionSystem(ParticleEditPacketSender* packetSender = NULL, ParticleTree* particles = NULL, 
                                VoxelTree* voxels = NULL, 
                                AbstractAudioInterface* audio = NULL);

    void init(ParticleEditPacketSender* packetSender, ParticleTree* particles, VoxelTree* voxels, 
                                AbstractAudioInterface* audio = NULL);
                                
    ~ParticleCollisionSystem();

    void update();
    void checkParticle(Particle* particle);
    void updateCollisionWithVoxels(Particle* particle);
    void updateCollisionWithParticles(Particle* particle);
    void applyHardCollision(Particle* particle, const glm::vec3& penetration, float elasticity, float damping);
    void updateCollisionSound(Particle* particle, const glm::vec3 &penetration, float frequency);

private:
    static bool updateOperation(OctreeElement* element, void* extraData);


    ParticleEditPacketSender* _packetSender;
    ParticleTree* _particles;
    VoxelTree* _voxels;
    AbstractAudioInterface* _audio;
};

#endif /* defined(__hifi__ParticleCollisionSystem__) */