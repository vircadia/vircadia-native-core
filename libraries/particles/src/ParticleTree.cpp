//
//  ParticleTree.cpp
//  hifi
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include "ParticleTree.h"

ParticleTree::ParticleTree(bool shouldReaverage) : Octree(shouldReaverage) {
    ParticleTreeElement* rootNode = createNewElement();
    rootNode->setTree(this);
    _rootNode = rootNode;
}

ParticleTreeElement* ParticleTree::createNewElement(unsigned char * octalCode) const {
    ParticleTreeElement* newElement = new ParticleTreeElement(octalCode); 
    return newElement;
}

bool ParticleTree::handlesEditPacketType(PACKET_TYPE packetType) const {
    // we handle these types of "edit" packets
    switch (packetType) {
        case PACKET_TYPE_PARTICLE_ADD_OR_EDIT:
        case PACKET_TYPE_PARTICLE_ERASE:
            return true;
    }
    return false;
}


class FindAndUpdateParticleArgs {
public:
    const Particle& searchParticle;
    bool found;
};
    
bool ParticleTree::findAndUpdateOperation(OctreeElement* element, void* extraData) {
    FindAndUpdateParticleArgs* args = static_cast<FindAndUpdateParticleArgs*>(extraData);
    ParticleTreeElement* particleTreeElement = static_cast<ParticleTreeElement*>(element);
    if (particleTreeElement->containsParticle(args->searchParticle)) {
        particleTreeElement->updateParticle(args->searchParticle);
        args->found = true;
        return false; // stop searching
    }
    return true;
}

void ParticleTree::storeParticle(const Particle& particle) {
    // First, look for the existing particle in the tree..
    FindAndUpdateParticleArgs args = { particle, false };
    recurseTreeWithOperation(findAndUpdateOperation, &args);
    
    // if we didn't find it in the tree, then store it...
    if (!args.found) {
        glm::vec3 position = particle.getPosition();
        float size = particle.getRadius();
        ParticleTreeElement* element = (ParticleTreeElement*)getOrCreateChildElementAt(position.x, position.y, position.z, size);

        element->storeParticle(particle);
    }    
    // what else do we need to do here to get reaveraging to work
    _isDirty = true;
}

class FindNearPointArgs {
public:
    glm::vec3 position;
    float targetRadius;
    bool found;
    const Particle* closestParticle;
    float closestParticleDistance;
};
    

bool ParticleTree::findNearPointOperation(OctreeElement* element, void* extraData) {
    FindNearPointArgs* args = static_cast<FindNearPointArgs*>(extraData);
    ParticleTreeElement* particleTreeElement = static_cast<ParticleTreeElement*>(element);


    // If this particleTreeElement contains the point, then search it...
    if (particleTreeElement->getAABox().contains(args->position)) {
        const Particle* thisClosestParticle = particleTreeElement->getClosestParticle(args->position);
        
        // we may have gotten NULL back, meaning no particle was available
        if (thisClosestParticle) {
            glm::vec3 particlePosition = thisClosestParticle->getPosition();
            float distanceFromPointToParticle = glm::distance(particlePosition, args->position);
            
            // If we're within our target radius
            if (distanceFromPointToParticle <= args->targetRadius) {
                // we are closer than anything else we've found
                if (distanceFromPointToParticle < args->closestParticleDistance) {
                    args->closestParticle = thisClosestParticle;
                    args->closestParticleDistance = distanceFromPointToParticle;
                    args->found = true;
                }
            }
        }
        
        // we should be able to optimize this...
        return true; // keep searching in case children have closer particles
    }
    
    // if this element doesn't contain the point, then none of it's children can contain the point, so stop searching
    return false;
}

const Particle* ParticleTree::findClosestParticle(glm::vec3 position, float targetRadius) {
    // First, look for the existing particle in the tree..
    FindNearPointArgs args = { position, targetRadius, false, NULL, FLT_MAX };
    recurseTreeWithOperation(findNearPointOperation, &args);
    return args.closestParticle;
}


int ParticleTree::processEditPacketData(PACKET_TYPE packetType, unsigned char* packetData, int packetLength,
                    unsigned char* editData, int maxLength, Node* senderNode) {

    int processedBytes = 0;
    // we handle these types of "edit" packets
    switch (packetType) {
        case PACKET_TYPE_PARTICLE_ADD_OR_EDIT: {
            Particle newParticle = Particle::fromEditPacket(editData, maxLength, processedBytes);
            storeParticle(newParticle);
            if (newParticle.isNewlyCreated()) {
                notifyNewlyCreatedParticle(newParticle, senderNode);
            }
        } break;
        
        case PACKET_TYPE_PARTICLE_ERASE: {
            processedBytes = 0;
        } break;
    }
    return processedBytes;
}

void ParticleTree::notifyNewlyCreatedParticle(const Particle& newParticle, Node* senderNode) {
    _newlyCreatedHooksLock.lockForRead();
    for (int i = 0; i < _newlyCreatedHooks.size(); i++) {
        _newlyCreatedHooks[i]->particleCreated(newParticle, senderNode);
    }
    _newlyCreatedHooksLock.unlock();
}

void ParticleTree::addNewlyCreatedHook(NewlyCreatedParticleHook* hook) {
    _newlyCreatedHooksLock.lockForWrite();
    _newlyCreatedHooks.push_back(hook);
    _newlyCreatedHooksLock.unlock();
}

void ParticleTree::removeNewlyCreatedHook(NewlyCreatedParticleHook* hook) {
    _newlyCreatedHooksLock.lockForWrite();
    for (int i = 0; i < _newlyCreatedHooks.size(); i++) {
        if (_newlyCreatedHooks[i] == hook) {
            _newlyCreatedHooks.erase(_newlyCreatedHooks.begin() + i);
            break;
        }
    }
    _newlyCreatedHooksLock.unlock();
}


bool ParticleTree::updateOperation(OctreeElement* element, void* extraData) {
    ParticleTreeUpdateArgs* args = static_cast<ParticleTreeUpdateArgs*>(extraData);
    ParticleTreeElement* particleTreeElement = static_cast<ParticleTreeElement*>(element);
    particleTreeElement->update(*args);
    return true;
}

bool ParticleTree::pruneOperation(OctreeElement* element, void* extraData) {
    ParticleTreeElement* particleTreeElement = static_cast<ParticleTreeElement*>(element);
    for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
        ParticleTreeElement* childAt = particleTreeElement->getChildAtIndex(i);
        if (childAt && childAt->isLeaf() && !childAt->hasParticles()) {
            particleTreeElement->deleteChildAtIndex(i);
        }
    }
    return true;
}

void ParticleTree::update() {
    _isDirty = true;

    ParticleTreeUpdateArgs args = { };
    recurseTreeWithOperation(updateOperation, &args);
    
    // now add back any of the particles that moved elements....
    int movingParticles = args._movingParticles.size();
    for (int i = 0; i < movingParticles; i++) {
        bool shouldDie = args._movingParticles[i].getShouldDie();

        // if the particle is still inside our total bounds, then re-add it
        AABox treeBounds = getRoot()->getAABox();
        
        if (!shouldDie && treeBounds.contains(args._movingParticles[i].getPosition())) {
            storeParticle(args._movingParticles[i]);
        }
    }
    
    // prune the tree...
    recurseTreeWithOperation(pruneOperation, NULL);
}


