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
#include <AvatarData.h>
#include <HeadData.h>
#include <HandData.h>

#include "Particle.h"
#include "ParticleCollisionSystem.h"
#include "ParticleEditHandle.h"
#include "ParticleEditPacketSender.h"
#include "ParticleTree.h"

ParticleCollisionSystem::ParticleCollisionSystem(ParticleEditPacketSender* packetSender, 
    ParticleTree* particles, VoxelTree* voxels, AbstractAudioInterface* audio, AvatarData* selfAvatar) {
    init(packetSender, particles, voxels, audio, selfAvatar);
}

void ParticleCollisionSystem::init(ParticleEditPacketSender* packetSender, 
    ParticleTree* particles, VoxelTree* voxels, AbstractAudioInterface* audio, AvatarData* selfAvatar) {
    _packetSender = packetSender;
    _particles = particles;
    _voxels = voxels;
    _audio = audio;
    _selfAvatar = selfAvatar;
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
    updateCollisionWithAvatars(particle);
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

void ParticleCollisionSystem::updateCollisionWithAvatars(Particle* particle) {

    // particles that are in hand, don't collide with other avatar parts
    if (particle->getInHand()) {
        return;
    }

    //printf("updateCollisionWithAvatars()...\n");
    glm::vec3 center = particle->getPosition() * (float)TREE_SCALE;
    float radius = particle->getRadius() * (float)TREE_SCALE;
    const float VOXEL_ELASTICITY = 1.4f;
    const float VOXEL_DAMPING = 0.0;
    const float VOXEL_COLLISION_FREQUENCY = 0.5f;
    glm::vec3 penetration;
    const PalmData* collidingPalm = NULL;
    
    // first check the selfAvatar if set...
    if (_selfAvatar) {
        AvatarData* avatar = (AvatarData*)_selfAvatar;
        //printf("updateCollisionWithAvatars()..._selfAvatar=%p\n", avatar);
        
        // check hands...
        const HandData* handData = avatar->getHandData();

        // if the particle penetrates the hand, then apply a hard collision
        if (handData->findSpherePenetration(center, radius, penetration, collidingPalm)) {
            penetration /= (float)TREE_SCALE;
            updateCollisionSound(particle, penetration, VOXEL_COLLISION_FREQUENCY);
    
            // determine if the palm that collided was moving, if so, then we add that palm velocity as well...
            glm::vec3 addedVelocity = NO_ADDED_VELOCITY;
            if (collidingPalm) {
                glm::vec3 palmVelocity = collidingPalm->getVelocity() / (float)TREE_SCALE;
                //printf("collidingPalm Velocity=%f,%f,%f\n", palmVelocity.x, palmVelocity.y, palmVelocity.z);
                addedVelocity = palmVelocity;
            }

            applyHardCollision(particle, penetration, VOXEL_ELASTICITY, VOXEL_DAMPING, addedVelocity);
        }
    }

    // loop through all the other avatars for potential interactions...
    NodeList* nodeList = NodeList::getInstance();
    for (NodeList::iterator node = nodeList->begin(); node != nodeList->end(); node++) {
        //qDebug() << "updateCollisionWithAvatars()... node:" << *node << "\n";
        if (node->getLinkedData() && node->getType() == NODE_TYPE_AGENT) {
            AvatarData* avatar = (AvatarData*)node->getLinkedData();
            //printf("updateCollisionWithAvatars()...avatar=%p\n", avatar);
            
            // check hands...
            const HandData* handData = avatar->getHandData();

            // if the particle penetrates the hand, then apply a hard collision
            if (handData->findSpherePenetration(center, radius, penetration, collidingPalm)) {
                penetration /= (float)TREE_SCALE;
                updateCollisionSound(particle, penetration, VOXEL_COLLISION_FREQUENCY);

                // determine if the palm that collided was moving, if so, then we add that palm velocity as well...
                glm::vec3 addedVelocity = NO_ADDED_VELOCITY;
                if (collidingPalm) {
                    glm::vec3 palmVelocity = collidingPalm->getVelocity() / (float)TREE_SCALE;
                    //printf("collidingPalm Velocity=%f,%f,%f\n", palmVelocity.x, palmVelocity.y, palmVelocity.z);
                    addedVelocity = palmVelocity;
                }

                applyHardCollision(particle, penetration, VOXEL_ELASTICITY, VOXEL_DAMPING, addedVelocity);

            }
        }
    }
}


void ParticleCollisionSystem::applyHardCollision(Particle* particle, const glm::vec3& penetration, 
                                                 float elasticity, float damping, const glm::vec3& addedVelocity) {
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
        velocity += addedVelocity;
        velocity *= glm::clamp(1.f - damping, 0.0f, 1.0f);
        if (glm::length(velocity) < HALTING_VELOCITY) {
            // If moving really slowly after a collision, and not applying forces, stop altogether
            velocity *= 0.f;
        }
    }
    ParticleEditHandle particleEditHandle(_packetSender, _particles, particle->getID());
    particleEditHandle.updateParticle(position, particle->getRadius(), particle->getXColor(), velocity,
                           particle->getGravity(), particle->getDamping(), particle->getInHand(), particle->getUpdateScript());
}


void ParticleCollisionSystem::updateCollisionSound(Particle* particle, const glm::vec3 &penetration, float frequency) {
                                
    //  consider whether to have the collision make a sound
    const float AUDIBLE_COLLISION_THRESHOLD = 0.1f;
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
                    1.f - DURATION_SCALING * powf(frequency, 0.5f) / velocityTowardCollision, false);
    }
}