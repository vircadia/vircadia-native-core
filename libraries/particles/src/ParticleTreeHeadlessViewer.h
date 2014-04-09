//
//  ParticleTreeHeadlessViewer.h
//  libraries/particles/src
//
//  Created by Brad Hefta-Gaub on 2/26/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ParticleTreeHeadlessViewer_h
#define hifi_ParticleTreeHeadlessViewer_h

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

#endif // hifi_ParticleTreeHeadlessViewer_h
