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
#include <EntityScriptingInterface.h> // for RayToEntityIntersectionResult
#include <Octree.h>
#include <OctreePacketData.h>
#include <OctreeRenderer.h>
#include <PacketHeaders.h>
#include <RenderArgs.h>
#include <SharedUtil.h>
#include <ViewFrustum.h>

#include "renderer/Model.h"

// Generic client side Octree renderer class.
class EntityTreeRenderer : public OctreeRenderer, public EntityItemFBXService {
    Q_OBJECT
public:
    EntityTreeRenderer(bool wantScripts);
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

    EntityTree* getTree() { return static_cast<EntityTree*>(_tree); }

    void processEraseMessage(const QByteArray& dataByteArray, const SharedNodePointer& sourceNode);

    virtual void init();
    virtual void render(RenderArgs::RenderMode renderMode = RenderArgs::DEFAULT_RENDER_MODE);

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
    
    // event handles which may generate entity related events
    void mouseReleaseEvent(QMouseEvent* event, unsigned int deviceID);
    void mousePressEvent(QMouseEvent* event, unsigned int deviceID);
    void mouseMoveEvent(QMouseEvent* event, unsigned int deviceID);

    /// connect our signals to anEntityScriptingInterface for firing of events related clicking,
    /// hovering over, and entering entities
    void connectSignalsToSlots(EntityScriptingInterface* entityScriptingInterface);

signals:
    void mousePressOnEntity(const EntityItemID& entityItemID, const MouseEvent& event);
    void mouseMoveOnEntity(const EntityItemID& entityItemID, const MouseEvent& event);
    void mouseReleaseOnEntity(const EntityItemID& entityItemID, const MouseEvent& event);

    void clickDownOnEntity(const EntityItemID& entityItemID, const MouseEvent& event);
    void holdingClickOnEntity(const EntityItemID& entityItemID, const MouseEvent& event);
    void clickReleaseOnEntity(const EntityItemID& entityItemID, const MouseEvent& event);

    void hoverEnterEntity(const EntityItemID& entityItemID, const MouseEvent& event);
    void hoverOverEntity(const EntityItemID& entityItemID, const MouseEvent& event);
    void hoverLeaveEntity(const EntityItemID& entityItemID, const MouseEvent& event);
    
private:
    QList<Model*> _releasedModels;
    void renderProxies(const EntityItem* entity, RenderArgs* args);
    PickRay computePickRay(float x, float y);
    RayToEntityIntersectionResult findRayIntersectionWorker(const PickRay& ray, Octree::lockType lockType);

    EntityItemID _currentHoverOverEntityID;
    EntityItemID _currentClickingOnEntityID;
    
    bool _wantScripts;
    ScriptEngine* _entitiesScriptEngine;

    QScriptValue loadEntityScript(EntityItem* entity);
    QScriptValue loadEntityScript(const EntityItemID& entityItemID);
    QScriptValueList createMouseEventArgs(const EntityItemID& entityID, QMouseEvent* event, unsigned int deviceID);
    
    QHash<EntityItemID, QScriptValue> _entityScripts;
};

#endif // hifi_EntityTreeRenderer_h
