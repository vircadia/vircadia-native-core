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

#include "EntityItemID.h"
#include <VariantMapToScriptValue.h>
#include <SharedUtil.h>
#include <SpatialParentFinder.h>

#include "EntitiesLogging.h"
#include "EntityActionFactoryInterface.h"
#include "EntityActionInterface.h"
#include "EntitySimulation.h"
#include "EntityTree.h"
#include "LightEntityItem.h"
#include "ModelEntityItem.h"
#include "QVariantGLM.h"
#include "SimulationOwner.h"
#include "ZoneEntityItem.h"
#include "WebEntityItem.h"

EntityScriptingInterface::EntityScriptingInterface(bool bidOnSimulationOwnership) :
    _entityTree(NULL),
    _bidOnSimulationOwnership(bidOnSimulationOwnership)
{
    auto nodeList = DependencyManager::get<NodeList>();
    connect(nodeList.data(), &NodeList::isAllowedEditorChanged, this, &EntityScriptingInterface::canAdjustLocksChanged);
    connect(nodeList.data(), &NodeList::canRezChanged, this, &EntityScriptingInterface::canRezChanged);
    connect(nodeList.data(), &NodeList::canRezTmpChanged, this, &EntityScriptingInterface::canRezTmpChanged);
    connect(nodeList.data(), &NodeList::canWriteAssetsChanged, this, &EntityScriptingInterface::canWriteAssetsChanged);
}

void EntityScriptingInterface::queueEntityMessage(PacketType packetType,
                                                  EntityItemID entityID, const EntityItemProperties& properties) {
    getEntityPacketSender()->queueEditEntityMessage(packetType, _entityTree, entityID, properties);
}

void EntityScriptingInterface::resetActivityTracking() {
    _activityTracking.addedEntityCount = 0;
    _activityTracking.deletedEntityCount = 0;
    _activityTracking.editedEntityCount = 0;
}

bool EntityScriptingInterface::canAdjustLocks() {
    auto nodeList = DependencyManager::get<NodeList>();
    return nodeList->isAllowedEditor();
}

bool EntityScriptingInterface::canRez() {
    auto nodeList = DependencyManager::get<NodeList>();
    return nodeList->getThisNodeCanRez();
}

bool EntityScriptingInterface::canRezTmp() {
    auto nodeList = DependencyManager::get<NodeList>();
    return nodeList->getThisNodeCanRezTmp();
}

bool EntityScriptingInterface::canWriteAssets() {
    auto nodeList = DependencyManager::get<NodeList>();
    return nodeList->getThisNodeCanWriteAssets();
}

void EntityScriptingInterface::setEntityTree(EntityTreePointer elementTree) {
    if (_entityTree) {
        disconnect(_entityTree.get(), &EntityTree::addingEntity, this, &EntityScriptingInterface::addingEntity);
        disconnect(_entityTree.get(), &EntityTree::deletingEntity, this, &EntityScriptingInterface::deletingEntity);
        disconnect(_entityTree.get(), &EntityTree::clearingEntities, this, &EntityScriptingInterface::clearingEntities);
    }

    _entityTree = elementTree;

    if (_entityTree) {
        connect(_entityTree.get(), &EntityTree::addingEntity, this, &EntityScriptingInterface::addingEntity);
        connect(_entityTree.get(), &EntityTree::deletingEntity, this, &EntityScriptingInterface::deletingEntity);
        connect(_entityTree.get(), &EntityTree::clearingEntities, this, &EntityScriptingInterface::clearingEntities);
    }
}

EntityItemProperties convertLocationToScriptSemantics(const EntityItemProperties& entitySideProperties) {
    // In EntityTree code, properties.position and properties.rotation are relative to the parent.  In javascript,
    // they are in world-space.  The local versions are put into localPosition and localRotation and position and
    // rotation are converted from local to world space.
    EntityItemProperties scriptSideProperties = entitySideProperties;
    scriptSideProperties.setLocalPosition(entitySideProperties.getPosition());
    scriptSideProperties.setLocalRotation(entitySideProperties.getRotation());
    scriptSideProperties.setLocalVelocity(entitySideProperties.getLocalVelocity());
    scriptSideProperties.setLocalAngularVelocity(entitySideProperties.getLocalAngularVelocity());

    bool success;
    glm::vec3 worldPosition = SpatiallyNestable::localToWorld(entitySideProperties.getPosition(),
                                                              entitySideProperties.getParentID(),
                                                              entitySideProperties.getParentJointIndex(),
                                                              success);
    glm::quat worldRotation = SpatiallyNestable::localToWorld(entitySideProperties.getRotation(),
                                                              entitySideProperties.getParentID(),
                                                              entitySideProperties.getParentJointIndex(),
                                                              success);
    glm::vec3 worldVelocity = SpatiallyNestable::localToWorldVelocity(entitySideProperties.getVelocity(),
                                                                      entitySideProperties.getParentID(),
                                                                      entitySideProperties.getParentJointIndex(),
                                                                      success);
    glm::vec3 worldAngularVelocity = SpatiallyNestable::localToWorldAngularVelocity(entitySideProperties.getAngularVelocity(),
                                                                                    entitySideProperties.getParentID(),
                                                                                    entitySideProperties.getParentJointIndex(),
                                                                                    success);

    scriptSideProperties.setPosition(worldPosition);
    scriptSideProperties.setRotation(worldRotation);
    scriptSideProperties.setVelocity(worldVelocity);
    scriptSideProperties.setAngularVelocity(worldAngularVelocity);

    return scriptSideProperties;
}


