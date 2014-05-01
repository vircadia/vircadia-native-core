//
//  ModelTreeRenderer.h
//  interface/src
//
//  Created by Brad Hefta-Gaub on 12/6/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ModelTreeRenderer_h
#define hifi_ModelTreeRenderer_h

#include <glm/glm.hpp>
#include <stdint.h>

#include <ModelTree.h>
#include <Octree.h>
#include <OctreePacketData.h>
#include <OctreeRenderer.h>
#include <PacketHeaders.h>
#include <SharedUtil.h>
#include <ViewFrustum.h>

#include "renderer/Model.h"

// Generic client side Octree renderer class.
class ModelTreeRenderer : public OctreeRenderer {
public:
    ModelTreeRenderer();
    virtual ~ModelTreeRenderer();

    virtual Octree* createTree() { return new ModelTree(true); }
    virtual NodeType_t getMyNodeType() const { return NodeType::ModelServer; }
    virtual PacketType getMyQueryMessageType() const { return PacketTypeModelQuery; }
    virtual PacketType getExpectedPacketType() const { return PacketTypeModelData; }
    virtual void renderElement(OctreeElement* element, RenderArgs* args);

    void update();

    ModelTree* getTree() { return (ModelTree*)_tree; }

    void processEraseMessage(const QByteArray& dataByteArray, const SharedNodePointer& sourceNode);

    virtual void init();
    virtual void render();

protected:
    Model* getModel(const QString& url);

    QMap<QString, Model*> _modelsItemModels;
};

#endif // hifi_ModelTreeRenderer_h
