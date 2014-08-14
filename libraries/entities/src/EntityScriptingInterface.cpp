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
    getEntityPacketSender()->queueEditEntityMessage(packetType, entityID, properties);
}

EntityItemID EntityScriptingInterface::addEntity(const EntityItemProperties& properties) {

    // The application will keep track of creatorTokenID
    uint32_t creatorTokenID = EntityItemID::getNextCreatorTokenID();

    EntityItemID id(NEW_ENTITY, creatorTokenID, false );

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
    EntityItemID actualID = entityID;

    if (!entityID.isKnownID) {
        actualID = EntityItemID::getIDfromCreatorTokenID(entityID.creatorTokenID);
        if (actualID == UNKNOWN_ENTITY_ID) {
            return entityID; // bailing early
        }
        
        // found it!
        entityID.id = actualID.id;
        entityID.isKnownID = true;
        bool wantDebug = false;
        if (wantDebug) {
            qDebug() << "EntityScriptingInterface::identifyEntity() ...found it... isKnownID=" << entityID.isKnownID 
                            << "id=" << entityID.id << "creatorTokenID=" << entityID.creatorTokenID;
        }
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
        EntityItem* entity = const_cast<EntityItem*>(_entityTree->findEntityByEntityItemID(identity));
        if (entity) {
        
            // TODO: look into sitting points!!!
            //entity->setSittingPoints(_entityTree->getGeometryForEntity(entity)->sittingPoints);
            
            results = entity->getProperties();
        } else {
            results.setIsUnknownID();
        }
        _entityTree->unlock();
    }
    
    return results;
}



EntityItemID EntityScriptingInterface::editEntity(EntityItemID entityID, const EntityItemProperties& properties) {
    EntityItemID actualID = entityID;
    
    // if the model is unknown, attempt to look it up
    if (!entityID.isKnownID) {
        actualID = EntityItemID::getIDfromCreatorTokenID(entityID.creatorTokenID);
    }

    // if at this point, we know the id, send the update to the model server
    if (actualID.id != UNKNOWN_ENTITY_ID) {
        entityID.id = actualID.id;
        entityID.isKnownID = true;
        //qDebug() << "EntityScriptingInterface::editEntity()... isKnownID=" << entityID.isKnownID << "id=" << entityID.id << "creatorTokenID=" << entityID.creatorTokenID;
        queueEntityMessage(PacketTypeEntityAddOrEdit, entityID, properties);
    }
    
    // If we have a local model tree set, then also update it. We can do this even if we don't know
    // the actual id, because we can edit out local models just with creatorTokenID
    if (_entityTree) {
        _entityTree->lockForWrite();
        //qDebug() << "EntityScriptingInterface::editEntity() ******* START _entityTree->updateEntity()...";
        _entityTree->updateEntity(entityID, properties);
        //qDebug() << "EntityScriptingInterface::editEntity() ******* DONE _entityTree->updateEntity()...";
        _entityTree->unlock();
    }
    return entityID;
}


// TODO: This deleteEntity() method uses the PacketTypeEntityAddOrEdit message to send
// a changed model with a shouldDie() property set to true. This works and is currently the only
// way to tell the model server to delete a model. But we should change this to use the PacketTypeEntityErase
// message which takes a list of model id's to delete.
void EntityScriptingInterface::deleteEntity(EntityItemID entityID) {

    EntityItemID actualID = entityID;
    
    // if the model is unknown, attempt to look it up
    if (!entityID.isKnownID) {
        actualID = EntityItemID::getIDfromCreatorTokenID(entityID.creatorTokenID);
    }

    // if at this point, we know the id, send the update to the model server
    if (actualID.id != UNKNOWN_ENTITY_ID) {
        entityID.id = actualID.id;
        entityID.isKnownID = true;

        bool wantDebug = false;
        if (wantDebug) {
            qDebug() << "EntityScriptingInterface::deleteEntity()... " 
                        << "isKnownID=" << entityID.isKnownID 
                        << "id=" << entityID.id 
                        << "creatorTokenID=" << entityID.creatorTokenID;
        }

        getEntityPacketSender()->queueEraseEntityMessage(entityID);
    }

    // If we have a local model tree set, then also update it.
    if (_entityTree) {
        _entityTree->lockForWrite();
        _entityTree->deleteEntity(entityID);
        _entityTree->unlock();
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
            bool wantDebug = false;
            if (wantDebug) {
                qDebug() << "EntityScriptingInterface::findClosestEntity()... isKnownID=" << result.isKnownID 
                            << "id=" << result.id << "creatorTokenID=" << result.creatorTokenID;
            }
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
        QVector<const EntityItem*> models;
        _entityTree->findEntities(center/(float)TREE_SCALE, radius/(float)TREE_SCALE, models);
        _entityTree->unlock();

        foreach (const EntityItem* model, models) {
            EntityItemID thisEntityItemID(model->getID(), UNKNOWN_ENTITY_TOKEN, true);
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

            //qDebug() << "findRayIntersectionWorker().... intersectedEntity=" << intersectedEntity;
            //intersectedEntity->debugDump();

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
