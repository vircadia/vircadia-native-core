//
//  ParticleTreeRenderer.h
//  interface/src
//
//  Created by Brad Hefta-Gaub on 12/6/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
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
#include "renderer/Model.h"

// Generic client side Octree renderer class.
class ParticleTreeRenderer : public OctreeRenderer {
public:
    ParticleTreeRenderer();
    virtual ~ParticleTreeRenderer();

    virtual Octree* createTree() { return new ParticleTree(true); }
    virtual NodeType_t getMyNodeType() const { return NodeType::ParticleServer; }
    virtual PacketType getMyQueryMessageType() const { return PacketTypeParticleQuery; }
    virtual PacketType getExpectedPacketType() const { return PacketTypeParticleData; }
    virtual void renderElement(OctreeElement* element, RenderArgs* args);

    void update();

    ParticleTree* getTree() { return (ParticleTree*)_tree; }

    void processEraseMessage(const QByteArray& dataByteArray, const SharedNodePointer& sourceNode);

    virtual void init();
    virtual void render();

protected:
    Model* getModel(const QString& url);

    QMap<QString, Model*> _particleModels;
};

#endif /* defined(__hifi__ParticleTreeRenderer__) */
