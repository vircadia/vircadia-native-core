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
    
    /// Implements our type specific root element factory
    virtual ParticleTreeElement* createNewElement(unsigned char * octalCode = NULL) const;
    
    /// Type safe version of getRoot()
    ParticleTreeElement* getRoot() { return (ParticleTreeElement*)_rootNode; }
    
    
    // These methods will allow the OctreeServer to send your tree inbound edit packets of your
    // own definition. Implement these to allow your octree based server to support editing
    virtual bool handlesEditPacketType(PACKET_TYPE packetType) const;
    virtual int processEditPacketData(PACKET_TYPE packetType, unsigned char* packetData, int packetLength,
                    unsigned char* editData, int maxLength);

    void update();    
private:
    void storeParticle(const Particle& particle);

    static bool updateOperation(OctreeElement* element, void* extraData);



};

#endif /* defined(__hifi__ParticleTree__) */