EntityItemProperties convertLocationFromScriptSemantics(const EntityItemProperties& scriptSideProperties) {
    // convert position and rotation properties from world-space to local, unless localPosition and localRotation
    // are set.  If they are set, they overwrite position and rotation.
    EntityItemProperties entitySideProperties = scriptSideProperties;
    bool success;

    // TODO -- handle velocity and angularVelocity

    if (scriptSideProperties.localPositionChanged()) {
        entitySideProperties.setPosition(scriptSideProperties.getLocalPosition());
    } else if (scriptSideProperties.positionChanged()) {
        glm::vec3 localPosition = SpatiallyNestable::worldToLocal(entitySideProperties.getPosition(),
                                                                  entitySideProperties.getParentID(),
                                                                  entitySideProperties.getParentJointIndex(),
                                                                  success);
        entitySideProperties.setPosition(localPosition);
    }

    if (scriptSideProperties.localRotationChanged()) {
        entitySideProperties.setRotation(scriptSideProperties.getLocalRotation());
    } else if (scriptSideProperties.rotationChanged()) {
        glm::quat localRotation = SpatiallyNestable::worldToLocal(entitySideProperties.getRotation(),
                                                                  entitySideProperties.getParentID(),
                                                                  entitySideProperties.getParentJointIndex(),
                                                                  success);
        entitySideProperties.setRotation(localRotation);
    }

    if (scriptSideProperties.localVelocityChanged()) {
        entitySideProperties.setVelocity(scriptSideProperties.getLocalVelocity());
    } else if (scriptSideProperties.velocityChanged()) {
        glm::vec3 localVelocity = SpatiallyNestable::worldToLocalVelocity(entitySideProperties.getVelocity(),
                                                                          entitySideProperties.getParentID(),
                                                                          entitySideProperties.getParentJointIndex(),
                                                                          success);
        entitySideProperties.setVelocity(localVelocity);
    }

    if (scriptSideProperties.localAngularVelocityChanged()) {
        entitySideProperties.setAngularVelocity(scriptSideProperties.getLocalAngularVelocity());
    } else if (scriptSideProperties.angularVelocityChanged()) {
        glm::vec3 localAngularVelocity =
            SpatiallyNestable::worldToLocalAngularVelocity(entitySideProperties.getAngularVelocity(),
                                                           entitySideProperties.getParentID(),
                                                           entitySideProperties.getParentJointIndex(),
                                                           success);
        entitySideProperties.setAngularVelocity(localAngularVelocity);
    }

    return entitySideProperties;
}


QUuid EntityScriptingInterface::addEntity(const EntityItemProperties& properties, bool clientOnly) {
    _activityTracking.addedEntityCount++;

    EntityItemProperties propertiesWithSimID = convertLocationFromScriptSemantics(properties);
    propertiesWithSimID.setDimensionsInitialized(properties.dimensionsChanged());

    if (clientOnly) {
        auto nodeList = DependencyManager::get<NodeList>();
        const QUuid myNodeID = nodeList->getSessionUUID();
        propertiesWithSimID.setClientOnly(clientOnly);
        propertiesWithSimID.setOwningAvatarID(myNodeID);
    }

    if (propertiesWithSimID.getParentID() == AVATAR_SELF_ID) {
        qDebug() << "ERROR: Cannot set entity parent ID to the local-only MyAvatar ID";
        propertiesWithSimID.setParentID(QUuid());
    }

    auto dimensions = propertiesWithSimID.getDimensions();
    float volume = dimensions.x * dimensions.y * dimensions.z;
    auto density = propertiesWithSimID.getDensity();
    auto newVelocity = propertiesWithSimID.getVelocity().length();
    float cost = calculateCost(density * volume, 0, newVelocity);
    cost *= costMultiplier;

    if (cost > _currentAvatarEnergy) {
        return QUuid();
    }

    EntityItemID id = EntityItemID(QUuid::createUuid());

    // If we have a local entity tree set, then also update it.
    bool success = true;
    if (_entityTree) {
        _entityTree->withWriteLock([&] {
            EntityItemPointer entity = _entityTree->addEntity(id, propertiesWithSimID);
            if (entity) {
                if (propertiesWithSimID.parentRelatedPropertyChanged()) {
                    // due to parenting, the server may not know where something is in world-space, so include the bounding cube.
                    bool success;
                    AACube queryAACube = entity->getQueryAACube(success);
                    if (success) {
                        propertiesWithSimID.setQueryAACube(queryAACube);
                    }
                }

                if (_bidOnSimulationOwnership) {
                    // This Node is creating a new object.  If it's in motion, set this Node as the simulator.
                    auto nodeList = DependencyManager::get<NodeList>();
                    const QUuid myNodeID = nodeList->getSessionUUID();

                    // and make note of it now, so we can act on it right away.
                    propertiesWithSimID.setSimulationOwner(myNodeID, SCRIPT_POKE_SIMULATION_PRIORITY);
                    entity->setSimulationOwner(myNodeID, SCRIPT_POKE_SIMULATION_PRIORITY);
                }

                entity->setLastBroadcast(usecTimestampNow());
                propertiesWithSimID.setLastEdited(entity->getLastEdited());
            } else {
                qCDebug(entities) << "script failed to add new Entity to local Octree";
                success = false;
            }
        });
    }

    // queue the packet
    if (success) {
        emit debitEnergySource(cost);
        queueEntityMessage(PacketType::EntityAdd, id, propertiesWithSimID);

        return id;
    } else {
        return QUuid();
    }
}

QUuid EntityScriptingInterface::addModelEntity(const QString& name, const QString& modelUrl, const QString& shapeType,
                                               bool dynamic, const glm::vec3& position, const glm::vec3& gravity) {
    _activityTracking.addedEntityCount++;

    EntityItemProperties properties;
    properties.setType(EntityTypes::Model);
    properties.setName(name);
    properties.setModelURL(modelUrl);
    properties.setShapeTypeFromString(shapeType);
    properties.setDynamic(dynamic);
    properties.setPosition(position);
    properties.setGravity(gravity);
    return addEntity(properties);
}

EntityItemProperties EntityScriptingInterface::getEntityProperties(QUuid identity) {
    EntityPropertyFlags noSpecificProperties;
    return getEntityProperties(identity, noSpecificProperties);
}

