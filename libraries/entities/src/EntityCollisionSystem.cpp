//
//  EntityCollisionSystem.cpp
//  libraries/entities/src
//
//  Created by Brad Hefta-Gaub on 9/23/14.
//  Copyright 2013-2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <algorithm>
#include <AbstractAudioInterface.h>
#include <VoxelTree.h>
#include <AvatarData.h>
#include <HeadData.h>
#include <HandData.h>

#include "EntityItem.h"
#include "EntityCollisionSystem.h"
#include "EntityEditPacketSender.h"
#include "EntityTree.h"
#include "EntityTreeElement.h"

const int MAX_COLLISIONS_PER_Entity = 16;

EntityCollisionSystem::EntityCollisionSystem(EntityEditPacketSender* packetSender,
    EntityTree* Entities, VoxelTree* voxels, AbstractAudioInterface* audio,
    AvatarHashMap* avatars) : _collisions(MAX_COLLISIONS_PER_Entity) {
    init(packetSender, Entities, voxels, audio, avatars);
}

void EntityCollisionSystem::init(EntityEditPacketSender* packetSender,
    EntityTree* Entities, VoxelTree* voxels, AbstractAudioInterface* audio,
    AvatarHashMap* avatars) {
    _packetSender = packetSender;
    _entities = Entities;
    _voxels = voxels;
    _audio = audio;
    _avatars = avatars;
}

EntityCollisionSystem::~EntityCollisionSystem() {
}

bool EntityCollisionSystem::updateOperation(OctreeElement* element, void* extraData) {
    EntityCollisionSystem* system = static_cast<EntityCollisionSystem*>(extraData);
    EntityTreeElement* entityTreeElement = static_cast<EntityTreeElement*>(element);

    // iterate the Entities...
    QList<EntityItem*>& entities = entityTreeElement->getEntities();
    uint16_t numberOfEntities = entities.size();
    for (uint16_t i = 0; i < numberOfEntities; i++) {
        EntityItem* entity = entities[i];
        system->checkEntity(entity);
    }

    return true;
}


void EntityCollisionSystem::update() {
    // update all Entities
    if (_entities->tryLockForRead()) {
        _entities->recurseTreeWithOperation(updateOperation, this);
        _entities->unlock();
    }
}


void EntityCollisionSystem::checkEntity(EntityItem* entity) {
    updateCollisionWithVoxels(entity);
    updateCollisionWithEntities(entity);
    updateCollisionWithAvatars(entity);
}

void EntityCollisionSystem::emitGlobalEntityCollisionWithVoxel(EntityItem* entity, 
                                            VoxelDetail* voxelDetails, const CollisionInfo& collision) {
    EntityItemID entityItemID = entity->getEntityItemID();
    emit EntityCollisionWithVoxel(entityItemID, *voxelDetails, collision);
}

void EntityCollisionSystem::emitGlobalEntityCollisionWithEntity(EntityItem* entityA, 
                                            EntityItem* entityB, const CollisionInfo& collision) {
                                            
    EntityItemID idA = entityA->getEntityItemID();
    EntityItemID idB = entityB->getEntityItemID();
    emit EntityCollisionWithEntity(idA, idB, collision);
}

void EntityCollisionSystem::updateCollisionWithVoxels(EntityItem* entity) {
    glm::vec3 center = entity->getPosition() * (float)(TREE_SCALE);
    float radius = entity->getRadius() * (float)(TREE_SCALE);
    const float ELASTICITY = 0.4f;
    const float DAMPING = 0.05f;
    const float COLLISION_FREQUENCY = 0.5f;
    CollisionInfo collisionInfo;
    collisionInfo._damping = DAMPING;
    collisionInfo._elasticity = ELASTICITY;
    VoxelDetail* voxelDetails = NULL;
    if (_voxels->findSpherePenetration(center, radius, collisionInfo._penetration, (void**)&voxelDetails)) {

        // let the Entities run their collision scripts if they have them
        //entity->collisionWithVoxel(voxelDetails, collisionInfo._penetration);

        // findSpherePenetration() only computes the penetration but we also want some other collision info
        // so we compute it ourselves here.  Note that we must multiply scale by TREE_SCALE when feeding 
        // the results to systems outside of this octree reference frame.
        updateCollisionSound(entity, collisionInfo._penetration, COLLISION_FREQUENCY);
        collisionInfo._contactPoint = (float)TREE_SCALE * (entity->getPosition() + entity->getRadius() * glm::normalize(collisionInfo._penetration));
        // let the global script run their collision scripts for Entities if they have them
        emitGlobalEntityCollisionWithVoxel(entity, voxelDetails, collisionInfo);

        // we must scale back down to the octree reference frame before updating the Entity properties
        collisionInfo._penetration /= (float)(TREE_SCALE);
        collisionInfo._contactPoint /= (float)(TREE_SCALE);
        entity->applyHardCollision(collisionInfo);
        queueEntityPropertiesUpdate(entity);

        delete voxelDetails; // cleanup returned details
    }
}

