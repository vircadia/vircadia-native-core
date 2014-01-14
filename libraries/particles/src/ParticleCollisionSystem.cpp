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
    glm::vec3 center = particle->getPosition() * (float)(TREE_SCALE);
    float radius = particle->getRadius() * (float)(TREE_SCALE);
    const float ELASTICITY = 0.4f;
    const float DAMPING = 0.0f;
    const float COLLISION_FREQUENCY = 0.5f;
    glm::vec3 penetration;
    VoxelDetail* voxelDetails = NULL;
    if (_voxels->findSpherePenetration(center, radius, penetration, (void**)&voxelDetails)) {

        // let the particles run their collision scripts if they have them
        particle->collisionWithVoxel(voxelDetails);

        penetration /= (float)(TREE_SCALE);
        updateCollisionSound(particle, penetration, COLLISION_FREQUENCY);
        applyHardCollision(particle, penetration, ELASTICITY, DAMPING);

        delete voxelDetails; // cleanup returned details
    }
}

void ParticleCollisionSystem::updateCollisionWithParticles(Particle* particleA) {
    glm::vec3 center = particleA->getPosition() * (float)(TREE_SCALE);
    float radius = particleA->getRadius() * (float)(TREE_SCALE);
    //const float ELASTICITY = 0.4f;
    //const float DAMPING = 0.0f;
    const float COLLISION_FREQUENCY = 0.5f;
    glm::vec3 penetration;
    Particle* particleB;
    if (_particles->findSpherePenetration(center, radius, penetration, (void**)&particleB)) {
        // NOTE: 'penetration' is the depth that 'particleA' overlaps 'particleB'.
        // That is, it points from A into B.
    
        // Even if the particles overlap... when the particles are already moving appart
        // we don't want to count this as a collision.
        glm::vec3 relativeVelocity = particleA->getVelocity() - particleB->getVelocity();
        if (glm::dot(relativeVelocity, penetration) > 0.0f) {
            particleA->collisionWithParticle(particleB);
            particleB->collisionWithParticle(particleA);

            glm::vec3 axis = glm::normalize(penetration);
            glm::vec3 axialVelocity = glm::dot(relativeVelocity, axis) * axis;

            // particles that are in hand are assigned an ureasonably large mass for collisions
            // which effectively makes them immovable but allows the other ball to reflect correctly.
            const float MAX_MASS = 1.0e6f;
            float massA = (particleA->getInHand()) ? MAX_MASS : particleA->getMass();
            float massB = (particleB->getInHand()) ? MAX_MASS : particleB->getMass();
            float totalMass = massA + massB;
    
            particleA->setVelocity(particleA->getVelocity() - axialVelocity * (2.0f * massB / totalMass));

            ParticleEditHandle particleEditHandle(_packetSender, _particles, particleA->getID());
            particleEditHandle.updateParticle(particleA->getPosition(), particleA->getRadius(), particleA->getXColor(), particleA->getVelocity(),
                           particleA->getGravity(), particleA->getDamping(), particleA->getInHand(), particleA->getScript());

            particleB->setVelocity(particleB->getVelocity() + axialVelocity * (2.0f * massA / totalMass));

            ParticleEditHandle penetratedparticleEditHandle(_packetSender, _particles, particleB->getID());
            penetratedparticleEditHandle.updateParticle(particleB->getPosition(), particleB->getRadius(), particleB->getXColor(), particleB->getVelocity(),
                           particleB->getGravity(), particleB->getDamping(), particleB->getInHand(), particleB->getScript());

            penetration /= (float)(TREE_SCALE);
            updateCollisionSound(particleA, penetration, COLLISION_FREQUENCY);
        }
    }
}

