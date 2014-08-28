//
//  ModelsScriptingInterface.cpp
//  libraries/models/src
//
//  Created by Brad Hefta-Gaub on 12/6/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ModelsScriptingInterface.h"
#include "ModelTree.h"

ModelsScriptingInterface::ModelsScriptingInterface() :
    _nextCreatorTokenID(0),
    _modelTree(NULL)
{
}


void ModelsScriptingInterface::queueModelMessage(PacketType packetType,
        ModelItemID modelID, const ModelItemProperties& properties) {
    getModelPacketSender()->queueModelEditMessage(packetType, modelID, properties);
}

ModelItemID ModelsScriptingInterface::addModel(const ModelItemProperties& properties) {

    // The application will keep track of creatorTokenID
    uint32_t creatorTokenID = ModelItem::getNextCreatorTokenID();

    ModelItemID id(NEW_MODEL, creatorTokenID, false );

    // queue the packet
    queueModelMessage(PacketTypeModelAddOrEdit, id, properties);

    // If we have a local model tree set, then also update it.
    if (_modelTree) {
        _modelTree->lockForWrite();
        _modelTree->addModel(id, properties);
        _modelTree->unlock();
    }

    return id;
}

ModelItemID ModelsScriptingInterface::identifyModel(ModelItemID modelID) {
    uint32_t actualID = modelID.id;

    if (!modelID.isKnownID) {
        actualID = ModelItem::getIDfromCreatorTokenID(modelID.creatorTokenID);
        if (actualID == UNKNOWN_MODEL_ID) {
            return modelID; // bailing early
        }
        
        // found it!
        modelID.id = actualID;
        modelID.isKnownID = true;
    }
    return modelID;
}

ModelItemProperties ModelsScriptingInterface::getModelProperties(ModelItemID modelID) {
    ModelItemProperties results;
    ModelItemID identity = identifyModel(modelID);
    if (!identity.isKnownID) {
        results.setIsUnknownID();
        return results;
    }
    if (_modelTree) {
        _modelTree->lockForRead();
        ModelItem* model = const_cast<ModelItem*>(_modelTree->findModelByID(identity.id, true));
        if (model && _modelTree->getGeometryForModel(*model)) {
            model->setSittingPoints(_modelTree->getGeometryForModel(*model)->sittingPoints);
            results.copyFromModelItem(*model);
        } else {
            results.setIsUnknownID();
        }
        _modelTree->unlock();
    }
    
    return results;
}



ModelItemID ModelsScriptingInterface::editModel(ModelItemID modelID, const ModelItemProperties& properties) {
    uint32_t actualID = modelID.id;
    
    // if the model is unknown, attempt to look it up
    if (!modelID.isKnownID) {
        actualID = ModelItem::getIDfromCreatorTokenID(modelID.creatorTokenID);
    }

    // if at this point, we know the id, send the update to the model server
    if (actualID != UNKNOWN_MODEL_ID) {
        modelID.id = actualID;
        modelID.isKnownID = true;
        queueModelMessage(PacketTypeModelAddOrEdit, modelID, properties);
    }
    
    // If we have a local model tree set, then also update it. We can do this even if we don't know
    // the actual id, because we can edit out local models just with creatorTokenID
    if (_modelTree) {
        _modelTree->lockForWrite();
        _modelTree->updateModel(modelID, properties);
        _modelTree->unlock();
    }
    return modelID;
}


// TODO: This deleteModel() method uses the PacketType_MODEL_ADD_OR_EDIT message to send
// a changed model with a shouldDie() property set to true. This works and is currently the only
// way to tell the model server to delete a model. But we should change this to use the PacketType_MODEL_ERASE
// message which takes a list of model id's to delete.
void ModelsScriptingInterface::deleteModel(ModelItemID modelID) {

    // setup properties to kill the model
    ModelItemProperties properties;
    properties.setShouldDie(true);

    uint32_t actualID = modelID.id;
    
    // if the model is unknown, attempt to look it up
    if (!modelID.isKnownID) {
        actualID = ModelItem::getIDfromCreatorTokenID(modelID.creatorTokenID);
    }

    // if at this point, we know the id, send the update to the model server
    if (actualID != UNKNOWN_MODEL_ID) {
        modelID.id = actualID;
        modelID.isKnownID = true;
        queueModelMessage(PacketTypeModelAddOrEdit, modelID, properties);
    }

    // If we have a local model tree set, then also update it.
    if (_modelTree) {
        _modelTree->lockForWrite();
        _modelTree->deleteModel(modelID);
        _modelTree->unlock();
    }
}

ModelItemID ModelsScriptingInterface::findClosestModel(const glm::vec3& center, float radius) const {
    ModelItemID result(UNKNOWN_MODEL_ID, UNKNOWN_MODEL_TOKEN, false);
    if (_modelTree) {
        _modelTree->lockForRead();
        const ModelItem* closestModel = _modelTree->findClosestModel(center/(float)TREE_SCALE, 
                                                                                radius/(float)TREE_SCALE);
        _modelTree->unlock();
        if (closestModel) {
            result.id = closestModel->getID();
            result.isKnownID = true;
        }
    }
    return result;
}