EntityItemProperties EntityScriptingInterface::getEntityProperties(QUuid identity, EntityPropertyFlags desiredProperties) {
    EntityItemProperties results;
    if (_entityTree) {
        _entityTree->withReadLock([&] {
            EntityItemPointer entity = _entityTree->findEntityByEntityItemID(EntityItemID(identity));
            if (entity) {
                if (desiredProperties.getHasProperty(PROP_POSITION) ||
                    desiredProperties.getHasProperty(PROP_ROTATION) ||
                    desiredProperties.getHasProperty(PROP_LOCAL_POSITION) ||
                    desiredProperties.getHasProperty(PROP_LOCAL_ROTATION)) {
                    // if we are explicitly getting position or rotation, we need parent information to make sense of them.
                    desiredProperties.setHasProperty(PROP_PARENT_ID);
                    desiredProperties.setHasProperty(PROP_PARENT_JOINT_INDEX);
                }

                if (desiredProperties.isEmpty()) {
                    // these are left out of EntityItem::getEntityProperties so that localPosition and localRotation
                    // don't end up in json saves, etc.  We still want them here, though.
                    EncodeBitstreamParams params; // unknown
                    desiredProperties = entity->getEntityProperties(params);
                    desiredProperties.setHasProperty(PROP_LOCAL_POSITION);
                    desiredProperties.setHasProperty(PROP_LOCAL_ROTATION);
                 }

                results = entity->getProperties(desiredProperties);

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
        });
    }

    return convertLocationToScriptSemantics(results);
}

QUuid EntityScriptingInterface::editEntity(QUuid id, const EntityItemProperties& scriptSideProperties) {
    _activityTracking.editedEntityCount++;

    EntityItemProperties properties = scriptSideProperties;

    auto dimensions = properties.getDimensions();
    float volume = dimensions.x * dimensions.y * dimensions.z;
    auto density = properties.getDensity();
    auto newVelocity = properties.getVelocity().length();
    float oldVelocity = { 0.0f };

    EntityItemID entityID(id);
    if (!_entityTree) {
        queueEntityMessage(PacketType::EntityEdit, entityID, properties);

        //if there is no local entity entity tree, no existing velocity, use 0.
        float cost = calculateCost(density * volume, oldVelocity, newVelocity);
        cost *= costMultiplier;

        if (cost > _currentAvatarEnergy) {
            return QUuid();
        } else {
            //debit the avatar energy and continue
            emit debitEnergySource(cost);
        }

        return id;
    }
    // If we have a local entity tree set, then also update it.

    bool updatedEntity = false;
    _entityTree->withWriteLock([&] {
        EntityItemPointer entity = _entityTree->findEntityByEntityItemID(entityID);
        if (!entity) {
            return;
        }

        auto nodeList = DependencyManager::get<NodeList>();
        if (entity->getClientOnly() && entity->getOwningAvatarID() != nodeList->getSessionUUID()) {
            // don't edit other avatar's avatarEntities
            return;
        }

        if (scriptSideProperties.parentRelatedPropertyChanged()) {
            // All of parentID, parentJointIndex, position, rotation are needed to make sense of any of them.
            // If any of these changed, pull any missing properties from the entity.

            //existing entity, retrieve old velocity for check down below
            oldVelocity = entity->getVelocity().length();

            if (!scriptSideProperties.parentIDChanged()) {
                properties.setParentID(entity->getParentID());
            } else if (scriptSideProperties.getParentID() == AVATAR_SELF_ID) {
                qDebug() << "ERROR: Cannot set entity parent ID to the local-only MyAvatar ID";
                properties.setParentID(QUuid());
            }
            if (!scriptSideProperties.parentJointIndexChanged()) {
                properties.setParentJointIndex(entity->getParentJointIndex());
            }
            if (!scriptSideProperties.localPositionChanged() && !scriptSideProperties.positionChanged()) {
                properties.setPosition(entity->getPosition());
            }
            if (!scriptSideProperties.localRotationChanged() && !scriptSideProperties.rotationChanged()) {
                properties.setRotation(entity->getOrientation());
            }
        }
        properties = convertLocationFromScriptSemantics(properties);
        properties.setClientOnly(entity->getClientOnly());
        properties.setOwningAvatarID(entity->getOwningAvatarID());

        float cost = calculateCost(density * volume, oldVelocity, newVelocity);
        cost *= costMultiplier;

        if (cost > _currentAvatarEnergy) {
            updatedEntity = false;
        } else {
            //debit the avatar energy and continue
            updatedEntity = _entityTree->updateEntity(entityID, properties);
            if (updatedEntity) {
                emit debitEnergySource(cost);
            }
        }
    });

    // FIXME: We need to figure out a better way to handle this. Allowing these edits to go through potentially
    // breaks avatar energy and entities that are parented.
    //
    // To handle cases where a script needs to edit an entity with a _known_ entity id but doesn't exist
    // in the local entity tree, we need to allow those edits to go through to the server.
    // if (!updatedEntity) {
    //     return QUuid();
    // }

    _entityTree->withReadLock([&] {
        EntityItemPointer entity = _entityTree->findEntityByEntityItemID(entityID);
        if (entity) {
            // make sure the properties has a type, so that the encode can know which properties to include
            properties.setType(entity->getType());
            bool hasTerseUpdateChanges = properties.hasTerseUpdateChanges();
            bool hasPhysicsChanges = properties.hasMiscPhysicsChanges() || hasTerseUpdateChanges;
            if (_bidOnSimulationOwnership && hasPhysicsChanges) {
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
                    // the "slide-no-rotate" glitch (and typical double-update) that we see during the "poke rolling
                    // balls" test.  However, even if we solve this problem we still need to provide a "slerp the visible
                    // proxy toward the true physical position" feature to hide the final glitches in the remote watcher's
                    // simulation.

                    if (entity->getSimulationPriority() < SCRIPT_POKE_SIMULATION_PRIORITY) {
                        // we re-assert our simulation ownership at a higher priority
                        properties.setSimulationOwner(myNodeID, SCRIPT_POKE_SIMULATION_PRIORITY);
                    }
                } else {
                    // we make a bid for simulation ownership
                    properties.setSimulationOwner(myNodeID, SCRIPT_POKE_SIMULATION_PRIORITY);
                    entity->pokeSimulationOwnership();
                }
            }
            if (properties.parentRelatedPropertyChanged() && entity->computePuffedQueryAACube()) {
                properties.setQueryAACube(entity->getQueryAACube());
            }
            entity->setLastBroadcast(usecTimestampNow());
            properties.setLastEdited(entity->getLastEdited());

            // if we've moved an entity with children, check/update the queryAACube of all descendents and tell the server
            // if they've changed.
            entity->forEachDescendant([&](SpatiallyNestablePointer descendant) {
                if (descendant->getNestableType() == NestableType::Entity) {
                    if (descendant->computePuffedQueryAACube()) {
                        EntityItemPointer entityDescendant = std::static_pointer_cast<EntityItem>(descendant);
                        EntityItemProperties newQueryCubeProperties;
                        newQueryCubeProperties.setQueryAACube(descendant->getQueryAACube());
                        newQueryCubeProperties.setLastEdited(properties.getLastEdited());
                        queueEntityMessage(PacketType::EntityEdit, descendant->getID(), newQueryCubeProperties);
                        entityDescendant->setLastBroadcast(usecTimestampNow());
                    }
                }
            });
        }
    });
    queueEntityMessage(PacketType::EntityEdit, entityID, properties);
    return id;
}