void ParticleCollisionSystem::updateCollisionWithAvatars(Particle* particle) {

    // particles that are in hand, don't collide with other avatar parts
    if (particle->getInHand()) {
        return;
    }

    //printf("updateCollisionWithAvatars()...\n");
    glm::vec3 center = particle->getPosition() * (float)(TREE_SCALE);
    float radius = particle->getRadius() * (float)(TREE_SCALE);
    const float ELASTICITY = 0.4f;
    const float DAMPING = 0.0f;
    const float COLLISION_FREQUENCY = 0.5f;
    glm::vec3 penetration;
    const PalmData* collidingPalm = NULL;

    // first check the selfAvatar if set...
    if (_selfAvatar) {
        AvatarData* avatar = (AvatarData*)_selfAvatar;
        //printf("updateCollisionWithAvatars()..._selfAvatar=%p\n", avatar);

        // check hands...
        const HandData* handData = avatar->getHandData();

        // TODO: combine hand and collision check into one.  Note: would need to supply
        // CollisionInfo class rather than just vec3 (penetration) so we can get back
        // added velocity.

        if (handData->findSpherePenetration(center, radius, penetration, collidingPalm)) {
            // TODO: dot collidingPalm and hand velocities and skip collision when they are moving apart.
            // apply a hard collision when ball collides with hand
            penetration /= (float)(TREE_SCALE);
            updateCollisionSound(particle, penetration, COLLISION_FREQUENCY);

            // determine if the palm that collided was moving, if so, then we add that palm velocity as well...
            glm::vec3 addedVelocity = NO_ADDED_VELOCITY;
            if (collidingPalm) {
                glm::vec3 palmVelocity = collidingPalm->getVelocity() / (float)(TREE_SCALE);
                //printf("collidingPalm Velocity=%f,%f,%f\n", palmVelocity.x, palmVelocity.y, palmVelocity.z);
                addedVelocity = palmVelocity;
            }

            applyHardCollision(particle, penetration, ELASTICITY, DAMPING, addedVelocity);
        } else if (avatar->findSpherePenetration(center, radius, penetration)) {
            // apply hard collision when particle collides with avatar
            penetration /= (float)(TREE_SCALE);
            updateCollisionSound(particle, penetration, COLLISION_FREQUENCY);
            glm::vec3 addedVelocity = avatar->getVelocity();
            applyHardCollision(particle, penetration, ELASTICITY, DAMPING, addedVelocity);
        }
    }

    // loop through all the other avatars for potential interactions...
    
    foreach(SharedNodePointer node, NodeList::getInstance()->getNodeHash()) {
        //qDebug() << "updateCollisionWithAvatars()... node:" << *node << "\n";
        if (node->getLinkedData() && node->getType() == NODE_TYPE_AGENT) {
            // TODO: dot collidingPalm and hand velocities and skip collision when they are moving apart.
            AvatarData* avatar = static_cast<AvatarData*>(node->getLinkedData());
            //printf("updateCollisionWithAvatars()...avatar=%p\n", avatar);
            
            // check hands...
            const HandData* handData = avatar->getHandData();
            
            if (handData->findSpherePenetration(center, radius, penetration, collidingPalm)) {
                // apply a hard collision when ball collides with hand
                penetration /= (float)(TREE_SCALE);
                updateCollisionSound(particle, penetration, COLLISION_FREQUENCY);
                
                // determine if the palm that collided was moving, if so, then we add that palm velocity as well...
                glm::vec3 addedVelocity = NO_ADDED_VELOCITY;
                if (collidingPalm) {
                    glm::vec3 palmVelocity = collidingPalm->getVelocity() / (float)(TREE_SCALE);
                    //printf("collidingPalm Velocity=%f,%f,%f\n", palmVelocity.x, palmVelocity.y, palmVelocity.z);
                    addedVelocity = palmVelocity;
                }
                
                applyHardCollision(particle, penetration, ELASTICITY, DAMPING, addedVelocity);
                
            } else if (avatar->findSpherePenetration(center, radius, penetration)) {
                penetration /= (float)(TREE_SCALE);
                updateCollisionSound(particle, penetration, COLLISION_FREQUENCY);
                glm::vec3 addedVelocity = avatar->getVelocity();
                applyHardCollision(particle, penetration, ELASTICITY, DAMPING, addedVelocity);
            }
        }
    }
}


void ParticleCollisionSystem::applyHardCollision(Particle* particle, const glm::vec3& penetration,
                                                 float elasticity, float damping, const glm::vec3& addedVelocity) {
    //
    //  Update the particle in response to a hard collision.  Position will be reset exactly
    //  to outside the colliding surface.  Velocity will be modified according to elasticity.
    //
    //  if elasticity = 0.0, collision is inelastic (vel normal to collision is lost)
    //  if elasticity = 1.0, collision is 100% elastic.
    //
    glm::vec3 position = particle->getPosition();
    glm::vec3 velocity = particle->getVelocity();

    const float EPSILON = 0.0f;
    float velocityDotPenetration = glm::dot(velocity, penetration);
    if (velocityDotPenetration > EPSILON) {
        position -= penetration;
        static float HALTING_VELOCITY = 0.2f / (float)(TREE_SCALE);
        // cancel out the velocity component in the direction of penetration

        float penetrationLength = glm::length(penetration);
        glm::vec3 direction = penetration / penetrationLength;
        velocity -= (glm::dot(velocity, direction) * (1.0f + elasticity)) * direction;
        velocity += addedVelocity;
        velocity *= glm::clamp(1.f - damping, 0.0f, 1.0f);
        if (glm::length(velocity) < HALTING_VELOCITY) {
            // If moving really slowly after a collision, and not applying forces, stop altogether
            velocity *= 0.f;
        }
    }
    const bool wantDebug = false;
    if (wantDebug) {
        printf("ParticleCollisionSystem::applyHardCollision() particle id:%d new velocity:%f,%f,%f inHand:%s\n",
            particle->getID(), velocity.x, velocity.y, velocity.z, debug::valueOf(particle->getInHand()));
    }

    ParticleEditHandle particleEditHandle(_packetSender, _particles, particle->getID());
    particleEditHandle.updateParticle(position, particle->getRadius(), particle->getXColor(), velocity,
                           particle->getGravity(), particle->getDamping(), particle->getInHand(), particle->getScript());
}


void ParticleCollisionSystem::updateCollisionSound(Particle* particle, const glm::vec3 &penetration, float frequency) {

    //  consider whether to have the collision make a sound
    const float AUDIBLE_COLLISION_THRESHOLD = 0.1f;
    const float COLLISION_LOUDNESS = 1.f;
    const float DURATION_SCALING = 0.004f;
    const float NOISE_SCALING = 0.1f;
    glm::vec3 velocity = particle->getVelocity() * (float)(TREE_SCALE);

    /*
    // how do we want to handle this??
    //
     glm::vec3 gravity = particle->getGravity() * (float)(TREE_SCALE);

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