void EntityCollisionSystem::updateCollisionWithEntities(EntityItem* entityA) {
    glm::vec3 center = entityA->getPosition() * (float)(TREE_SCALE);
    float radius = entityA->getRadius() * (float)(TREE_SCALE);
    //const float ELASTICITY = 0.4f;
    //const float DAMPING = 0.0f;
    const float COLLISION_FREQUENCY = 0.5f;
    glm::vec3 penetration;
    EntityItem* entityB;
    if (_entities->findSpherePenetration(center, radius, penetration, (void**)&entityB, Octree::NoLock)) {
        // NOTE: 'penetration' is the depth that 'entityA' overlaps 'entityB'.  It points from A into B.
        glm::vec3 penetrationInTreeUnits = penetration / (float)(TREE_SCALE);

        // Even if the Entities overlap... when the Entities are already moving appart
        // we don't want to count this as a collision.
        glm::vec3 relativeVelocity = entityA->getVelocity() - entityB->getVelocity();

        bool movingTowardEachOther = glm::dot(relativeVelocity, penetrationInTreeUnits) > 0.0f;
        bool doCollisions = true;

        if (doCollisions) {
            entityA->collisionWithEntity(entityB, penetration);
            entityB->collisionWithEntity(entityA, penetration * -1.0f); // the penetration is reversed

            CollisionInfo collision;
            collision._penetration = penetration;
            // for now the contactPoint is the average between the the two paricle centers
            collision._contactPoint = (0.5f * (float)TREE_SCALE) * (entityA->getPosition() + entityB->getPosition());
            emitGlobalEntityCollisionWithEntity(entityA, entityB, collision);

            glm::vec3 axis = glm::normalize(penetration);
            glm::vec3 axialVelocity = glm::dot(relativeVelocity, axis) * axis;

            float massA = entityA->getMass();
            float massB = entityB->getMass();
            float totalMass = massA + massB;

            // handle Entity A
            glm::vec3 newVelocityA = entityA->getVelocity() - axialVelocity * (2.0f * massB / totalMass);
            glm::vec3 newPositionA = entityA->getPosition() - 0.5f * penetrationInTreeUnits;

            EntityItemProperties propertiesA = entityA->getProperties();
            EntityItemID idA(entityA->getID());
            propertiesA.setVelocity(newVelocityA * (float)TREE_SCALE);
            propertiesA.setPosition(newPositionA * (float)TREE_SCALE);

            _entities->updateEntity(idA, propertiesA);
            _packetSender->queueEditEntityMessage(PacketTypeEntityAddOrEdit, idA, propertiesA);

            glm::vec3 newVelocityB = entityB->getVelocity() + axialVelocity * (2.0f * massA / totalMass);
            glm::vec3 newPositionB = entityB->getPosition() + 0.5f * penetrationInTreeUnits;

            EntityItemProperties propertiesB = entityB->getProperties();

            EntityItemID idB(entityB->getID());
            propertiesB.setVelocity(newVelocityB * (float)TREE_SCALE);
            propertiesB.setPosition(newPositionB * (float)TREE_SCALE);

            _entities->updateEntity(idB, propertiesB);

            _packetSender->queueEditEntityMessage(PacketTypeEntityAddOrEdit, idB, propertiesB);
            
            // TODO: Do we need this?
            //_packetSender->releaseQueuedMessages();

            //updateCollisionSound(entityA, penetration, COLLISION_FREQUENCY);
        }
    }
}

