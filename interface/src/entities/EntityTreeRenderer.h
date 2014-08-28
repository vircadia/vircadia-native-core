//
//  EntityTreeRenderer.h
//  interface/src
//
//  Created by Brad Hefta-Gaub on 12/6/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_EntityTreeRenderer_h
#define hifi_EntityTreeRenderer_h

#include <glm/glm.hpp>
#include <stdint.h>

#include <EntityTree.h>
#include <Octree.h>
#include <OctreePacketData.h>
#include <OctreeRenderer.h>
#include <PacketHeaders.h>
#include <SharedUtil.h>
#include <ViewFrustum.h>

#include "renderer/Model.h"

// Generic client side Octree renderer class.
class EntityTreeRenderer : public OctreeRenderer, public EntityItemFBXService {
    Q_OBJECT
public:
    EntityTreeRenderer();
    virtual ~EntityTreeRenderer();

    virtual Octree* createTree() { return new EntityTree(true); }
    virtual char getMyNodeType() const { return NodeType::EntityServer; }
    virtual PacketType getMyQueryMessageType() const { return PacketTypeEntityQuery; }
    virtual PacketType getExpectedPacketType() const { return PacketTypeEntityData; }
    virtual void renderElement(OctreeElement* element, RenderArgs* args);
    virtual float getSizeScale() const;
    virtual int getBoundaryLevelAdjust() const;
    virtual void setTree(Octree* newTree);

    void update();

    EntityTree* getTree() { return (EntityTree*)_tree; }

    void processEraseMessage(const QByteArray& dataByteArray, const SharedNodePointer& sourceNode);

    virtual void init();
    virtual void render(RenderMode renderMode = DEFAULT_RENDER_MODE);

    virtual const FBXGeometry* getGeometryForEntity(const EntityItem* entityItem);
    virtual const Model* getModelForEntityItem(const EntityItem* entityItem);
    
    /// clears the tree
    virtual void clear();

    //Q_INVOKABLE Model* getModel(const ModelEntityItem* modelEntityItem);

    // renderers for various types of entities
    void renderEntityTypeBox(EntityItem* entity, RenderArgs* args);
    void renderEntityTypeModel(EntityItem* entity, RenderArgs* args);
    
    static QThread* getMainThread();
    
    /// if a renderable entity item needs a model, we will allocate it for them
    Q_INVOKABLE Model* allocateModel(const QString& url);
    
    /// if a renderable entity item needs to update the URL of a model, we will handle that for the entity
    Q_INVOKABLE Model* updateModel(Model* original, const QString& newUrl); 

    /// if a renderable entity item is done with a model, it should return it to us
    void releaseModel(Model* model);
    
    void deleteReleasedModels();
private:
    QList<Model*> _releasedModels;
};

#endif // hifi_EntityTreeRenderer_h
