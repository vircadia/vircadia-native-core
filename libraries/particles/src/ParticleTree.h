//
//  ParticleTree.h
//  hifi
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __hifi__ParticleTree__
#define __hifi__ParticleTree__

#include <set>
#include <SimpleMovingAverage.h>
#include <OctreeElementBag.h>
#include <Octree.h>
#include <CoverageMap.h>
#include <JurisdictionMap.h>
#include <ViewFrustum.h>

#include "ParticleTreeElement.h"
#include "VoxelPacketData.h"
#include "VoxelSceneStats.h"
#include "VoxelEditPacketSender.h"

class ReadCodeColorBufferToTreeArgs;

class ParticleTree : public Octree {
    Q_OBJECT
public:
    ParticleTree(bool shouldReaverage = false);
    virtual ParticleTreeElement* createNewElement(unsigned char * octalCode = NULL) const;
    ParticleTreeElement* getRoot() { return (ParticleTreeElement*)_rootNode; }
private:
};

#endif /* defined(__hifi__ParticleTree__) */
