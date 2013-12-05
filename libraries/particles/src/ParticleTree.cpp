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
    element->storeParticle(particle);
}

int ParticleTree::processEditPacketData(PACKET_TYPE packetType, unsigned char* packetData, int packetLength,
                    unsigned char* editData, int maxLength) {

    int processedBytes = 0;
    // we handle these types of "edit" packets
    switch (packetType) {
        case PACKET_TYPE_PARTICLE_ADD: {
            Particle newParticle = Particle::fromEditPacket(editData, maxLength, processedBytes);
            storeParticle(newParticle);

            // It seems like we need some way to send the ID back to the creator??
            
        } break;
            
        case PACKET_TYPE_PARTICLE_ERASE: {
            processedBytes = 0;
        } break;
    }
    return processedBytes;
}

