//
//  EntityScriptingInterface.cpp
//  libraries/models/src
//
//  Created by Brad Hefta-Gaub on 12/6/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "EntityScriptingInterface.h"
#include "EntityTree.h"

EntityScriptingInterface::EntityScriptingInterface() :
    _nextCreatorTokenID(0),
    _entityTree(NULL)
{
}


void EntityScriptingInterface::queueEntityMessage(PacketType packetType,
        EntityItemID entityID, const EntityItemProperties& properties) {
    getEntityPacketSender()->queueEntityEditMessage(packetType, entityID, properties);
}

EntityItemID EntityScriptingInterface::addEntity(const EntityItemProperties& properties) {

    // The application will keep track of creatorTokenID
    uint32_t creatorTokenID = EntityItem::getNextCreatorTokenID();

    EntityItemID id(NEW_MODEL, creatorTokenID, false );

    // queue the packet
    queueEntityMessage(PacketTypeEntityAddOrEdit, id, properties);

    // If we have a local model tree set, then also update it.
    if (_entityTree) {
        _entityTree->lockForWrite();
        _entityTree->addEntity(id, properties);
        _entityTree->unlock();
    }

    return id;
}

EntityItemID EntityScriptingInterface::identifyEntity(EntityItemID entityID) {
    uint32_t actualID = entityID.id;

    if (!entityID.isKnownID) {
        actualID = EntityItem::getIDfromCreatorTokenID(entityID.creatorTokenID);
        if (actualID == UNKNOWN_MODEL_ID) {
            return entityID; // bailing early
        }
        
        // found it!
        entityID.id = actualID;
        entityID.isKnownID = true;
    }
    return entityID;
}

EntityItemProperties EntityScriptingInterface::getEntityProperties(EntityItemID entityID) {
    EntityItemProperties results;
    EntityItemID identity = identifyEntity(entityID);
    if (!identity.isKnownID) {
        results.setIsUnknownID();
        return results;
    }
    if (_entityTree) {
        _entityTree->lockForRead();
        const EntityItem* model = _entityTree->findEntityByID(identity.id, true);
        if (model) {
            results.copyFromEntityItem(*model);
        } else {
            results.setIsUnknownID();
        }
        _entityTree->unlock();
    }
    
    return results;
}



EntityItemID EntityScriptingInterface::editEntity(EntityItemID entityID, const EntityItemProperties& properties) {
    uint32_t actualID = entityID.id;
    
    // if the model is unknown, attempt to look it up
    if (!entityID.isKnownID) {
        actualID = EntityItem::getIDfromCreatorTokenID(entityID.creatorTokenID);
    }

    // if at this point, we know the id, send the update to the model server
    if (actualID != UNKNOWN_MODEL_ID) {
        entityID.id = actualID;
        entityID.isKnownID = true;
        queueEntityMessage(PacketTypeEntityAddOrEdit, entityID, properties);
    }
    
    // If we have a local model tree set, then also update it. We can do this even if we don't know
    // the actual id, because we can edit out local models just with creatorTokenID
    if (_entityTree) {
        _entityTree->lockForWrite();
        _entityTree->updateEntity(entityID, properties);
        _entityTree->unlock();
    }
    return entityID;
}


// TODO: This deleteEntity() method uses the PacketType_MODEL_ADD_OR_EDIT message to send
// a changed model with a shouldDie() property set to true. This works and is currently the only
// way to tell the model server to delete a model. But we should change this to use the PacketType_MODEL_ERASE
// message which takes a list of model id's to delete.
void EntityScriptingInterface::deleteEntity(EntityItemID entityID) {

    // setup properties to kill the model
    EntityItemProperties properties;
    properties.setShouldBeDeleted(true);

    uint32_t actualID = entityID.id;
    
    // if the model is unknown, attempt to look it up
    if (!entityID.isKnownID) {
        actualID = EntityItem::getIDfromCreatorTokenID(entityID.creatorTokenID);
    }

    // if at this point, we know the id, send the update to the model server
    if (actualID != UNKNOWN_MODEL_ID) {
        entityID.id = actualID;
        entityID.isKnownID = true;
        queueEntityMessage(PacketTypeEntityAddOrEdit, entityID, properties);
    }

    // If we have a local model tree set, then also update it.
    if (_entityTree) {
        _entityTree->lockForWrite();
        _entityTree->deleteEntity(entityID);
        _entityTree->unlock();
    }
}

