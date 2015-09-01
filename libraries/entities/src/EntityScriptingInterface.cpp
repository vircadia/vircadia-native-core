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

#include <VariantMapToScriptValue.h>

#include "EntitiesLogging.h"
#include "EntityActionFactoryInterface.h"
#include "EntityActionInterface.h"
#include "EntitySimulation.h"
#include "EntityTree.h"
#include "LightEntityItem.h"
#include "ModelEntityItem.h"
#include "SimulationOwner.h"
#include "ZoneEntityItem.h"


EntityScriptingInterface::EntityScriptingInterface() :
    _entityTree(NULL)
{
    auto nodeList = DependencyManager::get<NodeList>();
    connect(nodeList.data(), &NodeList::canAdjustLocksChanged, this, &EntityScriptingInterface::canAdjustLocksChanged);
    connect(nodeList.data(), &NodeList::canRezChanged, this, &EntityScriptingInterface::canRezChanged);
}

void EntityScriptingInterface::queueEntityMessage(PacketType packetType,
                                                  EntityItemID entityID, const EntityItemProperties& properties) {
    getEntityPacketSender()->queueEditEntityMessage(packetType, entityID, properties);
}

bool EntityScriptingInterface::canAdjustLocks() {
    auto nodeList = DependencyManager::get<NodeList>();
    return nodeList->getThisNodeCanAdjustLocks();
}

bool EntityScriptingInterface::canRez() {
    auto nodeList = DependencyManager::get<NodeList>();
    return nodeList->getThisNodeCanRez();
}

void EntityScriptingInterface::setEntityTree(EntityTree* modelTree) {
    if (_entityTree) {
        disconnect(_entityTree, &EntityTree::addingEntity, this, &EntityScriptingInterface::addingEntity);
        disconnect(_entityTree, &EntityTree::deletingEntity, this, &EntityScriptingInterface::deletingEntity);
        disconnect(_entityTree, &EntityTree::clearingEntities, this, &EntityScriptingInterface::clearingEntities);
    }

    _entityTree = modelTree;

    if (_entityTree) {
        connect(_entityTree, &EntityTree::addingEntity, this, &EntityScriptingInterface::addingEntity);
        connect(_entityTree, &EntityTree::deletingEntity, this, &EntityScriptingInterface::deletingEntity);
        connect(_entityTree, &EntityTree::clearingEntities, this, &EntityScriptingInterface::clearingEntities);
    }
}

QUuid EntityScriptingInterface::addEntity(const EntityItemProperties& properties) {

    EntityItemProperties propertiesWithSimID = properties;

    EntityItemID id = EntityItemID(QUuid::createUuid());

    // If we have a local entity tree set, then also update it.
    bool success = true;
    if (_entityTree) {
        _entityTree->lockForWrite();
        EntityItemPointer entity = _entityTree->addEntity(id, propertiesWithSimID);
        if (entity) {
            // This Node is creating a new object.  If it's in motion, set this Node as the simulator.
            auto nodeList = DependencyManager::get<NodeList>();
            const QUuid myNodeID = nodeList->getSessionUUID();
            propertiesWithSimID.setSimulationOwner(myNodeID, SCRIPT_EDIT_SIMULATION_PRIORITY);

            // and make note of it now, so we can act on it right away.
            entity->setSimulationOwner(myNodeID, SCRIPT_EDIT_SIMULATION_PRIORITY);

            entity->setLastBroadcast(usecTimestampNow());
        } else {
            qCDebug(entities) << "script failed to add new Entity to local Octree";
            success = false;
        }
        _entityTree->unlock();
    }

    // queue the packet
    if (success) {
        queueEntityMessage(PacketType::EntityAdd, id, propertiesWithSimID);
    }

    return id;
}

