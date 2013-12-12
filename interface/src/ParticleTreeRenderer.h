//
//  ParticleTreeRenderer.h
//  hifi
//
//  Created by Brad Hefta-Gaub on 12/6/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//

#ifndef __hifi__ParticleTreeRenderer__
#define __hifi__ParticleTreeRenderer__

#include <glm/glm.hpp>
#include <stdint.h>

#include <PacketHeaders.h>
#include <SharedUtil.h>
#include <Octree.h>
#include <OctreePacketData.h>
#include <OctreeRenderer.h>
#include <ParticleTree.h>
#include <ViewFrustum.h>

// Generic client side Octree renderer class.
class ParticleTreeRenderer : public OctreeRenderer {
public:
    ParticleTreeRenderer();
    virtual ~ParticleTreeRenderer();

    virtual Octree* createTree() { return new ParticleTree(true); }
    virtual NODE_TYPE getMyNodeType() const { return NODE_TYPE_PARTICLE_SERVER; }
    virtual PACKET_TYPE getMyQueryMessageType() const { return PACKET_TYPE_PARTICLE_QUERY; }
    virtual PACKET_TYPE getExpectedPacketType() const { return PACKET_TYPE_PARTICLE_DATA; }
    virtual void renderElement(OctreeElement* element, RenderArgs* args);

    void update();

    ParticleTree* getTree() { return (ParticleTree*)_tree; }

protected:
};

#endif /* defined(__hifi__ParticleTreeRenderer__) */