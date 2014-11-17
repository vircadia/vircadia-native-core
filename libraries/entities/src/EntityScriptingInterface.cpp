//
//  EntityScriptingInterface.cpp
//  libraries/entities/src
//
//  Created by Brad Hefta-Gaub on 12/6/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "EntityScriptingInterface.h"
#include "EntityTree.h"
#include "ModelEntityItem.h"

EntityScriptingInterface::EntityScriptingInterface() :
    _nextCreatorTokenID(0),
    _entityTree(NULL)
{
}

void EntityScriptingInterface::queueEntityMessage(PacketType packetType,
        EntityItemID entityID, const EntityItemProperties& properties) {
    getEntityPacketSender()->queueEditEntityMessage(packetType, entityID, properties);
}

EntityItemID EntityScriptingInterface::addEntity(const EntityItemProperties& properties) {

    // The application will keep track of creatorTokenID
    uint32_t creatorTokenID = EntityItemID::getNextCreatorTokenID();

    EntityItemID id(NEW_ENTITY, creatorTokenID, false );

    // If we have a local entity tree set, then also update it.
    if (_entityTree) {
        _entityTree->lockForWrite();
        _entityTree->addEntity(id, properties);
        _entityTree->unlock();
    }

    // queue the packet
    queueEntityMessage(PacketTypeEntityAddOrEdit, id, properties);

    return id;
}

EntityItemID EntityScriptingInterface::identifyEntity(EntityItemID entityID) {
    EntityItemID actualID = entityID;

    if (!entityID.isKnownID) {
        actualID = EntityItemID::getIDfromCreatorTokenID(entityID.creatorTokenID);
        if (actualID == UNKNOWN_ENTITY_ID) {
            return entityID; // bailing early
        }
        
        // found it!
        entityID.id = actualID.id;
        entityID.isKnownID = true;
    }
    return entityID;
}

EntityItemProperties EntityScriptingInterface::getEntityProperties(EntityItemID entityID) {
    EntityItemProperties results;
    EntityItemID identity = identifyEntity(entityID);
    if (_entityTree) {
        _entityTree->lockForRead();
        EntityItem* entity = const_cast<EntityItem*>(_entityTree->findEntityByEntityItemID(identity));
        
        if (entity) {
            results = entity->getProperties();

            // TODO: improve sitting points and naturalDimensions in the future, 
            //       for now we've included the old sitting points model behavior for entity types that are models
            //        we've also added this hack for setting natural dimensions of models
            if (entity->getType() == EntityTypes::Model) {
                const FBXGeometry* geometry = _entityTree->getGeometryForEntity(entity);
                if (geometry) {
                    results.setSittingPoints(geometry->sittingPoints);
                    Extents meshExtents = geometry->getUnscaledMeshExtents();
                    results.setNaturalDimensions(meshExtents.maximum - meshExtents.minimum);
                }
            }

        } else {
            results.setIsUnknownID();
        }
        _entityTree->unlock();
    }
    
    return results;
}

EntityItemID EntityScriptingInterface::editEntity(EntityItemID entityID, const EntityItemProperties& properties) {
    EntityItemID actualID = entityID;
    // if the entity is unknown, attempt to look it up
    if (!entityID.isKnownID) {
        actualID = EntityItemID::getIDfromCreatorTokenID(entityID.creatorTokenID);
        if (actualID.id != UNKNOWN_ENTITY_ID) {
            entityID.id = actualID.id;
            entityID.isKnownID = true;
        }
    }
    
    // If we have a local entity tree set, then also update it. We can do this even if we don't know
    // the actual id, because we can edit out local entities just with creatorTokenID
    if (_entityTree) {
        _entityTree->lockForWrite();
        _entityTree->updateEntity(entityID, properties);
        _entityTree->unlock();
    }

    // if at this point, we know the id, send the update to the entity server
    if (entityID.isKnownID) {
        // make sure the properties has a type, so that the encode can know which properties to include
        if (properties.getType() == EntityTypes::Unknown) {
            EntityItem* entity = _entityTree->findEntityByEntityItemID(entityID);
            if (entity) {
                EntityItemProperties tempProperties = properties;
                tempProperties.setType(entity->getType());
                queueEntityMessage(PacketTypeEntityAddOrEdit, entityID, tempProperties);
                return entityID;
            }
        }
        
        // if the properties already includes the type, then use it as is
        queueEntityMessage(PacketTypeEntityAddOrEdit, entityID, properties);
    }
    
    return entityID;
}

void EntityScriptingInterface::deleteEntity(EntityItemID entityID) {

    EntityItemID actualID = entityID;
    
    // if the entity is unknown, attempt to look it up
    if (!entityID.isKnownID) {
        actualID = EntityItemID::getIDfromCreatorTokenID(entityID.creatorTokenID);
        if (actualID.id != UNKNOWN_ENTITY_ID) {
            entityID.id = actualID.id;
            entityID.isKnownID = true;
        }
    }

    // If we have a local entity tree set, then also update it.
    if (_entityTree) {
        _entityTree->lockForWrite();
        _entityTree->deleteEntity(entityID);
        _entityTree->unlock();
    }

    // if at this point, we know the id, send the update to the entity server
    if (entityID.isKnownID) {
        getEntityPacketSender()->queueEraseEntityMessage(entityID);
    }
}

EntityItemID EntityScriptingInterface::findClosestEntity(const glm::vec3& center, float radius) const {
    EntityItemID result(UNKNOWN_ENTITY_ID, UNKNOWN_ENTITY_TOKEN, false);
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


void EntityScriptingInterface::dumpTree() const {
    if (_entityTree) {
        _entityTree->lockForRead();
        _entityTree->dumpTree();
        _entityTree->unlock();
    }
}

QVector<EntityItemID> EntityScriptingInterface::findEntities(const glm::vec3& center, float radius) const {
    QVector<EntityItemID> result;
    if (_entityTree) {
        _entityTree->lockForRead();
        QVector<const EntityItem*> entities;
        _entityTree->findEntities(center/(float)TREE_SCALE, radius/(float)TREE_SCALE, entities);
        _entityTree->unlock();

        foreach (const EntityItem* entity, entities) {
            EntityItemID thisEntityItemID(entity->getID(), UNKNOWN_ENTITY_TOKEN, true);
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
        EntityItem* intersectedEntity = NULL;
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
    face(),
    entity(NULL)
{ 
}

QScriptValue RayToEntityIntersectionResultToScriptValue(QScriptEngine* engine, const RayToEntityIntersectionResult& value) {
    QScriptValue obj = engine->newObject();
    obj.setProperty("intersects", value.intersects);
    obj.setProperty("accurate", value.accurate);
    QScriptValue entityItemValue = EntityItemIDtoScriptValue(engine, value.entityID);
    obj.setProperty("entityID", entityItemValue);

    QScriptValue propertiesValue = EntityItemPropertiesToScriptValue(engine, value.properties);
    obj.setProperty("properties", propertiesValue);

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
    QScriptValue entityIDValue = object.property("entityID");
    if (entityIDValue.isValid()) {
        EntityItemIDfromScriptValue(entityIDValue, value.entityID);
    }
    QScriptValue entityPropertiesValue = object.property("properties");
    if (entityPropertiesValue.isValid()) {
        EntityItemPropertiesFromScriptValue(entityPropertiesValue, value.properties);
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