EntityItemProperties EntityScriptingInterface::getEntityProperties(QUuid identity) {
    EntityItemProperties results;
    if (_entityTree) {
        _entityTree->lockForRead();

        EntityItemPointer entity = _entityTree->findEntityByEntityItemID(EntityItemID(identity));

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
                    results.calculateNaturalPosition(meshExtents.minimum, meshExtents.maximum);
                }
            }

        }
        _entityTree->unlock();
    }

    return results;
}

QUuid EntityScriptingInterface::editEntity(QUuid id, EntityItemProperties properties) {
    EntityItemID entityID(id);
    // If we have a local entity tree set, then also update it.
    if (_entityTree) {
        _entityTree->lockForWrite();
        bool updatedEntity = _entityTree->updateEntity(entityID, properties);
        _entityTree->unlock();

        if (updatedEntity) {
            _entityTree->lockForRead();
            EntityItemPointer entity = _entityTree->findEntityByEntityItemID(entityID);
            if (entity) {
                // make sure the properties has a type, so that the encode can know which properties to include
                properties.setType(entity->getType());
                bool hasTerseUpdateChanges = properties.hasTerseUpdateChanges();
                bool hasPhysicsChanges = properties.hasMiscPhysicsChanges() || hasTerseUpdateChanges;
                if (hasPhysicsChanges) {
                    auto nodeList = DependencyManager::get<NodeList>();
                    const QUuid myNodeID = nodeList->getSessionUUID();

                    if (entity->getSimulatorID() == myNodeID) {
                        // we think we already own the simulation, so make sure to send ALL TerseUpdate properties
                        if (hasTerseUpdateChanges) {
                            entity->getAllTerseUpdateProperties(properties);
                        }
                        // TODO: if we knew that ONLY TerseUpdate properties have changed in properties AND the object 
                        // is dynamic AND it is active in the physics simulation then we could chose to NOT queue an update 
                        // and instead let the physics simulation decide when to send a terse update.  This would remove
                        // the "slide-no-rotate" glitch (and typical a double-update) that we see during the "poke rolling
                        // balls" test.  However, even if we solve this problem we still need to provide a "slerp the visible
                        // proxy toward the true physical position" feature to hide the final glitches in the remote watcher's
                        // simulation.

                        if (entity->getSimulationPriority() < SCRIPT_EDIT_SIMULATION_PRIORITY) {
                            // we re-assert our simulation ownership at a higher priority
                            properties.setSimulationOwner(myNodeID, 
                                    glm::max(entity->getSimulationPriority(), SCRIPT_EDIT_SIMULATION_PRIORITY));
                        }
                    } else {
                        // we make a bid for simulation ownership
                        properties.setSimulationOwner(myNodeID, SCRIPT_EDIT_SIMULATION_PRIORITY);
                        entity->flagForOwnership();
                    }
                }
                entity->setLastBroadcast(usecTimestampNow());
            }
            _entityTree->unlock();
            queueEntityMessage(PacketType::EntityEdit, entityID, properties);
            return id;
        }
        return QUuid();
    }

    queueEntityMessage(PacketType::EntityEdit, entityID, properties);
    return id;
}

void EntityScriptingInterface::deleteEntity(QUuid id) {
    EntityItemID entityID(id);
    bool shouldDelete = true;

    // If we have a local entity tree set, then also update it.
    if (_entityTree) {
        _entityTree->lockForWrite();

        EntityItemPointer entity = _entityTree->findEntityByEntityItemID(entityID);
        if (entity) {
            if (entity->getLocked()) {
                shouldDelete = false;
            } else {
                _entityTree->deleteEntity(entityID);
            }
        }

        _entityTree->unlock();
    }

    // if at this point, we know the id, and we should still delete the entity, send the update to the entity server
    if (shouldDelete) {
        getEntityPacketSender()->queueEraseEntityMessage(entityID);
    }
}