void EntityScriptingInterface::deleteEntity(QUuid id) {
    _activityTracking.deletedEntityCount++;

    EntityItemID entityID(id);
    bool shouldDelete = true;

    // If we have a local entity tree set, then also update it.
    if (_entityTree) {
        _entityTree->withWriteLock([&] {
            EntityItemPointer entity = _entityTree->findEntityByEntityItemID(entityID);
            if (entity) {

                auto nodeList = DependencyManager::get<NodeList>();
                const QUuid myNodeID = nodeList->getSessionUUID();
                if (entity->getClientOnly() && entity->getOwningAvatarID() != myNodeID) {
                    // don't delete other avatar's avatarEntities
                    shouldDelete = false;
                    return;
                }

                auto dimensions = entity->getDimensions();
                float volume = dimensions.x * dimensions.y * dimensions.z;
                auto density = entity->getDensity();
                auto velocity = entity->getVelocity().length();
                float cost = calculateCost(density * volume, velocity, 0);
                cost *= costMultiplier;

                if (cost > _currentAvatarEnergy) {
                    shouldDelete = false;
                    return;
                } else {
                    //debit the avatar energy and continue
                    emit debitEnergySource(cost);
                }

                if (entity->getLocked()) {
                    shouldDelete = false;
                } else {
                    _entityTree->deleteEntity(entityID);
                }
            }
        });
    }

    // if at this point, we know the id, and we should still delete the entity, send the update to the entity server
    if (shouldDelete) {
        getEntityPacketSender()->queueEraseEntityMessage(entityID);
    }
}

void EntityScriptingInterface::setEntitiesScriptEngine(EntitiesScriptEngineProvider* engine) {
    std::lock_guard<std::recursive_mutex> lock(_entitiesScriptEngineLock);
    _entitiesScriptEngine = engine;
}

void EntityScriptingInterface::callEntityMethod(QUuid id, const QString& method, const QStringList& params) {
    std::lock_guard<std::recursive_mutex> lock(_entitiesScriptEngineLock);
    if (_entitiesScriptEngine) {
        EntityItemID entityID{ id };
        _entitiesScriptEngine->callEntityScriptMethod(entityID, method, params);
    }
}

QUuid EntityScriptingInterface::findClosestEntity(const glm::vec3& center, float radius) const {
    EntityItemID result;
    if (_entityTree) {
        EntityItemPointer closestEntity;
        _entityTree->withReadLock([&] {
            closestEntity = _entityTree->findClosestEntity(center, radius);
        });
        if (closestEntity) {
            result = closestEntity->getEntityItemID();
        }
    }
    return result;
}


void EntityScriptingInterface::dumpTree() const {
    if (_entityTree) {
        _entityTree->withReadLock([&] {
            _entityTree->dumpTree();
        });
    }
}

QVector<QUuid> EntityScriptingInterface::findEntities(const glm::vec3& center, float radius) const {
    QVector<QUuid> result;
    if (_entityTree) {
        QVector<EntityItemPointer> entities;
        _entityTree->withReadLock([&] {
            _entityTree->findEntities(center, radius, entities);
        });

        foreach (EntityItemPointer entity, entities) {
            result << entity->getEntityItemID();
        }
    }
    return result;
}

QVector<QUuid> EntityScriptingInterface::findEntitiesInBox(const glm::vec3& corner, const glm::vec3& dimensions) const {
    QVector<QUuid> result;
    if (_entityTree) {
        QVector<EntityItemPointer> entities;
        _entityTree->withReadLock([&] {
            AABox box(corner, dimensions);
            _entityTree->findEntities(box, entities);
        });

        foreach (EntityItemPointer entity, entities) {
            result << entity->getEntityItemID();
        }
    }
    return result;
}

QVector<QUuid> EntityScriptingInterface::findEntitiesInFrustum(QVariantMap frustum) const {
    QVector<QUuid> result;

    const QString POSITION_PROPERTY = "position";
    bool positionOK = frustum.contains(POSITION_PROPERTY);
    glm::vec3 position = positionOK ? qMapToGlmVec3(frustum[POSITION_PROPERTY]) : glm::vec3();

    const QString ORIENTATION_PROPERTY = "orientation";
    bool orientationOK = frustum.contains(ORIENTATION_PROPERTY);
    glm::quat orientation = orientationOK ? qMapToGlmQuat(frustum[ORIENTATION_PROPERTY]) : glm::quat();

    const QString PROJECTION_PROPERTY = "projection";
    bool projectionOK = frustum.contains(PROJECTION_PROPERTY);
    glm::mat4 projection = projectionOK ? qMapToGlmMat4(frustum[PROJECTION_PROPERTY]) : glm::mat4();

    const QString CENTER_RADIUS_PROPERTY = "centerRadius";
    bool centerRadiusOK = frustum.contains(CENTER_RADIUS_PROPERTY);
    float centerRadius = centerRadiusOK ? frustum[CENTER_RADIUS_PROPERTY].toFloat() : 0.0f;

    if (positionOK && orientationOK && projectionOK && centerRadiusOK) {
        ViewFrustum viewFrustum;
        viewFrustum.setPosition(position);
        viewFrustum.setOrientation(orientation);
        viewFrustum.setProjection(projection);
        viewFrustum.setCenterRadius(centerRadius);
        viewFrustum.calculate();

        if (_entityTree) {
            QVector<EntityItemPointer> entities;
            _entityTree->withReadLock([&] {
                _entityTree->findEntities(viewFrustum, entities);
            });

            foreach(EntityItemPointer entity, entities) {
                result << entity->getEntityItemID();
            }
        }
    }

    return result;
}

