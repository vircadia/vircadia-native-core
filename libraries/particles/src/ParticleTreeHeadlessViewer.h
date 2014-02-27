//
//  ParticleTreeHeadlessViewer.h
//  hifi
//
//  Created by Brad Hefta-Gaub on 2/26/14
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//
//

#ifndef __hifi__ParticleTreeHeadlessViewer__
#define __hifi__ParticleTreeHeadlessViewer__

#include <PacketHeaders.h>
#include <SharedUtil.h>
#include <Octree.h>
#include <OctreePacketData.h>
#include <OctreeHeadlessViewer.h>
#include "ParticleTree.h"
#include <ViewFrustum.h>

// Generic client side Octree renderer class.
class ParticleTreeHeadlessViewer : public OctreeHeadlessViewer {
    Q_OBJECT
public:
    ParticleTreeHeadlessViewer();
    virtual ~ParticleTreeHeadlessViewer();

    virtual Octree* createTree() { return new ParticleTree(true); }
    virtual NodeType_t getMyNodeType() const { return NodeType::ParticleServer; }
    virtual PacketType getMyQueryMessageType() const { return PacketTypeParticleQuery; }
    virtual PacketType getExpectedPacketType() const { return PacketTypeParticleData; }

    void update();

    ParticleTree* getTree() { return (ParticleTree*)_tree; }

    void processEraseMessage(const QByteArray& dataByteArray, const SharedNodePointer& sourceNode);

    virtual void init();
};

#endif /* defined(__hifi__ParticleTreeHeadlessViewer__) */