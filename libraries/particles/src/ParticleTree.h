//
//  ParticleTree.h
//  hifi
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __hifi__ParticleTree__
#define __hifi__ParticleTree__

#include <Octree.h>
#include "ParticleTreeElement.h"

class ParticleTree : public Octree {
    Q_OBJECT
public:
    ParticleTree(bool shouldReaverage = false);
    virtual ParticleTreeElement* createNewElement(unsigned char * octalCode = NULL) const;
    ParticleTreeElement* getRoot() { return (ParticleTreeElement*)_rootNode; }
private:
};

#endif /* defined(__hifi__ParticleTree__) */
