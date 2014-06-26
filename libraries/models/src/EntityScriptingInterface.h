//
//  ModelsScriptingInterface.h
//  libraries/models/src
//
//  Created by Brad Hefta-Gaub on 12/6/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ModelsScriptingInterface_h
#define hifi_ModelsScriptingInterface_h

#include <QtCore/QObject>

#include <CollisionInfo.h>
#include <Octree.h>
#include <OctreeScriptingInterface.h>
#include <RegisteredMetaTypes.h>

#include "ModelEditPacketSender.h"

class RayToModelIntersectionResult {
public:
    RayToModelIntersectionResult();
    bool intersects;
    bool accurate;
    ModelItemID modelID;
    ModelItemProperties modelProperties;
    float distance;
    BoxFace face;
    glm::vec3 intersection;
};

Q_DECLARE_METATYPE(RayToModelIntersectionResult)

QScriptValue RayToModelIntersectionResultToScriptValue(QScriptEngine* engine, const RayToModelIntersectionResult& results);
void RayToModelIntersectionResultFromScriptValue(const QScriptValue& object, RayToModelIntersectionResult& results);


/// handles scripting of Model commands from JS passed to assigned clients
class ModelsScriptingInterface : public OctreeScriptingInterface {
    Q_OBJECT
public:
    ModelsScriptingInterface();
    
    ModelEditPacketSender* getModelPacketSender() const { return (ModelEditPacketSender*)getPacketSender(); }
    virtual NodeType_t getServerNodeType() const { return NodeType::ModelServer; }
    virtual OctreeEditPacketSender* createPacketSender() { return new ModelEditPacketSender(); }

    void setModelTree(ModelTree* modelTree) { _modelTree = modelTree; }
    ModelTree* getModelTree(ModelTree*) { return _modelTree; }
    
public slots:
    /// adds a model with the specific properties
    ModelItemID addModel(const ModelItemProperties& properties);

    /// identify a recently created model to determine its true ID
    ModelItemID identifyModel(ModelItemID modelID);

    /// gets the current model properties for a specific model
    /// this function will not find return results in script engine contexts which don't have access to models
    ModelItemProperties getModelProperties(ModelItemID modelID);

    /// edits a model updating only the included properties, will return the identified ModelItemID in case of
    /// successful edit, if the input modelID is for an unknown model this function will have no effect
    ModelItemID editModel(ModelItemID modelID, const ModelItemProperties& properties);

    /// deletes a model
    void deleteModel(ModelItemID modelID);

    /// finds the closest model to the center point, within the radius
    /// will return a ModelItemID.isKnownID = false if no models are in the radius
    /// this function will not find any models in script engine contexts which don't have access to models
    ModelItemID findClosestModel(const glm::vec3& center, float radius) const;

    /// finds models within the search sphere specified by the center point and radius
    /// this function will not find any models in script engine contexts which don't have access to models
    QVector<ModelItemID> findModels(const glm::vec3& center, float radius) const;

    /// If the scripting context has visible voxels, this will determine a ray intersection, the results
    /// may be inaccurate if the engine is unable to access the visible voxels, in which case result.accurate
    /// will be false.
    Q_INVOKABLE RayToModelIntersectionResult findRayIntersection(const PickRay& ray);

    /// If the scripting context has visible voxels, this will determine a ray intersection, and will block in
    /// order to return an accurate result
    Q_INVOKABLE RayToModelIntersectionResult findRayIntersectionBlocking(const PickRay& ray);


signals:
    void modelCollisionWithVoxel(const ModelItemID& modelID, const VoxelDetail& voxel, const CollisionInfo& collision);
    void modelCollisionWithModel(const ModelItemID& idA, const ModelItemID& idB, const CollisionInfo& collision);

private:
    void queueModelMessage(PacketType packetType, ModelItemID modelID, const ModelItemProperties& properties);

    /// actually does the work of finding the ray intersection, can be called in locking mode or tryLock mode
    RayToModelIntersectionResult findRayIntersectionWorker(const PickRay& ray, Octree::lockType lockType);

    uint32_t _nextCreatorTokenID;
    ModelTree* _modelTree;
};

#endif // hifi_ModelsScriptingInterface_h
