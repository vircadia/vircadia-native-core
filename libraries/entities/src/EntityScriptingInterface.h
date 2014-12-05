//
//  EntityScriptingInterface.h
//  libraries/models/src
//
//  Created by Brad Hefta-Gaub on 12/6/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
// TODO: How will we handle collision callbacks with Entities
//

#ifndef hifi_EntityScriptingInterface_h
#define hifi_EntityScriptingInterface_h

#include <QtCore/QObject>

#include <CollisionInfo.h>
#include <Octree.h>
#include <OctreeScriptingInterface.h>
#include <RegisteredMetaTypes.h>

#include "EntityEditPacketSender.h"


class EntityTree;
class MouseEvent;


class RayToEntityIntersectionResult {
public:
    RayToEntityIntersectionResult();
    bool intersects;
    bool accurate;
    EntityItemID entityID;
    EntityItemProperties properties;
    float distance;
    BoxFace face;
    glm::vec3 intersection;
    EntityItem* entity;
};

Q_DECLARE_METATYPE(RayToEntityIntersectionResult)

QScriptValue RayToEntityIntersectionResultToScriptValue(QScriptEngine* engine, const RayToEntityIntersectionResult& results);
void RayToEntityIntersectionResultFromScriptValue(const QScriptValue& object, RayToEntityIntersectionResult& results);


/// handles scripting of Entity commands from JS passed to assigned clients
class EntityScriptingInterface : public OctreeScriptingInterface {
    Q_OBJECT
public:
    EntityScriptingInterface();
    
    EntityEditPacketSender* getEntityPacketSender() const { return (EntityEditPacketSender*)getPacketSender(); }
    virtual NodeType_t getServerNodeType() const { return NodeType::EntityServer; }
    virtual OctreeEditPacketSender* createPacketSender() { return new EntityEditPacketSender(); }

    void setEntityTree(EntityTree* modelTree) { _entityTree = modelTree; }
    EntityTree* getEntityTree(EntityTree*) { return _entityTree; }
    
public slots:
    /// adds a model with the specific properties
    Q_INVOKABLE EntityItemID addEntity(const EntityItemProperties& properties);

    /// identify a recently created model to determine its true ID
    Q_INVOKABLE EntityItemID identifyEntity(EntityItemID entityID);

    /// gets the current model properties for a specific model
    /// this function will not find return results in script engine contexts which don't have access to models
    Q_INVOKABLE EntityItemProperties getEntityProperties(EntityItemID entityID);

    /// edits a model updating only the included properties, will return the identified EntityItemID in case of
    /// successful edit, if the input entityID is for an unknown model this function will have no effect
    Q_INVOKABLE EntityItemID editEntity(EntityItemID entityID, const EntityItemProperties& properties);

    /// deletes a model
    Q_INVOKABLE void deleteEntity(EntityItemID entityID);

    /// finds the closest model to the center point, within the radius
    /// will return a EntityItemID.isKnownID = false if no models are in the radius
    /// this function will not find any models in script engine contexts which don't have access to models
    Q_INVOKABLE EntityItemID findClosestEntity(const glm::vec3& center, float radius) const;

    /// finds models within the search sphere specified by the center point and radius
    /// this function will not find any models in script engine contexts which don't have access to models
    Q_INVOKABLE QVector<EntityItemID> findEntities(const glm::vec3& center, float radius) const;

    /// If the scripting context has visible voxels, this will determine a ray intersection, the results
    /// may be inaccurate if the engine is unable to access the visible voxels, in which case result.accurate
    /// will be false.
    Q_INVOKABLE RayToEntityIntersectionResult findRayIntersection(const PickRay& ray);

    /// If the scripting context has visible voxels, this will determine a ray intersection, and will block in
    /// order to return an accurate result
    Q_INVOKABLE RayToEntityIntersectionResult findRayIntersectionBlocking(const PickRay& ray);

    Q_INVOKABLE void setLightsArePickable(bool value);
    Q_INVOKABLE bool getLightsArePickable() const;

    Q_INVOKABLE void dumpTree() const;

signals:
    void entityCollisionWithVoxel(const EntityItemID& entityID, const VoxelDetail& voxel, const CollisionInfo& collision);
    void entityCollisionWithEntity(const EntityItemID& idA, const EntityItemID& idB, const CollisionInfo& collision);

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

private:
    void queueEntityMessage(PacketType packetType, EntityItemID entityID, const EntityItemProperties& properties);

    /// actually does the work of finding the ray intersection, can be called in locking mode or tryLock mode
    RayToEntityIntersectionResult findRayIntersectionWorker(const PickRay& ray, Octree::lockType lockType);

    uint32_t _nextCreatorTokenID;
    EntityTree* _entityTree;
};

#endif // hifi_EntityScriptingInterface_h