QVector<ModelItemID> ModelsScriptingInterface::findModels(const glm::vec3& center, float radius) const {
    QVector<ModelItemID> result;
    if (_modelTree) {
        QVector<const ModelItem*> models;
        _modelTree->findModels(center/(float)TREE_SCALE, radius/(float)TREE_SCALE, models);

        foreach (const ModelItem* model, models) {
            ModelItemID thisModelItemID(model->getID(), UNKNOWN_MODEL_TOKEN, true);
            result << thisModelItemID;
        }
    }
    return result;
}

RayToModelIntersectionResult ModelsScriptingInterface::findRayIntersection(const PickRay& ray) {
    return findRayIntersectionWorker(ray, Octree::TryLock);
}

RayToModelIntersectionResult ModelsScriptingInterface::findRayIntersectionBlocking(const PickRay& ray) {
    return findRayIntersectionWorker(ray, Octree::Lock);
}

RayToModelIntersectionResult ModelsScriptingInterface::findRayIntersectionWorker(const PickRay& ray, 
                                                                                    Octree::lockType lockType) {
    RayToModelIntersectionResult result;
    if (_modelTree) {
        OctreeElement* element;
        ModelItem* intersectedModel;
        result.intersects = _modelTree->findRayIntersection(ray.origin, ray.direction, element, result.distance, result.face, 
                                                                (void**)&intersectedModel, lockType, &result.accurate);
        if (result.intersects && intersectedModel) {
            result.modelID = intersectedModel->getModelItemID();
            result.modelProperties = intersectedModel->getProperties();
            result.intersection = ray.origin + (ray.direction * result.distance);
        }
    }
    return result;
}


RayToModelIntersectionResult::RayToModelIntersectionResult() : 
    intersects(false), 
    accurate(true), // assume it's accurate
    modelID(),
    modelProperties(),
    distance(0),
    face()
{ 
};

QScriptValue RayToModelIntersectionResultToScriptValue(QScriptEngine* engine, const RayToModelIntersectionResult& value) {
    QScriptValue obj = engine->newObject();
    obj.setProperty("intersects", value.intersects);
    obj.setProperty("accurate", value.accurate);
    QScriptValue modelItemValue = ModelItemIDtoScriptValue(engine, value.modelID);
    obj.setProperty("modelID", modelItemValue);

    QScriptValue modelPropertiesValue = ModelItemPropertiesToScriptValue(engine, value.modelProperties);
    obj.setProperty("modelProperties", modelPropertiesValue);

    obj.setProperty("distance", value.distance);

    QString faceName = "";    
    // handle BoxFace
    switch (value.face) {
        case MIN_X_FACE:
            faceName = "MIN_X_FACE";
            break;
        case MAX_X_FACE:
            faceName = "MAX_X_FACE";
            break;
        case MIN_Y_FACE:
            faceName = "MIN_Y_FACE";
            break;
        case MAX_Y_FACE:
            faceName = "MAX_Y_FACE";
            break;
        case MIN_Z_FACE:
            faceName = "MIN_Z_FACE";
            break;
        case MAX_Z_FACE:
            faceName = "MAX_Z_FACE";
            break;
        case UNKNOWN_FACE:
            faceName = "UNKNOWN_FACE";
            break;
    }
    obj.setProperty("face", faceName);

    QScriptValue intersection = vec3toScriptValue(engine, value.intersection);
    obj.setProperty("intersection", intersection);
    return obj;
}

void RayToModelIntersectionResultFromScriptValue(const QScriptValue& object, RayToModelIntersectionResult& value) {
    value.intersects = object.property("intersects").toVariant().toBool();
    value.accurate = object.property("accurate").toVariant().toBool();
    QScriptValue modelIDValue = object.property("modelID");
    if (modelIDValue.isValid()) {
        ModelItemIDfromScriptValue(modelIDValue, value.modelID);
    }
    QScriptValue modelPropertiesValue = object.property("modelProperties");
    if (modelPropertiesValue.isValid()) {
        ModelItemPropertiesFromScriptValue(modelPropertiesValue, value.modelProperties);
    }
    value.distance = object.property("distance").toVariant().toFloat();

    QString faceName = object.property("face").toVariant().toString();
    if (faceName == "MIN_X_FACE") {
        value.face = MIN_X_FACE;
    } else if (faceName == "MAX_X_FACE") {
        value.face = MAX_X_FACE;
    } else if (faceName == "MIN_Y_FACE") {
        value.face = MIN_Y_FACE;
    } else if (faceName == "MAX_Y_FACE") {
        value.face = MAX_Y_FACE;
    } else if (faceName == "MIN_Z_FACE") {
        value.face = MIN_Z_FACE;
    } else {
        value.face = MAX_Z_FACE;
    };
    QScriptValue intersection = object.property("intersection");
    if (intersection.isValid()) {
        vec3FromScriptValue(intersection, value.intersection);
    }
}
