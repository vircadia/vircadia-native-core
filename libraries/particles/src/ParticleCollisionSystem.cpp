//
//  ParticleCollisionSystem.cpp
//  hifi
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//

#include <AbstractAudioInterface.h>
#include <VoxelTree.h>

#include "Particle.h"
#include "ParticleCollisionSystem.h"
#include "ParticleEditHandle.h"
#include "ParticleEditPacketSender.h"
#include "ParticleTree.h"

ParticleCollisionSystem::ParticleCollisionSystem(ParticleEditPacketSender* packetSender, 
    ParticleTree* particles, VoxelTree* voxels, AbstractAudioInterface* audio) {
    init(packetSender, particles, voxels, audio);
}

void ParticleCollisionSystem::init(ParticleEditPacketSender* packetSender, 
    ParticleTree* particles, VoxelTree* voxels, AbstractAudioInterface* audio) {
    _packetSender = packetSender;
    _particles = particles;
    _voxels = voxels;
    _audio = audio;
}

ParticleCollisionSystem::~ParticleCollisionSystem() {
}

bool ParticleCollisionSystem::updateOperation(OctreeElement* element, void* extraData) {
    ParticleCollisionSystem* system = static_cast<ParticleCollisionSystem*>(extraData);
    ParticleTreeElement* particleTreeElement = static_cast<ParticleTreeElement*>(element);

    // iterate the particles...
    std::vector<Particle>& particles = particleTreeElement->getParticles();
    uint16_t numberOfParticles = particles.size();
    for (uint16_t i = 0; i < numberOfParticles; i++) {
        Particle* particle = &particles[i];
        system->checkParticle(particle);
    }

    return true;
}


void ParticleCollisionSystem::update() {
    // update all particles
    _particles->lockForWrite();
    _particles->recurseTreeWithOperation(updateOperation, this);
    _particles->unlock();
}


void ParticleCollisionSystem::checkParticle(Particle* particle) {
    updateCollisionWithVoxels(particle);
    updateCollisionWithParticles(particle);
}

void ParticleCollisionSystem::updateCollisionWithVoxels(Particle* particle) {
    glm::vec3 center = particle->getPosition() * (float)TREE_SCALE;
    float radius = particle->getRadius() * (float)TREE_SCALE;
    const float VOXEL_ELASTICITY = 1.4f;
    const float VOXEL_DAMPING = 0.0;
    const float VOXEL_COLLISION_FREQUENCY = 0.5f;
    glm::vec3 penetration;
    OctreeElement* penetratedVoxel;
    if (_voxels->findSpherePenetration(center, radius, penetration, &penetratedVoxel)) {
        penetration /= (float)TREE_SCALE;
        updateCollisionSound(particle, penetration, VOXEL_COLLISION_FREQUENCY);
        applyHardCollision(particle, penetration, VOXEL_ELASTICITY, VOXEL_DAMPING);
    }
}

void ParticleCollisionSystem::updateCollisionWithParticles(Particle* particle) {
    glm::vec3 center = particle->getPosition() * (float)TREE_SCALE;
    float radius = particle->getRadius() * (float)TREE_SCALE;
    const float VOXEL_ELASTICITY = 1.4f;
    const float VOXEL_DAMPING = 0.0;
    const float VOXEL_COLLISION_FREQUENCY = 0.5f;
    glm::vec3 penetration;
    OctreeElement* penetratedElement;
    if (_particles->findSpherePenetration(center, radius, penetration, &penetratedElement)) {
        penetration /= (float)TREE_SCALE;
        updateCollisionSound(particle, penetration, VOXEL_COLLISION_FREQUENCY);
        applyHardCollision(particle, penetration, VOXEL_ELASTICITY, VOXEL_DAMPING);
    }
}

void ParticleCollisionSystem::applyHardCollision(Particle* particle, const glm::vec3& penetration, 
                                float elasticity, float damping) {
    //
    //  Update the avatar in response to a hard collision.  Position will be reset exactly
    //  to outside the colliding surface.  Velocity will be modified according to elasticity.
    //
    //  if elasticity = 1.0, collision is inelastic.
    //  if elasticity > 1.0, collision is elastic. 
    //  
    glm::vec3 position = particle->getPosition();
    glm::vec3 velocity = particle->getVelocity();

    position -= penetration;
    static float HALTING_VELOCITY = 0.2f / (float) TREE_SCALE;
    // cancel out the velocity component in the direction of penetration
    float penetrationLength = glm::length(penetration);
    const float EPSILON = 0.0f;

    if (penetrationLength > EPSILON) {
        glm::vec3 direction = penetration / penetrationLength;
        velocity -= glm::dot(velocity, direction) * direction * elasticity;
        velocity *= glm::clamp(1.f - damping, 0.0f, 1.0f);
        if (glm::length(velocity) < HALTING_VELOCITY) {
            // If moving really slowly after a collision, and not applying forces, stop altogether
            velocity *= 0.f;
        }
    }
    ParticleEditHandle particleEditHandle(_packetSender, _particles, particle->getID());
    particleEditHandle.updateParticle(position, particle->getRadius(), particle->getColor(), velocity, 
                           particle->getGravity(), particle->getDamping(), particle->getUpdateScript());
}


void ParticleCollisionSystem::updateCollisionSound(Particle* particle, const glm::vec3 &penetration, float frequency) {
                                
    //  consider whether to have the collision make a sound
    const float AUDIBLE_COLLISION_THRESHOLD = 0.02f;
    const float COLLISION_LOUDNESS = 1.f;
    const float DURATION_SCALING = 0.004f;
    const float NOISE_SCALING = 0.1f;
    glm::vec3 velocity = particle->getVelocity() * (float)TREE_SCALE;

    /*
    // how do we want to handle this??
    //
     glm::vec3 gravity = particle->getGravity() * (float)TREE_SCALE;
    
    if (glm::length(gravity) > EPSILON) {
        //  If gravity is on, remove the effect of gravity on velocity for this
        //  frame, so that we are not constantly colliding with the surface 
        velocity -= _scale * glm::length(gravity) * GRAVITY_EARTH * deltaTime * glm::normalize(gravity);
    }
    */
    float velocityTowardCollision = glm::dot(velocity, glm::normalize(penetration));
    float velocityTangentToCollision = glm::length(velocity) - velocityTowardCollision;
    
    if (velocityTowardCollision > AUDIBLE_COLLISION_THRESHOLD) {
        //  Volume is proportional to collision velocity
        //  Base frequency is modified upward by the angle of the collision
        //  Noise is a function of the angle of collision
        //  Duration of the sound is a function of both base frequency and velocity of impact
        _audio->startCollisionSound(
                    fmin(COLLISION_LOUDNESS * velocityTowardCollision, 1.f),
                    frequency * (1.f + velocityTangentToCollision / velocityTowardCollision),
                    fmin(velocityTangentToCollision / velocityTowardCollision * NOISE_SCALING, 1.f),
                    1.f - DURATION_SCALING * powf(frequency, 0.5f) / velocityTowardCollision, true);
    }
}