EntityItemID EntityScriptingInterface::findClosestEntity(const glm::vec3& center, float radius) const {
    EntityItemID result(UNKNOWN_MODEL_ID, UNKNOWN_MODEL_TOKEN, false);
    if (_entityTree) {
        _entityTree->lockForRead();
        const EntityItem* closestEntity = _entityTree->findClosestEntity(center/(float)TREE_SCALE, 
                                                                                radius/(float)TREE_SCALE);
        _entityTree->unlock();
        if (closestEntity) {
            result.id = closestEntity->getID();
            result.isKnownID = true;
        }
    }
    return result;
}


QVector<EntityItemID> EntityScriptingInterface::findEntities(const glm::vec3& center, float radius) const {
    QVector<EntityItemID> result;
    if (_entityTree) {
        _entityTree->lockForRead();
        QVector<const EntityItem*> models;
        _entityTree->findEntities(center/(float)TREE_SCALE, radius/(float)TREE_SCALE, models);
        _entityTree->unlock();

        foreach (const EntityItem* model, models) {
            EntityItemID thisEntityItemID(model->getID(), UNKNOWN_MODEL_TOKEN, true);
            result << thisEntityItemID;
        }
    }
    return result;
}

RayToEntityIntersectionResult EntityScriptingInterface::findRayIntersection(const PickRay& ray) {
    return findRayIntersectionWorker(ray, Octree::TryLock);
}

RayToEntityIntersectionResult EntityScriptingInterface::findRayIntersectionBlocking(const PickRay& ray) {
    return findRayIntersectionWorker(ray, Octree::Lock);
}

RayToEntityIntersectionResult EntityScriptingInterface::findRayIntersectionWorker(const PickRay& ray, 
                                                                                    Octree::lockType lockType) {
    RayToEntityIntersectionResult result;
    if (_entityTree) {
        OctreeElement* element;
        EntityItem* intersectedEntity;
        result.intersects = _entityTree->findRayIntersection(ray.origin, ray.direction, element, result.distance, result.face, 
                                                                (void**)&intersectedEntity, lockType, &result.accurate);
        if (result.intersects && intersectedEntity) {
            result.entityID = intersectedEntity->getEntityItemID();
            result.properties = intersectedEntity->getProperties();
            result.intersection = ray.origin + (ray.direction * result.distance);
        }
    }
    return result;
}


RayToEntityIntersectionResult::RayToEntityIntersectionResult() : 
    intersects(false), 
    accurate(true), // assume it's accurate
    entityID(),
    properties(),
    distance(0),
    face()
{ 
};

QScriptValue RayToEntityIntersectionResultToScriptValue(QScriptEngine* engine, const RayToEntityIntersectionResult& value) {
    QScriptValue obj = engine->newObject();
    obj.setProperty("intersects", value.intersects);
    obj.setProperty("accurate", value.accurate);
    QScriptValue modelItemValue = EntityItemIDtoScriptValue(engine, value.entityID);
    obj.setProperty("entityID", modelItemValue);

    QScriptValue modelPropertiesValue = EntityItemPropertiesToScriptValue(engine, value.properties);
    obj.setProperty("properties", modelPropertiesValue);

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

void RayToEntityIntersectionResultFromScriptValue(const QScriptValue& object, RayToEntityIntersectionResult& value) {
    value.intersects = object.property("intersects").toVariant().toBool();
    value.accurate = object.property("accurate").toVariant().toBool();
    QScriptValue modelIDValue = object.property("entityID");
    if (modelIDValue.isValid()) {
        EntityItemIDfromScriptValue(modelIDValue, value.entityID);
    }
    QScriptValue modelPropertiesValue = object.property("properties");
    if (modelPropertiesValue.isValid()) {
        EntityItemPropertiesFromScriptValue(modelPropertiesValue, value.properties);
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
