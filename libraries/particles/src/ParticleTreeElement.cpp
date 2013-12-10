//
//  ParticleTreeElement.cpp
//  hifi
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include <QtCore/QDebug>

#include "ParticleTree.h"
#include "ParticleTreeElement.h"

ParticleTreeElement::ParticleTreeElement(unsigned char* octalCode) : OctreeElement() { 
    init(octalCode);
};

ParticleTreeElement::~ParticleTreeElement() {
    _voxelMemoryUsage -= sizeof(ParticleTreeElement);
}

// This will be called primarily on addChildAt(), which means we're adding a child of our
// own type to our own tree. This means we should initialize that child with any tree and type 
// specific settings that our children must have. One example is out VoxelSystem, which
// we know must match ours.
OctreeElement* ParticleTreeElement::createNewElement(unsigned char* octalCode) const {
    ParticleTreeElement* newChild = new ParticleTreeElement(octalCode);
    return newChild;
}

void ParticleTreeElement::init(unsigned char* octalCode) {
    OctreeElement::init(octalCode);
    _voxelMemoryUsage += sizeof(ParticleTreeElement);
}

ParticleTreeElement* ParticleTreeElement::addChildAtIndex(int index) { 
    return (ParticleTreeElement*)OctreeElement::addChildAtIndex(index); 
}


bool ParticleTreeElement::appendElementData(OctreePacketData* packetData) const {
    bool success = true; // assume the best...

    // write our particles out...
    uint16_t numberOfParticles = _particles.size();
    success = packetData->appendValue(numberOfParticles);

    //printf("ParticleTreeElement::appendElementData()... numberOfParticles=%d\n",numberOfParticles);
    
    if (success) {
        for (uint16_t i = 0; i < numberOfParticles; i++) {
            const Particle& particle = _particles[i];
            success = particle.appendParticleData(packetData);
            if (!success) {
                break;
            }
        }
    }
    return success;
}

void ParticleTreeElement::update(ParticleTreeUpdateArgs& args) {
    markWithChangedTime();

    // update our contained particles
    uint16_t numberOfParticles = _particles.size();
    for (uint16_t i = 0; i < numberOfParticles; i++) {
        _particles[i].update();

        // If the particle wants to die, or if it's left our bounding box, then move it
        // into the arguments moving particles. These will be added back or deleted completely
        if (_particles[i].getShouldDie() || !_box.contains(_particles[i].getPosition())) {
        
            glm::vec3 position = _particles[i].getPosition() * (float)TREE_SCALE;
            glm::vec3 boxBRN =_box.getCorner() * (float)TREE_SCALE;
            glm::vec3 boxTLF =_box.calcTopFarLeft() * (float)TREE_SCALE;
            
            /**
            printf("particle [%f,%f,%f] no longer contained in ParticleTreeElement() box [%f,%f,%f] -> [%f,%f,%f]\n",
                position.x, position.y, position.z, 
                boxBRN.x, boxBRN.y, boxBRN.z,
                boxTLF.x, boxTLF.y, boxTLF.z);
            **/
                
            args._movingParticles.push_back(_particles[i]);
            
            // erase this particle
            _particles.erase(_particles.begin()+i);
            //printf("removed particle[%d]\n",i);
            
            // reduce our index since we just removed this item
            i--;
            numberOfParticles--;
            //printf("numberOfParticles=%d\n",numberOfParticles);
        }
    }
}

int ParticleTreeElement::readElementDataFromBuffer(const unsigned char* data, int bytesLeftToRead, 
            ReadBitstreamToTreeParams& args) { 
    
    const unsigned char* dataAt = data;
    int bytesRead = 0;
    uint16_t numberOfParticles = 0;
    int expectedBytesPerParticle = Particle::expectedBytes();
    
    _particles.clear();
    
    if (bytesLeftToRead >= sizeof(numberOfParticles)) {
    
        // read our particles in....
        numberOfParticles = *(uint16_t*)dataAt;
        dataAt += sizeof(numberOfParticles);
        bytesLeftToRead -= sizeof(numberOfParticles);
        bytesRead += sizeof(numberOfParticles);
        
        if (bytesLeftToRead >= (numberOfParticles * expectedBytesPerParticle)) {
            for (uint16_t i = 0; i < numberOfParticles; i++) {
                Particle tempParticle;
                int bytesForThisParticle = tempParticle.readParticleDataFromBuffer(dataAt, bytesLeftToRead, args);
                _particles.push_back(tempParticle);
                dataAt += bytesForThisParticle;
                bytesLeftToRead -= bytesForThisParticle;
                bytesRead += bytesForThisParticle;
            }
        }
    }
    return bytesRead;
}

// will average a "common reduced LOD view" from the the child elements...
void ParticleTreeElement::calculateAverageFromChildren() {
    // nothing to do here yet...
}

// will detect if children are leaves AND collapsable into the parent node
// and in that case will collapse children and make this node
// a leaf, returns TRUE if all the leaves are collapsed into a 
// single node
bool ParticleTreeElement::collapseChildren() {
    // nothing to do here yet...
    return false;
}


void ParticleTreeElement::storeParticle(const Particle& particle) {
    _particles.push_back(particle);

    markWithChangedTime();

    /***
    printf("ParticleTreeElement::storeParticle() element=%p _particles.size()=%ld particle.getPosition()=%f,%f,%f\n", 
        this, _particles.size(),
        particle.getPosition().x, particle.getPosition().y, particle.getPosition().z);
    **/
}