QUuid EntityScriptingInterface::findClosestEntity(const glm::vec3& center, float radius) const {
    EntityItemID result;
    if (_entityTree) {
        _entityTree->lockForRead();
        EntityItemPointer closestEntity = _entityTree->findClosestEntity(center, radius);
        _entityTree->unlock();
        if (closestEntity) {
            result = closestEntity->getEntityItemID();
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

QVector<QUuid> EntityScriptingInterface::findEntities(const glm::vec3& center, float radius) const {
    QVector<QUuid> result;
    if (_entityTree) {
        _entityTree->lockForRead();
        QVector<EntityItemPointer> entities;
        _entityTree->findEntities(center, radius, entities);
        _entityTree->unlock();

        foreach (EntityItemPointer entity, entities) {
            result << entity->getEntityItemID();
        }
    }
    return result;
}

QVector<QUuid> EntityScriptingInterface::findEntitiesInBox(const glm::vec3& corner, const glm::vec3& dimensions) const {
    QVector<QUuid> result;
    if (_entityTree) {
        _entityTree->lockForRead();
        AABox box(corner, dimensions);
        QVector<EntityItemPointer> entities;
        _entityTree->findEntities(box, entities);
        _entityTree->unlock();

        foreach (EntityItemPointer entity, entities) {
            result << entity->getEntityItemID();
        }
    }
    return result;
}

RayToEntityIntersectionResult EntityScriptingInterface::findRayIntersection(const PickRay& ray, bool precisionPicking) {
    return findRayIntersectionWorker(ray, Octree::TryLock, precisionPicking);
}

RayToEntityIntersectionResult EntityScriptingInterface::findRayIntersectionBlocking(const PickRay& ray, bool precisionPicking) {
    return findRayIntersectionWorker(ray, Octree::Lock, precisionPicking);
}

RayToEntityIntersectionResult EntityScriptingInterface::findRayIntersectionWorker(const PickRay& ray,
                                                                                    Octree::lockType lockType,
                                                                                    bool precisionPicking) {


    RayToEntityIntersectionResult result;
    if (_entityTree) {
        OctreeElement* element;
        EntityItemPointer intersectedEntity = NULL;
        result.intersects = _entityTree->findRayIntersection(ray.origin, ray.direction, element, result.distance, result.face,
                                                                (void**)&intersectedEntity, lockType, &result.accurate,
                                                                precisionPicking);
        if (result.intersects && intersectedEntity) {
            result.entityID = intersectedEntity->getEntityItemID();
            result.properties = intersectedEntity->getProperties();
            result.intersection = ray.origin + (ray.direction * result.distance);
        }
    }
    return result;
}

void EntityScriptingInterface::setLightsArePickable(bool value) {
    LightEntityItem::setLightsArePickable(value);
}

bool EntityScriptingInterface::getLightsArePickable() const {
    return LightEntityItem::getLightsArePickable();
}

void EntityScriptingInterface::setZonesArePickable(bool value) {
    ZoneEntityItem::setZonesArePickable(value);
}

bool EntityScriptingInterface::getZonesArePickable() const {
    return ZoneEntityItem::getZonesArePickable();
}

void EntityScriptingInterface::setDrawZoneBoundaries(bool value) {
    ZoneEntityItem::setDrawZoneBoundaries(value);
}

bool EntityScriptingInterface::getDrawZoneBoundaries() const {
    return ZoneEntityItem::getDrawZoneBoundaries();
}

void EntityScriptingInterface::setSendPhysicsUpdates(bool value) {
    EntityItem::setSendPhysicsUpdates(value);
}

bool EntityScriptingInterface::getSendPhysicsUpdates() const {
    return EntityItem::getSendPhysicsUpdates();
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
    // EntityItemIDfromScriptValue(entityIDValue, value.entityID);
    quuidFromScriptValue(entityIDValue, value.entityID);
    QScriptValue entityPropertiesValue = object.property("properties");
    if (entityPropertiesValue.isValid()) {
        EntityItemPropertiesFromScriptValueHonorReadOnly(entityPropertiesValue, value.properties);
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

bool EntityScriptingInterface::setVoxels(QUuid entityID,
                                         std::function<bool(PolyVoxEntityItem&)> actor) {
    if (!_entityTree) {
        return false;
    }

    EntityItemPointer entity = _entityTree->findEntityByEntityItemID(entityID);
    if (!entity) {
        qCDebug(entities) << "EntityScriptingInterface::setVoxelSphere no entity with ID" << entityID;
        return false;
    }

    EntityTypes::EntityType entityType = entity->getType();
    if (entityType != EntityTypes::PolyVox) {
        return false;
    }

    auto polyVoxEntity = std::dynamic_pointer_cast<PolyVoxEntityItem>(entity);
    _entityTree->lockForWrite();
    bool result = actor(*polyVoxEntity);
    _entityTree->unlock();
    return result;
}

bool EntityScriptingInterface::setPoints(QUuid entityID, std::function<bool(LineEntityItem&)> actor) {
    if (!_entityTree) {
        return false;
    }

    EntityItemPointer entity = static_cast<EntityItemPointer>(_entityTree->findEntityByEntityItemID(entityID));
    if (!entity) {
        qCDebug(entities) << "EntityScriptingInterface::setPoints no entity with ID" << entityID;
    }

    EntityTypes::EntityType entityType = entity->getType();

    if (entityType != EntityTypes::Line) {
        return false;
    }

    auto now = usecTimestampNow();

    auto lineEntity = std::static_pointer_cast<LineEntityItem>(entity);
    _entityTree->lockForWrite();
    bool success = actor(*lineEntity);
    entity->setLastEdited(now);
    entity->setLastBroadcast(now);
    _entityTree->unlock();

    _entityTree->lockForRead();
    EntityItemProperties properties = entity->getProperties();
    _entityTree->unlock();

    properties.setLinePointsDirty();
    properties.setLastEdited(now);

    queueEntityMessage(PacketType::EntityEdit, entityID, properties);
    return success;
}


bool EntityScriptingInterface::setVoxelSphere(QUuid entityID, const glm::vec3& center, float radius, int value) {
    return setVoxels(entityID, [center, radius, value](PolyVoxEntityItem& polyVoxEntity) {
            return polyVoxEntity.setSphere(center, radius, value);
        });
}

bool EntityScriptingInterface::setVoxel(QUuid entityID, const glm::vec3& position, int value) {
    return setVoxels(entityID, [position, value](PolyVoxEntityItem& polyVoxEntity) {
            return polyVoxEntity.setVoxelInVolume(position, value);
        });
}

bool EntityScriptingInterface::setAllVoxels(QUuid entityID, int value) {
    return setVoxels(entityID, [value](PolyVoxEntityItem& polyVoxEntity) {
            return polyVoxEntity.setAll(value);
        });
}

bool EntityScriptingInterface::setAllPoints(QUuid entityID, const QVector<glm::vec3>& points) {
    EntityItemPointer entity = static_cast<EntityItemPointer>(_entityTree->findEntityByEntityItemID(entityID));
    if (!entity) {
        qCDebug(entities) << "EntityScriptingInterface::setPoints no entity with ID" << entityID;
    }

    EntityTypes::EntityType entityType = entity->getType();

    if (entityType == EntityTypes::Line) {
        return setPoints(entityID, [points](LineEntityItem& lineEntity) -> bool
        {
            return (LineEntityItem*)lineEntity.setLinePoints(points);
        });
    }

    return false;
}

bool EntityScriptingInterface::appendPoint(QUuid entityID, const glm::vec3& point) {
    EntityItemPointer entity = static_cast<EntityItemPointer>(_entityTree->findEntityByEntityItemID(entityID));
    if (!entity) {
        qCDebug(entities) << "EntityScriptingInterface::setPoints no entity with ID" << entityID;
    }
    
    EntityTypes::EntityType entityType = entity->getType();
    
    if (entityType == EntityTypes::Line) {
        return setPoints(entityID, [point](LineEntityItem& lineEntity) -> bool
        {
            return (LineEntityItem*)lineEntity.appendPoint(point);
        });
    }
    
    return false;
}


bool EntityScriptingInterface::actionWorker(const QUuid& entityID,
                                            std::function<bool(EntitySimulation*, EntityItemPointer)> actor) {
    if (!_entityTree) {
        return false;
    }

    _entityTree->lockForWrite();

    EntitySimulation* simulation = _entityTree->getSimulation();
    EntityItemPointer entity = _entityTree->findEntityByEntityItemID(entityID);
    if (!entity) {
        qDebug() << "actionWorker -- unknown entity" << entityID;
        _entityTree->unlock();
        return false;
    }

    if (!simulation) {
        qDebug() << "actionWorker -- no simulation" << entityID;
        _entityTree->unlock();
        return false;
    }

    bool success = actor(simulation, entity);
    if (success) {
        _entityTree->entityChanged(entity);
    }
    _entityTree->unlock();

    // transmit the change
    _entityTree->lockForRead();
    EntityItemProperties properties = entity->getProperties();
    _entityTree->unlock();
    properties.setActionDataDirty();
    auto now = usecTimestampNow();
    properties.setLastEdited(now);
    queueEntityMessage(PacketType::EntityEdit, entityID, properties);

    return success;
}


QUuid EntityScriptingInterface::addAction(const QString& actionTypeString,
                                          const QUuid& entityID,
                                          const QVariantMap& arguments) {
    QUuid actionID = QUuid::createUuid();
    auto actionFactory = DependencyManager::get<EntityActionFactoryInterface>();
    bool success = actionWorker(entityID, [&](EntitySimulation* simulation, EntityItemPointer entity) {
            // create this action even if the entity doesn't have physics info.  it will often be the
            // case that a script adds an action immediately after an object is created, and the physicsInfo
            // is computed asynchronously.
            // if (!entity->getPhysicsInfo()) {
            //     return false;
            // }
            EntityActionType actionType = EntityActionInterface::actionTypeFromString(actionTypeString);
            if (actionType == ACTION_TYPE_NONE) {
                return false;
            }
            EntityActionPointer action = actionFactory->factory(actionType, actionID, entity, arguments);
            if (action) {
                entity->addAction(simulation, action);
                auto nodeList = DependencyManager::get<NodeList>();
                const QUuid myNodeID = nodeList->getSessionUUID();
                if (entity->getSimulatorID() != myNodeID) {
                    entity->flagForOwnership();
                }
                return true;
            }
            return false;
        });
    if (success) {
        return actionID;
    }
    return QUuid();
}


bool EntityScriptingInterface::updateAction(const QUuid& entityID, const QUuid& actionID, const QVariantMap& arguments) {
    return actionWorker(entityID, [&](EntitySimulation* simulation, EntityItemPointer entity) {
            bool success = entity->updateAction(simulation, actionID, arguments);
            if (success) {
                auto nodeList = DependencyManager::get<NodeList>();
                const QUuid myNodeID = nodeList->getSessionUUID();
                if (entity->getSimulatorID() != myNodeID) {
                    entity->flagForOwnership();
                }
            }
            return success;
        });
}

bool EntityScriptingInterface::deleteAction(const QUuid& entityID, const QUuid& actionID) {
    return actionWorker(entityID, [&](EntitySimulation* simulation, EntityItemPointer entity) {
            return entity->removeAction(simulation, actionID);
        });
}

QVector<QUuid> EntityScriptingInterface::getActionIDs(const QUuid& entityID) {
    QVector<QUuid> result;
    actionWorker(entityID, [&](EntitySimulation* simulation, EntityItemPointer entity) {
            QList<QUuid> actionIDs = entity->getActionIDs();
            result = QVector<QUuid>::fromList(actionIDs);
            return true;
        });
    return result;
}

QVariantMap EntityScriptingInterface::getActionArguments(const QUuid& entityID, const QUuid& actionID) {
    QVariantMap result;
    actionWorker(entityID, [&](EntitySimulation* simulation, EntityItemPointer entity) {
            result = entity->getActionArguments(actionID);
            return true;
        });
    return result;
}

glm::vec3 EntityScriptingInterface::voxelCoordsToWorldCoords(const QUuid& entityID, glm::vec3 voxelCoords) {
    if (!_entityTree) {
        return glm::vec3(0.0f);
    }

    EntityItemPointer entity = _entityTree->findEntityByEntityItemID(entityID);
    if (!entity) {
        qCDebug(entities) << "EntityScriptingInterface::voxelCoordsToWorldCoords no entity with ID" << entityID;
        return glm::vec3(0.0f);
    }

    EntityTypes::EntityType entityType = entity->getType();
    if (entityType != EntityTypes::PolyVox) {
        return glm::vec3(0.0f);
    }

    auto polyVoxEntity = std::dynamic_pointer_cast<PolyVoxEntityItem>(entity);
    return polyVoxEntity->voxelCoordsToWorldCoords(voxelCoords);
}

glm::vec3 EntityScriptingInterface::worldCoordsToVoxelCoords(const QUuid& entityID, glm::vec3 worldCoords) {
    if (!_entityTree) {
        return glm::vec3(0.0f);
    }

    EntityItemPointer entity = _entityTree->findEntityByEntityItemID(entityID);
    if (!entity) {
        qCDebug(entities) << "EntityScriptingInterface::worldCoordsToVoxelCoords no entity with ID" << entityID;
        return glm::vec3(0.0f);
    }

    EntityTypes::EntityType entityType = entity->getType();
    if (entityType != EntityTypes::PolyVox) {
        return glm::vec3(0.0f);
    }

    auto polyVoxEntity = std::dynamic_pointer_cast<PolyVoxEntityItem>(entity);
    return polyVoxEntity->worldCoordsToVoxelCoords(worldCoords);
}

glm::vec3 EntityScriptingInterface::voxelCoordsToLocalCoords(const QUuid& entityID, glm::vec3 voxelCoords) {
    if (!_entityTree) {
        return glm::vec3(0.0f);
    }

    EntityItemPointer entity = _entityTree->findEntityByEntityItemID(entityID);
    if (!entity) {
        qCDebug(entities) << "EntityScriptingInterface::voxelCoordsToLocalCoords no entity with ID" << entityID;
        return glm::vec3(0.0f);
    }

    EntityTypes::EntityType entityType = entity->getType();
    if (entityType != EntityTypes::PolyVox) {
        return glm::vec3(0.0f);
    }

    auto polyVoxEntity = std::dynamic_pointer_cast<PolyVoxEntityItem>(entity);
    return polyVoxEntity->voxelCoordsToLocalCoords(voxelCoords);
}

glm::vec3 EntityScriptingInterface::localCoordsToVoxelCoords(const QUuid& entityID, glm::vec3 localCoords) {
    if (!_entityTree) {
        return glm::vec3(0.0f);
    }

    EntityItemPointer entity = _entityTree->findEntityByEntityItemID(entityID);
    if (!entity) {
        qCDebug(entities) << "EntityScriptingInterface::localCoordsToVoxelCoords no entity with ID" << entityID;
        return glm::vec3(0.0f);
    }

    EntityTypes::EntityType entityType = entity->getType();
    if (entityType != EntityTypes::PolyVox) {
        return glm::vec3(0.0f);
    }

    auto polyVoxEntity = std::dynamic_pointer_cast<PolyVoxEntityItem>(entity);
    return polyVoxEntity->localCoordsToVoxelCoords(localCoords);
}