void EntityCollisionSystem::updateCollisionWithAvatars(EntityItem* entity) {
    
    // TODO: implement support for colliding with avatars

#if 0
    // Entities that are in hand, don't collide with avatars
    if (!_avatars) {
        return;
    }

    glm::vec3 center = entity->getPosition() * (float)(TREE_SCALE);
    float radius = entity->getRadius() * (float)(TREE_SCALE);
    const float ELASTICITY = 0.9f;
    const float DAMPING = 0.1f;
    const float COLLISION_FREQUENCY = 0.5f;
    glm::vec3 penetration;

    _collisions.clear();
    foreach (const AvatarSharedPointer& avatarPointer, _avatars->getAvatarHash()) {
        AvatarData* avatar = avatarPointer.data();

        float totalRadius = avatar->getBoundingRadius() + radius;
        glm::vec3 relativePosition = center - avatar->getPosition();
        if (glm::dot(relativePosition, relativePosition) > (totalRadius * totalRadius)) {
            continue;
        }

        if (avatar->findSphereCollisions(center, radius, _collisions)) {
            int numCollisions = _collisions.size();
            for (int i = 0; i < numCollisions; ++i) {
                CollisionInfo* collision = _collisions.getCollision(i);
                collision->_damping = DAMPING;
                collision->_elasticity = ELASTICITY;
    
                collision->_addedVelocity /= (float)(TREE_SCALE);
                glm::vec3 relativeVelocity = collision->_addedVelocity - entity->getVelocity();
    
                if (glm::dot(relativeVelocity, collision->_penetration) <= 0.f) {
                    // only collide when Entity and collision point are moving toward each other
                    // (doing this prevents some "collision snagging" when Entity penetrates the object)
                    updateCollisionSound(entity, collision->_penetration, COLLISION_FREQUENCY);
                    collision->_penetration /= (float)(TREE_SCALE);
                    entity->applyHardCollision(*collision);
                    queueEntityPropertiesUpdate(entity);
                }
            }
        }
    }
    
#endif
}

void EntityCollisionSystem::queueEntityPropertiesUpdate(EntityItem* entity) {
    // queue the result for sending to the Entity server
    EntityItemProperties properties = entity->getProperties();
    EntityItemID entityItemID(entity->getID());

    properties.setPosition(entity->getPosition() * (float)TREE_SCALE);
    properties.setVelocity(entity->getVelocity() * (float)TREE_SCALE);
    _packetSender->queueEditEntityMessage(PacketTypeEntityAddOrEdit, entityItemID, properties);
}


void EntityCollisionSystem::updateCollisionSound(EntityItem* Entity, const glm::vec3 &penetration, float frequency) {

    //  consider whether to have the collision make a sound
    const float AUDIBLE_COLLISION_THRESHOLD = 0.3f;
    const float COLLISION_LOUDNESS = 1.f;
    const float DURATION_SCALING = 0.004f;
    const float NOISE_SCALING = 0.1f;
    glm::vec3 velocity = Entity->getVelocity() * (float)(TREE_SCALE);

    /*
    // how do we want to handle this??
    //
     glm::vec3 gravity = Entity->getGravity() * (float)(TREE_SCALE);

    if (glm::length(gravity) > EPSILON) {
        //  If gravity is on, remove the effect of gravity on velocity for this
        //  frame, so that we are not constantly colliding with the surface
        velocity -= _scale * glm::length(gravity) * GRAVITY_EARTH * deltaTime * glm::normalize(gravity);
    }
    */
    float normalSpeed = glm::dot(velocity, glm::normalize(penetration));
    // NOTE: it is possible for normalSpeed to be NaN at this point 
    // (sometimes the average penetration of a bunch of voxels is a zero length vector which cannot be normalized) 
    // however the check below will fail (NaN comparisons always fail) and everything will be fine.

    if (normalSpeed > AUDIBLE_COLLISION_THRESHOLD) {
        //  Volume is proportional to collision velocity
        //  Base frequency is modified upward by the angle of the collision
        //  Noise is a function of the angle of collision
        //  Duration of the sound is a function of both base frequency and velocity of impact
        float tangentialSpeed = glm::length(velocity) - normalSpeed;
        _audio->startCollisionSound( std::min(COLLISION_LOUDNESS * normalSpeed, 1.f),
            frequency * (1.f + tangentialSpeed / normalSpeed),
            std::min(tangentialSpeed / normalSpeed * NOISE_SCALING, 1.f),
            1.f - DURATION_SCALING * powf(frequency, 0.5f) / normalSpeed, false);
    }
}
