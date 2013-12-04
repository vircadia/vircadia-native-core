//
//  ParticleTreeElement.cpp
//  hifi
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include <QtCore/QDebug>
#include <NodeList.h>
#include <PerfStat.h>

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

bool ParticleTreeElement::appendElementData(OctreePacketData* packetData) const {
    // will write to the encoded stream all of the contents of this element
    return true;
}


int ParticleTreeElement::readElementDataFromBuffer(const unsigned char* data, int bytesLeftToRead, 
            ReadBitstreamToTreeParams& args) { 
            
    // will read from the encoded stream all of the contents of this element
    return 0;
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

