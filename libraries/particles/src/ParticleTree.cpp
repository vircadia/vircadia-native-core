//
//  ParticleTree.cpp
//  hifi
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include "ParticleTree.h"

ParticleTree::ParticleTree(bool shouldReaverage) : Octree(shouldReaverage) {
    _rootNode = createNewElement();
}

ParticleTreeElement* ParticleTree::createNewElement(unsigned char * octalCode) const {
    ParticleTreeElement* newElement = new ParticleTreeElement(octalCode); 
    return newElement;
}

bool ParticleTree::handlesEditPacketType(PACKET_TYPE packetType) const {
    // we handle these types of "edit" packets
    switch (packetType) {
        case PACKET_TYPE_PARTICLE_ADD:
        case PACKET_TYPE_PARTICLE_ERASE:
            return true;
    }
    return false;
}

void ParticleTree::storeParticle(const Particle& particle) {
    glm::vec3 position = particle.getPosition();
    float size = particle.getRadius();
    ParticleTreeElement* element = (ParticleTreeElement*)getOrCreateChildElementAt(position.x, position.y, position.z, size);

    //printf("ParticleTree::storeParticle() element=%p particle.getPosition()=%f,%f,%f\n", 
    //        element, particle.getPosition().x, particle.getPosition().y, particle.getPosition().z);

    element->storeParticle(particle);
    
    // what else do we need to do here to get reaveraging to work
    _isDirty = true;
}

int ParticleTree::processEditPacketData(PACKET_TYPE packetType, unsigned char* packetData, int packetLength,
                    unsigned char* editData, int maxLength) {

    int processedBytes = 0;
    // we handle these types of "edit" packets
    switch (packetType) {
        case PACKET_TYPE_PARTICLE_ADD: {
        
            //printf("got PACKET_TYPE_PARTICLE_ADD....\n");
            Particle newParticle = Particle::fromEditPacket(editData, maxLength, processedBytes);

            //printf("newParticle...getPosition()=%f,%f,%f\n", newParticle.getPosition().x, newParticle.getPosition().y, newParticle.getPosition().z);
            storeParticle(newParticle);

            // It seems like we need some way to send the ID back to the creator??
        } break;
            
        case PACKET_TYPE_PARTICLE_ERASE: {
            processedBytes = 0;
        } break;
    }
    return processedBytes;
}


bool ParticleTree::updateOperation(OctreeElement* element, void* extraData) {
    ParticleTreeUpdateArgs* args = static_cast<ParticleTreeUpdateArgs*>(extraData);
    ParticleTreeElement* particleTreeElement = static_cast<ParticleTreeElement*>(element);
    particleTreeElement->update(*args);
    return true;
}

void ParticleTree::update() {
    _isDirty = true;

    ParticleTreeUpdateArgs args = { };
    recurseTreeWithOperation(updateOperation, &args);
    
    // now add back any of the particles that moved elements....
    int movingParticles = args._movingParticles.size();
    for (int i = 0; i < movingParticles; i++) {
        // if the particle is still inside our total bounds, then re-add it
        AABox treeBounds = getRoot()->getAABox();
        float velocityScalar = glm::length(args._movingParticles[i].getVelocity());
        const float STILL_MOVING = 0.0001;
        
        if (velocityScalar > STILL_MOVING && treeBounds.contains(args._movingParticles[i].getPosition())) {
            printf("re-storing moved particle...\n");
            storeParticle(args._movingParticles[i]);
        } else {
            printf("out of bounds or !velocityScalar=[%f], not re-storing moved particle...\n",velocityScalar);
        }
    }
}