RayToEntityIntersectionResult EntityScriptingInterface::findRayIntersection(const PickRay& ray, bool precisionPicking, 
                const QScriptValue& entityIdsToInclude, const QScriptValue& entityIdsToDiscard, bool visibleOnly, bool collidableOnly) {

    QVector<EntityItemID> entitiesToInclude = qVectorEntityItemIDFromScriptValue(entityIdsToInclude);
    QVector<EntityItemID> entitiesToDiscard = qVectorEntityItemIDFromScriptValue(entityIdsToDiscard);
    return findRayIntersectionWorker(ray, Octree::Lock, precisionPicking, entitiesToInclude, entitiesToDiscard, visibleOnly, collidableOnly);
}

// FIXME - we should remove this API and encourage all users to use findRayIntersection() instead. We've changed
//         findRayIntersection() to be blocking because it never makes sense for a script to get back a non-answer
RayToEntityIntersectionResult EntityScriptingInterface::findRayIntersectionBlocking(const PickRay& ray, bool precisionPicking, 
                const QScriptValue& entityIdsToInclude, const QScriptValue& entityIdsToDiscard) {

    qWarning() << "Entities.findRayIntersectionBlocking() is obsolete, use Entities.findRayIntersection() instead.";
    const QVector<EntityItemID>& entitiesToInclude = qVectorEntityItemIDFromScriptValue(entityIdsToInclude);
    const QVector<EntityItemID> entitiesToDiscard = qVectorEntityItemIDFromScriptValue(entityIdsToDiscard);
    return findRayIntersectionWorker(ray, Octree::Lock, precisionPicking, entitiesToInclude, entitiesToDiscard);
}

