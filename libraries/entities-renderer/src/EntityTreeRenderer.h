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

#include <EntityTree.h>
#include <EntityScriptingInterface.h> // for RayToEntityIntersectionResult
#include <MouseEvent.h>
#include <OctreeRenderer.h>

class Model;
class ScriptEngine;
class AbstractViewStateInterface;
class AbstractScriptingServicesInterface;

class ScriptEngine;

class EntityScriptDetails {
public:
    QString scriptText;
    QScriptValue scriptObject;
};

// Generic client side Octree renderer class.
class EntityTreeRenderer : public OctreeRenderer, public EntityItemFBXService {
    Q_OBJECT
public:
    EntityTreeRenderer(bool wantScripts, AbstractViewStateInterface* viewState, 
                                AbstractScriptingServicesInterface* scriptingServices);
    virtual ~EntityTreeRenderer();

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
    virtual void render(RenderArgs::RenderMode renderMode = RenderArgs::DEFAULT_RENDER_MODE, 
                                        RenderArgs::RenderSide renderSide = RenderArgs::MONO);

    virtual const FBXGeometry* getGeometryForEntity(const EntityItem* entityItem);
    virtual const Model* getModelForEntityItem(const EntityItem* entityItem);
    
    /// clears the tree
    virtual void clear();

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

    void enterEntity(const EntityItemID& entityItemID);
    void leaveEntity(const EntityItemID& entityItemID);

public slots:
    void deletingEntity(const EntityItemID& entityID);
    void changingEntityID(const EntityItemID& oldEntityID, const EntityItemID& newEntityID);
    void entitySciptChanging(const EntityItemID& entityID);
    void entityCollisionWithVoxel(const EntityItemID& entityID, const VoxelDetail& voxel, const Collision& collision);
    void entityCollisionWithEntity(const EntityItemID& idA, const EntityItemID& idB, const Collision& collision);

    // optional slots that can be wired to menu items
    void setDisplayElementChildProxies(bool value) { _displayElementChildProxies = value; }
    void setDisplayModelBounds(bool value) { _displayModelBounds = value; }
    void setDisplayModelElementProxy(bool value) { _displayModelElementProxy = value; }
    void setDontDoPrecisionPicking(bool value) { _dontDoPrecisionPicking = value; }
    
protected:
    virtual Octree* createTree() { return new EntityTree(true); }

private:
    void renderElementProxy(EntityTreeElement* entityTreeElement);
    void checkAndCallPreload(const EntityItemID& entityID);
    void checkAndCallUnload(const EntityItemID& entityID);

    QList<Model*> _releasedModels;
    void renderProxies(const EntityItem* entity, RenderArgs* args);
    RayToEntityIntersectionResult findRayIntersectionWorker(const PickRay& ray, Octree::lockType lockType, 
                                                                bool precisionPicking);

    EntityItemID _currentHoverOverEntityID;
    EntityItemID _currentClickingOnEntityID;

    QScriptValueList createEntityArgs(const EntityItemID& entityID);
    void checkEnterLeaveEntities();
    void leaveAllEntities();
    glm::vec3 _lastAvatarPosition;
    QVector<EntityItemID> _currentEntitiesInside;
    
    bool _wantScripts;
    ScriptEngine* _entitiesScriptEngine;

    QScriptValue loadEntityScript(EntityItem* entity);
    QScriptValue loadEntityScript(const EntityItemID& entityItemID);
    QScriptValue getPreviouslyLoadedEntityScript(const EntityItemID& entityItemID);
    QString loadScriptContents(const QString& scriptMaybeURLorText, bool& isURL);
    QScriptValueList createMouseEventArgs(const EntityItemID& entityID, QMouseEvent* event, unsigned int deviceID);
    QScriptValueList createMouseEventArgs(const EntityItemID& entityID, const MouseEvent& mouseEvent);
    
    QHash<EntityItemID, EntityScriptDetails> _entityScripts;

    bool _lastMouseEventValid;
    MouseEvent _lastMouseEvent;
    AbstractViewStateInterface* _viewState;
    AbstractScriptingServicesInterface* _scriptingServices;
    bool _displayElementChildProxies;
    bool _displayModelBounds;
    bool _displayModelElementProxy;
    bool _dontDoPrecisionPicking;
    
};

#endif // hifi_EntityTreeRenderer_h