RayToEntityIntersectionResult EntityScriptingInterface::findRayIntersectionWorker(const PickRay& ray,
        Octree::lockType lockType, bool precisionPicking, const QVector<EntityItemID>& entityIdsToInclude,
        const QVector<EntityItemID>& entityIdsToDiscard, bool visibleOnly, bool collidableOnly) {


    RayToEntityIntersectionResult result;
    if (_entityTree) {
        OctreeElementPointer element;
        EntityItemPointer intersectedEntity = NULL;
        result.intersects = _entityTree->findRayIntersection(ray.origin, ray.direction,
            entityIdsToInclude, entityIdsToDiscard, visibleOnly, collidableOnly, precisionPicking,
            element, result.distance, result.face, result.surfaceNormal,
            (void**)&intersectedEntity, lockType, &result.accurate);
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

    QScriptValue surfaceNormal = vec3toScriptValue(engine, value.surfaceNormal);
    obj.setProperty("surfaceNormal", surfaceNormal);
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
    QScriptValue surfaceNormal = object.property("surfaceNormal");
    if (surfaceNormal.isValid()) {
        vec3FromScriptValue(surfaceNormal, value.surfaceNormal);
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
    bool result;
    _entityTree->withWriteLock([&] {
        result = actor(*polyVoxEntity);
    });
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
    bool success;
    _entityTree->withWriteLock([&] {
        success = actor(*lineEntity);
        entity->setLastEdited(now);
        entity->setLastBroadcast(now);
    });

    EntityItemProperties properties;
    _entityTree->withReadLock([&] {
        properties = entity->getProperties();
    });

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

bool EntityScriptingInterface::setVoxelsInCuboid(QUuid entityID, const glm::vec3& lowPosition,
                                                 const glm::vec3& cuboidSize, int value) {
    return setVoxels(entityID, [lowPosition, cuboidSize, value](PolyVoxEntityItem& polyVoxEntity) {
            return polyVoxEntity.setCuboid(lowPosition, cuboidSize, value);
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
                                            std::function<bool(EntitySimulationPointer, EntityItemPointer)> actor) {
    if (!_entityTree) {
        return false;
    }

    auto nodeList = DependencyManager::get<NodeList>();
    const QUuid myNodeID = nodeList->getSessionUUID();

    EntityItemProperties properties;

    EntityItemPointer entity;
    bool doTransmit = false;
    _entityTree->withWriteLock([&] {
        EntitySimulationPointer simulation = _entityTree->getSimulation();
        entity = _entityTree->findEntityByEntityItemID(entityID);
        if (!entity) {
            qDebug() << "actionWorker -- unknown entity" << entityID;
            return;
        }

        if (!simulation) {
            qDebug() << "actionWorker -- no simulation" << entityID;
            return;
        }

        if (entity->getClientOnly() && entity->getOwningAvatarID() != myNodeID) {
            return;
        }

        doTransmit = actor(simulation, entity);
        if (doTransmit) {
            properties.setClientOnly(entity->getClientOnly());
            properties.setOwningAvatarID(entity->getOwningAvatarID());
            _entityTree->entityChanged(entity);
        }
    });

    // transmit the change
    if (doTransmit) {
        _entityTree->withReadLock([&] {
            properties = entity->getProperties();
        });

        properties.setActionDataDirty();
        auto now = usecTimestampNow();
        properties.setLastEdited(now);
        queueEntityMessage(PacketType::EntityEdit, entityID, properties);
    }

    return doTransmit;
}


QUuid EntityScriptingInterface::addAction(const QString& actionTypeString,
                                          const QUuid& entityID,
                                          const QVariantMap& arguments) {
    QUuid actionID = QUuid::createUuid();
    auto actionFactory = DependencyManager::get<EntityActionFactoryInterface>();
    bool success = false;
    actionWorker(entityID, [&](EntitySimulationPointer simulation, EntityItemPointer entity) {
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
            if (!action) {
                return false;
            }
            action->setIsMine(true);
            success = entity->addAction(simulation, action);
            entity->grabSimulationOwnership();
            return false; // Physics will cause a packet to be sent, so don't send from here.
        });
    if (success) {
        return actionID;
    }
    return QUuid();
}


bool EntityScriptingInterface::updateAction(const QUuid& entityID, const QUuid& actionID, const QVariantMap& arguments) {
    return actionWorker(entityID, [&](EntitySimulationPointer simulation, EntityItemPointer entity) {
            bool success = entity->updateAction(simulation, actionID, arguments);
            if (success) {
                entity->grabSimulationOwnership();
            }
            return success;
        });
}

bool EntityScriptingInterface::deleteAction(const QUuid& entityID, const QUuid& actionID) {
    bool success = false;
    actionWorker(entityID, [&](EntitySimulationPointer simulation, EntityItemPointer entity) {
            success = entity->removeAction(simulation, actionID);
            if (success) {
                // reduce from grab to poke
                entity->pokeSimulationOwnership();
            }
            return false; // Physics will cause a packet to be sent, so don't send from here.
        });
    return success;
}

QVector<QUuid> EntityScriptingInterface::getActionIDs(const QUuid& entityID) {
    QVector<QUuid> result;
    actionWorker(entityID, [&](EntitySimulationPointer simulation, EntityItemPointer entity) {
            QList<QUuid> actionIDs = entity->getActionIDs();
            result = QVector<QUuid>::fromList(actionIDs);
            return false; // don't send an edit packet
        });
    return result;
}

QVariantMap EntityScriptingInterface::getActionArguments(const QUuid& entityID, const QUuid& actionID) {
    QVariantMap result;
    actionWorker(entityID, [&](EntitySimulationPointer simulation, EntityItemPointer entity) {
            result = entity->getActionArguments(actionID);
            return false; // don't send an edit packet
        });
    return result;
}

EntityItemPointer EntityScriptingInterface::checkForTreeEntityAndTypeMatch(const QUuid& entityID,
                                                                           EntityTypes::EntityType entityType) {
    if (!_entityTree) {
        return EntityItemPointer();
    }
    
    EntityItemPointer entity = _entityTree->findEntityByEntityItemID(entityID);
    if (!entity) {
        qDebug() << "EntityScriptingInterface::checkForTreeEntityAndTypeMatch - no entity with ID" << entityID;
        return entity;
    }
    
    if (entityType != EntityTypes::Unknown && entity->getType() != entityType) {
        return EntityItemPointer();
    }
    
    return entity;
}

glm::vec3 EntityScriptingInterface::voxelCoordsToWorldCoords(const QUuid& entityID, glm::vec3 voxelCoords) {
    if (auto entity = checkForTreeEntityAndTypeMatch(entityID, EntityTypes::PolyVox)) {
        auto polyVoxEntity = std::dynamic_pointer_cast<PolyVoxEntityItem>(entity);
        return polyVoxEntity->voxelCoordsToWorldCoords(voxelCoords);
    } else {
        return glm::vec3(0.0f);
    }
}

glm::vec3 EntityScriptingInterface::worldCoordsToVoxelCoords(const QUuid& entityID, glm::vec3 worldCoords) {
    if (auto entity = checkForTreeEntityAndTypeMatch(entityID, EntityTypes::PolyVox)) {
        auto polyVoxEntity = std::dynamic_pointer_cast<PolyVoxEntityItem>(entity);
        return polyVoxEntity->worldCoordsToVoxelCoords(worldCoords);
    } else {
        return glm::vec3(0.0f);
    }
}

glm::vec3 EntityScriptingInterface::voxelCoordsToLocalCoords(const QUuid& entityID, glm::vec3 voxelCoords) {
    if (auto entity = checkForTreeEntityAndTypeMatch(entityID, EntityTypes::PolyVox)) {
        auto polyVoxEntity = std::dynamic_pointer_cast<PolyVoxEntityItem>(entity);
        return polyVoxEntity->voxelCoordsToLocalCoords(voxelCoords);
    } else {
        return glm::vec3(0.0f);
    }
}

glm::vec3 EntityScriptingInterface::localCoordsToVoxelCoords(const QUuid& entityID, glm::vec3 localCoords) {
    if (auto entity = checkForTreeEntityAndTypeMatch(entityID, EntityTypes::PolyVox)) {
        auto polyVoxEntity = std::dynamic_pointer_cast<PolyVoxEntityItem>(entity);
        return polyVoxEntity->localCoordsToVoxelCoords(localCoords);
    } else {
        return glm::vec3(0.0f);
    }
}

glm::vec3 EntityScriptingInterface::getAbsoluteJointTranslationInObjectFrame(const QUuid& entityID, int jointIndex) {
    if (auto entity = checkForTreeEntityAndTypeMatch(entityID, EntityTypes::Model)) {
        auto modelEntity = std::dynamic_pointer_cast<ModelEntityItem>(entity);
        return modelEntity->getAbsoluteJointTranslationInObjectFrame(jointIndex);
    } else {
        return glm::vec3(0.0f);
    }
}

glm::quat EntityScriptingInterface::getAbsoluteJointRotationInObjectFrame(const QUuid& entityID, int jointIndex) {
    if (auto entity = checkForTreeEntityAndTypeMatch(entityID, EntityTypes::Model)) {
        auto modelEntity = std::dynamic_pointer_cast<ModelEntityItem>(entity);
        return modelEntity->getAbsoluteJointRotationInObjectFrame(jointIndex);
    } else {
        return glm::quat();
    }
}

bool EntityScriptingInterface::setAbsoluteJointTranslationInObjectFrame(const QUuid& entityID,
                                                                        int jointIndex, glm::vec3 translation) {
    if (auto entity = checkForTreeEntityAndTypeMatch(entityID, EntityTypes::Model)) {
        auto now = usecTimestampNow();
        auto modelEntity = std::dynamic_pointer_cast<ModelEntityItem>(entity);
        bool result = modelEntity->setAbsoluteJointTranslationInObjectFrame(jointIndex, translation);
        if (result) {
            EntityItemProperties properties;
            _entityTree->withWriteLock([&] {
                properties = entity->getProperties();
                entity->setLastBroadcast(now);
            });

            properties.setJointTranslationsDirty();
            properties.setLastEdited(now);
            queueEntityMessage(PacketType::EntityEdit, entityID, properties);
            return true;
        }
    }
    return false;
}

bool EntityScriptingInterface::setAbsoluteJointRotationInObjectFrame(const QUuid& entityID,
                                                                     int jointIndex, glm::quat rotation) {
    if (auto entity = checkForTreeEntityAndTypeMatch(entityID, EntityTypes::Model)) {
        auto now = usecTimestampNow();
        auto modelEntity = std::dynamic_pointer_cast<ModelEntityItem>(entity);
        bool result = modelEntity->setAbsoluteJointRotationInObjectFrame(jointIndex, rotation);
        if (result) {
            EntityItemProperties properties;
            _entityTree->withWriteLock([&] {
                properties = entity->getProperties();
                entity->setLastBroadcast(now);
            });

            properties.setJointRotationsDirty();
            properties.setLastEdited(now);
            queueEntityMessage(PacketType::EntityEdit, entityID, properties);
            return true;
        }
    }
    return false;
}

glm::vec3 EntityScriptingInterface::getLocalJointTranslation(const QUuid& entityID, int jointIndex) {
    if (auto entity = checkForTreeEntityAndTypeMatch(entityID, EntityTypes::Model)) {
        auto modelEntity = std::dynamic_pointer_cast<ModelEntityItem>(entity);
        return modelEntity->getLocalJointTranslation(jointIndex);
    } else {
        return glm::vec3(0.0f);
    }
}

glm::quat EntityScriptingInterface::getLocalJointRotation(const QUuid& entityID, int jointIndex) {
    if (auto entity = checkForTreeEntityAndTypeMatch(entityID, EntityTypes::Model)) {
        auto modelEntity = std::dynamic_pointer_cast<ModelEntityItem>(entity);
        return modelEntity->getLocalJointRotation(jointIndex);
    } else {
        return glm::quat();
    }
}

bool EntityScriptingInterface::setLocalJointTranslation(const QUuid& entityID, int jointIndex, glm::vec3 translation) {
    if (auto entity = checkForTreeEntityAndTypeMatch(entityID, EntityTypes::Model)) {
        auto now = usecTimestampNow();
        auto modelEntity = std::dynamic_pointer_cast<ModelEntityItem>(entity);
        bool result = modelEntity->setLocalJointTranslation(jointIndex, translation);
        if (result) {
            EntityItemProperties properties;
            _entityTree->withWriteLock([&] {
                properties = entity->getProperties();
                entity->setLastBroadcast(now);
            });

            properties.setJointTranslationsDirty();
            properties.setLastEdited(now);
            queueEntityMessage(PacketType::EntityEdit, entityID, properties);
            return true;
        }
    }
    return false;
}

bool EntityScriptingInterface::setLocalJointRotation(const QUuid& entityID, int jointIndex, glm::quat rotation) {
    if (auto entity = checkForTreeEntityAndTypeMatch(entityID, EntityTypes::Model)) {
        auto now = usecTimestampNow();
        auto modelEntity = std::dynamic_pointer_cast<ModelEntityItem>(entity);
        bool result = modelEntity->setLocalJointRotation(jointIndex, rotation);
        if (result) {
            EntityItemProperties properties;
            _entityTree->withWriteLock([&] {
                properties = entity->getProperties();
                entity->setLastBroadcast(now);
            });

            properties.setJointRotationsDirty();
            properties.setLastEdited(now);
            queueEntityMessage(PacketType::EntityEdit, entityID, properties);
            return true;
        }
    }
    return false;
}



bool EntityScriptingInterface::setLocalJointRotations(const QUuid& entityID, const QVector<glm::quat>& rotations) {
    if (auto entity = checkForTreeEntityAndTypeMatch(entityID, EntityTypes::Model)) {
        auto now = usecTimestampNow();
        auto modelEntity = std::dynamic_pointer_cast<ModelEntityItem>(entity);

        bool result = false;
        for (int index = 0; index < rotations.size(); index++) {
            result |= modelEntity->setLocalJointRotation(index, rotations[index]);
        }
        if (result) {
            EntityItemProperties properties;
            _entityTree->withWriteLock([&] {
                entity->setLastEdited(now);
                entity->setLastBroadcast(now);
                properties = entity->getProperties();
            });

            properties.setJointRotationsDirty();
            properties.setLastEdited(now);
            queueEntityMessage(PacketType::EntityEdit, entityID, properties);
            return true;
        }
    }
    return false;
}


bool EntityScriptingInterface::setLocalJointTranslations(const QUuid& entityID, const QVector<glm::vec3>& translations) {
    if (auto entity = checkForTreeEntityAndTypeMatch(entityID, EntityTypes::Model)) {
        auto now = usecTimestampNow();
        auto modelEntity = std::dynamic_pointer_cast<ModelEntityItem>(entity);

        bool result = false;
        for (int index = 0; index < translations.size(); index++) {
            result |= modelEntity->setLocalJointTranslation(index, translations[index]);
        }
        if (result) {
            EntityItemProperties properties;
            _entityTree->withWriteLock([&] {
                entity->setLastEdited(now);
                entity->setLastBroadcast(now);
                properties = entity->getProperties();
            });

            properties.setJointTranslationsDirty();
            properties.setLastEdited(now);
            queueEntityMessage(PacketType::EntityEdit, entityID, properties);
            return true;
        }
    }
    return false;
}

bool EntityScriptingInterface::setLocalJointsData(const QUuid& entityID,
                                                  const QVector<glm::quat>& rotations,
                                                  const QVector<glm::vec3>& translations) {
    // for a model with 80 joints, sending both these in one edit packet causes the packet to be too large.
    return setLocalJointRotations(entityID, rotations) ||
        setLocalJointTranslations(entityID, translations);
}

int EntityScriptingInterface::getJointIndex(const QUuid& entityID, const QString& name) {
    if (!_entityTree) {
        return -1;
    }
    int result;
    QMetaObject::invokeMethod(_entityTree.get(), "getJointIndex", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(int, result), Q_ARG(QUuid, entityID), Q_ARG(QString, name));
    return result;
}

QStringList EntityScriptingInterface::getJointNames(const QUuid& entityID) {
    if (!_entityTree) {
        return QStringList();
    }
    QStringList result;
    QMetaObject::invokeMethod(_entityTree.get(), "getJointNames", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(QStringList, result), Q_ARG(QUuid, entityID));
    return result;
}

QVector<QUuid> EntityScriptingInterface::getChildrenIDs(const QUuid& parentID) {
    QVector<QUuid> result;
    if (!_entityTree) {
        return result;
    }

    EntityItemPointer entity = _entityTree->findEntityByEntityItemID(parentID);
    if (!entity) {
        qDebug() << "EntityScriptingInterface::getChildrenIDs - no entity with ID" << parentID;
        return result;
    }

    _entityTree->withReadLock([&] {
        entity->forEachChild([&](SpatiallyNestablePointer child) {
            result.push_back(child->getID());
        });
    });

    return result;
}

bool EntityScriptingInterface::isChildOfParent(QUuid childID, QUuid parentID) {
    bool isChild = false;

    if (!_entityTree) {
        return isChild;
    }

    _entityTree->withReadLock([&] {
        EntityItemPointer parent = _entityTree->findEntityByEntityItemID(parentID);
        parent->forEachDescendant([&](SpatiallyNestablePointer descendant) {
            if(descendant->getID() == childID) {
                isChild = true;
                return; 
            }
        });
    });
    
    return isChild;
}

QVector<QUuid> EntityScriptingInterface::getChildrenIDsOfJoint(const QUuid& parentID, int jointIndex) {
    QVector<QUuid> result;
    if (!_entityTree) {
        return result;
    }
    _entityTree->withReadLock([&] {
        QSharedPointer<SpatialParentFinder> parentFinder = DependencyManager::get<SpatialParentFinder>();
        if (!parentFinder) {
            return;
        }
        bool success;
        SpatiallyNestableWeakPointer parentWP = parentFinder->find(parentID, success);
        if (!success) {
            return;
        }
        SpatiallyNestablePointer parent = parentWP.lock();
        if (!parent) {
            return;
        }
        parent->forEachChild([&](SpatiallyNestablePointer child) {
            if (child->getParentJointIndex() == jointIndex) {
                result.push_back(child->getID());
            }
        });
    });
    return result;
}

QUuid EntityScriptingInterface::getKeyboardFocusEntity() const {
    QUuid result;
    QMetaObject::invokeMethod(qApp, "getKeyboardFocusEntity", Qt::DirectConnection, Q_RETURN_ARG(QUuid, result));
    return result;
}

void EntityScriptingInterface::setKeyboardFocusEntity(QUuid id) {
    QMetaObject::invokeMethod(qApp, "setKeyboardFocusEntity", Qt::QueuedConnection, Q_ARG(QUuid, id));
}

void EntityScriptingInterface::sendMousePressOnEntity(QUuid id, PointerEvent event) {
    QMetaObject::invokeMethod(qApp, "sendMousePressOnEntity", Qt::QueuedConnection, Q_ARG(QUuid, id), Q_ARG(PointerEvent, event));
}

void EntityScriptingInterface::sendMouseMoveOnEntity(QUuid id, PointerEvent event) {
    QMetaObject::invokeMethod(qApp, "sendMouseMoveOnEntity", Qt::QueuedConnection, Q_ARG(QUuid, id), Q_ARG(PointerEvent, event));
}

void EntityScriptingInterface::sendMouseReleaseOnEntity(QUuid id, PointerEvent event) {
    QMetaObject::invokeMethod(qApp, "sendMouseReleaseOnEntity", Qt::QueuedConnection, Q_ARG(QUuid, id), Q_ARG(PointerEvent, event));
}

void EntityScriptingInterface::sendClickDownOnEntity(QUuid id, PointerEvent event) {
    QMetaObject::invokeMethod(qApp, "sendClickDownOnEntity", Qt::QueuedConnection, Q_ARG(QUuid, id), Q_ARG(PointerEvent, event));
}

void EntityScriptingInterface::sendHoldingClickOnEntity(QUuid id, PointerEvent event) {
    QMetaObject::invokeMethod(qApp, "sendHoldingClickOnEntity", Qt::QueuedConnection, Q_ARG(QUuid, id), Q_ARG(PointerEvent, event));
}

void EntityScriptingInterface::sendClickReleaseOnEntity(QUuid id, PointerEvent event) {
    QMetaObject::invokeMethod(qApp, "sendClickReleaseOnEntity", Qt::QueuedConnection, Q_ARG(QUuid, id), Q_ARG(PointerEvent, event));
}

void EntityScriptingInterface::sendHoverEnterEntity(QUuid id, PointerEvent event) {
    QMetaObject::invokeMethod(qApp, "sendHoverEnterEntity", Qt::QueuedConnection, Q_ARG(QUuid, id), Q_ARG(PointerEvent, event));
}

void EntityScriptingInterface::sendHoverOverEntity(QUuid id, PointerEvent event) {
    QMetaObject::invokeMethod(qApp, "sendHoverOverEntity", Qt::QueuedConnection, Q_ARG(QUuid, id), Q_ARG(PointerEvent, event));
}

void EntityScriptingInterface::sendHoverLeaveEntity(QUuid id, PointerEvent event) {
    QMetaObject::invokeMethod(qApp, "sendHoverLeaveEntity", Qt::QueuedConnection, Q_ARG(QUuid, id), Q_ARG(PointerEvent, event));
}

bool EntityScriptingInterface::wantsHandControllerPointerEvents(QUuid id) {
    bool result = false;
    if (_entityTree) {
        _entityTree->withReadLock([&] {
            EntityItemPointer entity = _entityTree->findEntityByEntityItemID(EntityItemID(id));
            if (entity) {
                result = entity->wantsHandControllerPointerEvents();
            }
        });
    }
    return result;
}

void EntityScriptingInterface::emitScriptEvent(const EntityItemID& entityID, const QVariant& message) {
    if (_entityTree) {
        _entityTree->withReadLock([&] {
            EntityItemPointer entity = _entityTree->findEntityByEntityItemID(EntityItemID(entityID));
            if (entity) {
                entity->emitScriptEvent(message);
            }
        });
    }
}

float EntityScriptingInterface::calculateCost(float mass, float oldVelocity, float newVelocity) {
    return std::abs(mass * (newVelocity - oldVelocity));
}

void EntityScriptingInterface::setCurrentAvatarEnergy(float energy) {
  //  qCDebug(entities) << "NEW AVATAR ENERGY IN ENTITY SCRIPTING INTERFACE: " << energy;
    _currentAvatarEnergy = energy;
}

float EntityScriptingInterface::getCostMultiplier() {
    return costMultiplier;
}

void EntityScriptingInterface::setCostMultiplier(float value) {
    costMultiplier = value;
}

QObject* EntityScriptingInterface::getWebViewRoot(const QUuid& entityID) {
    if (auto entity = checkForTreeEntityAndTypeMatch(entityID, EntityTypes::Web)) {
        auto webEntity = std::dynamic_pointer_cast<WebEntityItem>(entity);
        QObject* root = webEntity->getRootItem();
        return root;
    } else {
        return nullptr;
    }
}
