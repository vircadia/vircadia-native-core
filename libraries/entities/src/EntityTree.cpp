//
//  EntityTree.cpp
//  libraries/entities/src
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <PerfStat.h>
#include <QDateTime>
#include <QtScript/QScriptEngine>

#include "EntityTree.h"
#include "EntitySimulation.h"
#include "VariantMapToScriptValue.h"

#include "AddEntityOperator.h"
#include "MovingEntitiesOperator.h"
#include "UpdateEntityOperator.h"
#include "QVariantGLM.h"
#include "EntitiesLogging.h"
#include "RecurseOctreeToMapOperator.h"
#include "LogHandler.h"

static const quint64 DELETED_ENTITIES_EXTRA_USECS_TO_CONSIDER = USECS_PER_MSEC * 50;

EntityTree::EntityTree(bool shouldReaverage) :
    Octree(shouldReaverage),
    _fbxService(NULL),
    _simulation(NULL)
{
    resetClientEditStats();
}

EntityTree::~EntityTree() {
    eraseAllOctreeElements(false);
}

void EntityTree::createRootElement() {
    _rootElement = createNewElement();
}

OctreeElementPointer EntityTree::createNewElement(unsigned char* octalCode) {
    EntityTreeElementPointer newElement = EntityTreeElementPointer(new EntityTreeElement(octalCode),
                                                                   // see comment int EntityTreeElement::createNewElement
                                                                   [=](EntityTreeElement* dyingElement) {
                                                                       EntityTreeElementPointer tmpSharedPointer(dyingElement);
                                                                       dyingElement->notifyDeleteHooks();
                                                                   });
    newElement->setTree(std::static_pointer_cast<EntityTree>(shared_from_this()));
    return std::static_pointer_cast<OctreeElement>(newElement);
}

void EntityTree::eraseAllOctreeElements(bool createNewRoot) {
    emit clearingEntities();

    // this would be a good place to clean up our entities...
    if (_simulation) {
        _simulation->clearEntities();
    }
    foreach (EntityTreeElementPointer element, _entityToElementMap) {
        element->cleanupEntities();
    }
    _entityToElementMap.clear();
    Octree::eraseAllOctreeElements(createNewRoot);

    resetClientEditStats();
}

bool EntityTree::handlesEditPacketType(PacketType packetType) const {
    // we handle these types of "edit" packets
    switch (packetType) {
        case PacketType::EntityAdd:
        case PacketType::EntityEdit:
        case PacketType::EntityErase:
            return true;
        default:
            return false;
    }
}

/// Adds a new entity item to the tree
void EntityTree::postAddEntity(EntityItemPointer entity) {
    assert(entity);
    // check to see if we need to simulate this entity..
    if (_simulation) {
        _simulation->addEntity(entity);
    }
    _isDirty = true;
    maybeNotifyNewCollisionSoundURL("", entity->getCollisionSoundURL());
    emit addingEntity(entity->getEntityItemID());
}

bool EntityTree::updateEntity(const EntityItemID& entityID, const EntityItemProperties& properties, const SharedNodePointer& senderNode) {
    EntityTreeElementPointer containingElement = getContainingElement(entityID);
    if (!containingElement) {
        return false;
    }

    EntityItemPointer existingEntity = containingElement->getEntityWithEntityItemID(entityID);
    if (!existingEntity) {
        return false;
    }

    return updateEntityWithElement(existingEntity, properties, containingElement, senderNode);
}

bool EntityTree::updateEntity(EntityItemPointer entity, const EntityItemProperties& properties, const SharedNodePointer& senderNode) {
    EntityTreeElementPointer containingElement = getContainingElement(entity->getEntityItemID());
    if (!containingElement) {
        return false;
    }
    return updateEntityWithElement(entity, properties, containingElement, senderNode);
}

bool EntityTree::updateEntityWithElement(EntityItemPointer entity, const EntityItemProperties& origProperties,
                                         EntityTreeElementPointer containingElement, const SharedNodePointer& senderNode) {
    EntityItemProperties properties = origProperties;

    bool allowLockChange;
    QUuid senderID;
    if (senderNode.isNull()) {
        auto nodeList = DependencyManager::get<NodeList>();
        allowLockChange = nodeList->getThisNodeCanAdjustLocks();
        senderID = nodeList->getSessionUUID();
    } else {
        allowLockChange = senderNode->getCanAdjustLocks();
        senderID = senderNode->getUUID();
    }

    if (!allowLockChange && (entity->getLocked() != properties.getLocked())) {
        qCDebug(entities) << "Refusing disallowed lock adjustment.";
        return false;
    }

    // enforce support for locked entities. If an entity is currently locked, then the only
    // property we allow you to change is the locked property.
    if (entity->getLocked()) {
        if (properties.lockedChanged()) {
            bool wantsLocked = properties.getLocked();
            if (!wantsLocked) {
                EntityItemProperties tempProperties;
                tempProperties.setLocked(wantsLocked);
                UpdateEntityOperator theOperator(getThisPointer(), containingElement, entity, tempProperties);
                recurseTreeWithOperator(&theOperator);
                _isDirty = true;
            }
        }
    } else {
        if (getIsServer()) {
            bool simulationBlocked = !entity->getSimulatorID().isNull();
            if (properties.simulationOwnerChanged()) {
                QUuid submittedID = properties.getSimulationOwner().getID();
                // a legit interface will only submit their own ID or NULL:
                if (submittedID.isNull()) {
                    if (entity->getSimulatorID() == senderID) {
                        // We only allow the simulation owner to clear their own simulationID's.
                        simulationBlocked = false;
                        properties.clearSimulationOwner(); // clear everything
                    }
                    // else: We assume the sender really did believe it was the simulation owner when it sent
                } else if (submittedID == senderID) {
                    // the sender is trying to take or continue ownership
                    if (entity->getSimulatorID().isNull()) {
                        // the sender it taking ownership
                        properties.promoteSimulationPriority(RECRUIT_SIMULATION_PRIORITY);
                        simulationBlocked = false;
                    } else if (entity->getSimulatorID() == senderID) {
                        // the sender is asserting ownership
                        simulationBlocked = false;
                    } else {
                        // the sender is trying to steal ownership from another simulator
                        // so we apply the rules for ownership change:
                        // (1) higher priority wins
                        // (2) equal priority wins if ownership filter has expired except...
                        uint8_t oldPriority = entity->getSimulationPriority();
                        uint8_t newPriority = properties.getSimulationOwner().getPriority();
                        if (newPriority > oldPriority ||
                             (newPriority == oldPriority && properties.getSimulationOwner().hasExpired())) {
                            simulationBlocked = false;
                        }
                    }
                } else {
                    // the entire update is suspect --> ignore it
                    return false;
                }
            } else {
                simulationBlocked = senderID != entity->getSimulatorID();
            }
            if (simulationBlocked) {
                // squash ownership and physics-related changes.
                properties.setSimulationOwnerChanged(false);
                properties.setPositionChanged(false);
                properties.setRotationChanged(false);
                properties.setVelocityChanged(false);
                properties.setAngularVelocityChanged(false);
                properties.setAccelerationChanged(false);
            }
        }
        // else client accepts what the server says

        QString entityScriptBefore = entity->getScript();
        quint64 entityScriptTimestampBefore = entity->getScriptTimestamp();
        QString collisionSoundURLBefore = entity->getCollisionSoundURL();
        uint32_t preFlags = entity->getDirtyFlags();
        UpdateEntityOperator theOperator(getThisPointer(), containingElement, entity, properties);
        recurseTreeWithOperator(&theOperator);
        _isDirty = true;

        uint32_t newFlags = entity->getDirtyFlags() & ~preFlags;
        if (newFlags) {
            if (_simulation) {
                if (newFlags & DIRTY_SIMULATION_FLAGS) {
                    _simulation->changeEntity(entity);
                }
            } else {
                // normally the _simulation clears ALL updateFlags, but since there is none we do it explicitly
                entity->clearDirtyFlags();
            }
        }

        QString entityScriptAfter = entity->getScript();
        quint64 entityScriptTimestampAfter = entity->getScriptTimestamp();
        bool reload = entityScriptTimestampBefore != entityScriptTimestampAfter;
        if (entityScriptBefore != entityScriptAfter || reload) {
            emitEntityScriptChanging(entity->getEntityItemID(), reload); // the entity script has changed
        }
        maybeNotifyNewCollisionSoundURL(collisionSoundURLBefore, entity->getCollisionSoundURL());
     }

    // TODO: this final containingElement check should eventually be removed (or wrapped in an #ifdef DEBUG).
    containingElement = getContainingElement(entity->getEntityItemID());
    if (!containingElement) {
        qCDebug(entities) << "UNEXPECTED!!!! after updateEntity() we no longer have a containing element??? entityID="
                << entity->getEntityItemID();
        return false;
    }

    return true;
}

EntityItemPointer EntityTree::addEntity(const EntityItemID& entityID, const EntityItemProperties& properties) {
    EntityItemPointer result = NULL;

    if (getIsClient()) {
        // if our Node isn't allowed to create entities in this domain, don't try.
        auto nodeList = DependencyManager::get<NodeList>();
        if (nodeList && !nodeList->getThisNodeCanRez()) {
            return NULL;
        }
    }

    bool recordCreationTime = false;
    if (properties.getCreated() == UNKNOWN_CREATED_TIME) {
        // the entity's creation time was not specified in properties, which means this is a NEW entity
        // and we must record its creation time
        recordCreationTime = true;
    }

    // You should not call this on existing entities that are already part of the tree! Call updateEntity()
    EntityTreeElementPointer containingElement = getContainingElement(entityID);
    if (containingElement) {
        qCDebug(entities) << "UNEXPECTED!!! ----- don't call addEntity() on existing entity items. entityID=" << entityID
                          << "containingElement=" << containingElement.get();
        return result;
    }

    // construct the instance of the entity
    EntityTypes::EntityType type = properties.getType();
    result = EntityTypes::constructEntityItem(type, entityID, properties);

    if (result) {
        if (recordCreationTime) {
            result->recordCreationTime();
        }
        // Recurse the tree and store the entity in the correct tree element
        AddEntityOperator theOperator(getThisPointer(), result);
        recurseTreeWithOperator(&theOperator);

        postAddEntity(result);
    }
    return result;
}

void EntityTree::emitEntityScriptChanging(const EntityItemID& entityItemID, const bool reload) {
    emit entityScriptChanging(entityItemID, reload);
}

void EntityTree::maybeNotifyNewCollisionSoundURL(const QString& previousCollisionSoundURL, const QString& nextCollisionSoundURL) {
    if (!nextCollisionSoundURL.isEmpty() && (nextCollisionSoundURL != previousCollisionSoundURL)) {
        emit newCollisionSoundURL(QUrl(nextCollisionSoundURL));
    }
}

void EntityTree::setSimulation(EntitySimulation* simulation) {
    this->withWriteLock([&] {
        if (simulation) {
            // assert that the simulation's backpointer has already been properly connected
            assert(simulation->getEntityTree().get() == this);
        }
        if (_simulation && _simulation != simulation) {
            // It's important to clearEntities() on the simulation since taht will update each
            // EntityItem::_simulationState correctly so as to not confuse the next _simulation.
            _simulation->clearEntities();
        }
        _simulation = simulation;
    });
}

void EntityTree::deleteEntity(const EntityItemID& entityID, bool force, bool ignoreWarnings) {
    EntityTreeElementPointer containingElement = getContainingElement(entityID);
    if (!containingElement) {
        if (!ignoreWarnings) {
            qCDebug(entities) << "UNEXPECTED!!!!  EntityTree::deleteEntity() entityID doesn't exist!!! entityID=" << entityID;
        }
        return;
    }

    EntityItemPointer existingEntity = containingElement->getEntityWithEntityItemID(entityID);
    if (!existingEntity) {
        if (!ignoreWarnings) {
            qCDebug(entities) << "UNEXPECTED!!!! don't call EntityTree::deleteEntity() on entity items that don't exist. "
                        "entityID=" << entityID;
        }
        return;
    }

    if (existingEntity->getLocked() && !force) {
        if (!ignoreWarnings) {
            qCDebug(entities) << "ERROR! EntityTree::deleteEntity() trying to delete locked entity. entityID=" << entityID;
        }
        return;
    }

    emit deletingEntity(entityID);

    // NOTE: callers must lock the tree before using this method
    DeleteEntityOperator theOperator(getThisPointer(), entityID);
    recurseTreeWithOperator(&theOperator);
    processRemovedEntities(theOperator);
    _isDirty = true;
}

void EntityTree::deleteEntities(QSet<EntityItemID> entityIDs, bool force, bool ignoreWarnings) {
    // NOTE: callers must lock the tree before using this method
    DeleteEntityOperator theOperator(getThisPointer());
    foreach(const EntityItemID& entityID, entityIDs) {
        EntityTreeElementPointer containingElement = getContainingElement(entityID);
        if (!containingElement) {
            if (!ignoreWarnings) {
                qCDebug(entities) << "UNEXPECTED!!!!  EntityTree::deleteEntities() entityID doesn't exist!!! entityID=" << entityID;
            }
            continue;
        }

        EntityItemPointer existingEntity = containingElement->getEntityWithEntityItemID(entityID);
        if (!existingEntity) {
            if (!ignoreWarnings) {
                qCDebug(entities) << "UNEXPECTED!!!! don't call EntityTree::deleteEntities() on entity items that don't exist. "
                            "entityID=" << entityID;
            }
            continue;
        }

        if (existingEntity->getLocked() && !force) {
            if (!ignoreWarnings) {
                qCDebug(entities) << "ERROR! EntityTree::deleteEntities() trying to delete locked entity. entityID=" << entityID;
            }
            continue;
        }

        // tell our delete operator about this entityID
        theOperator.addEntityIDToDeleteList(entityID);
        emit deletingEntity(entityID);
    }

    if (theOperator.getEntities().size() > 0) {
        recurseTreeWithOperator(&theOperator);
        processRemovedEntities(theOperator);
        _isDirty = true;
    }
}

void EntityTree::processRemovedEntities(const DeleteEntityOperator& theOperator) {
    quint64 deletedAt = usecTimestampNow();
    const RemovedEntities& entities = theOperator.getEntities();
    foreach(const EntityToDeleteDetails& details, entities) {
        EntityItemPointer theEntity = details.entity;

        if (getIsServer()) {
            // set up the deleted entities ID
            QWriteLocker locker(&_recentlyDeletedEntitiesLock);
            _recentlyDeletedEntityItemIDs.insert(deletedAt, theEntity->getEntityItemID());
        }

        if (_simulation) {
            theEntity->clearActions(_simulation);
            _simulation->removeEntity(theEntity);
        }
    }
}


class FindNearPointArgs {
public:
    glm::vec3 position;
    float targetRadius;
    bool found;
    EntityItemPointer closestEntity;
    float closestEntityDistance;
};


bool EntityTree::findNearPointOperation(OctreeElementPointer element, void* extraData) {
    FindNearPointArgs* args = static_cast<FindNearPointArgs*>(extraData);
    EntityTreeElementPointer entityTreeElement = std::static_pointer_cast<EntityTreeElement>(element);

    glm::vec3 penetration;
    bool sphereIntersection = entityTreeElement->getAACube().findSpherePenetration(args->position, args->targetRadius, penetration);

    // If this entityTreeElement contains the point, then search it...
    if (sphereIntersection) {
        EntityItemPointer thisClosestEntity = entityTreeElement->getClosestEntity(args->position);

        // we may have gotten NULL back, meaning no entity was available
        if (thisClosestEntity) {
            glm::vec3 entityPosition = thisClosestEntity->getPosition();
            float distanceFromPointToEntity = glm::distance(entityPosition, args->position);

            // If we're within our target radius
            if (distanceFromPointToEntity <= args->targetRadius) {
                // we are closer than anything else we've found
                if (distanceFromPointToEntity < args->closestEntityDistance) {
                    args->closestEntity = thisClosestEntity;
                    args->closestEntityDistance = distanceFromPointToEntity;
                    args->found = true;
                }
            }
        }

        // we should be able to optimize this...
        return true; // keep searching in case children have closer entities
    }

    // if this element doesn't contain the point, then none of its children can contain the point, so stop searching
    return false;
}
// combines the ray cast arguments into a single object
class RayArgs {
public:
    glm::vec3 origin;
    glm::vec3 direction;
    OctreeElementPointer& element;
    float& distance;
    BoxFace& face;
    glm::vec3& surfaceNormal;
    const QVector<EntityItemID>& entityIdsToInclude;
    void** intersectedObject;
    bool found;
    bool precisionPicking;
};


bool findRayIntersectionOp(OctreeElementPointer element, void* extraData) {
    RayArgs* args = static_cast<RayArgs*>(extraData);
    bool keepSearching = true;
    EntityTreeElementPointer entityTreeElementPointer = std::dynamic_pointer_cast<EntityTreeElement>(element);
    if (entityTreeElementPointer ->findRayIntersection(args->origin, args->direction, keepSearching,
        args->element, args->distance, args->face, args->surfaceNormal, args->entityIdsToInclude,
        args->intersectedObject, args->precisionPicking)) {
        args->found = true;
    }
    return keepSearching;
}

bool EntityTree::findRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                                    OctreeElementPointer& element, float& distance, 
                                    BoxFace& face, glm::vec3& surfaceNormal, const QVector<EntityItemID>& entityIdsToInclude, void** intersectedObject,
                                    Octree::lockType lockType, bool* accurateResult, bool precisionPicking) {
    RayArgs args = { origin, direction, element, distance, face, surfaceNormal, entityIdsToInclude, intersectedObject, false, precisionPicking };
    distance = FLT_MAX;

    bool requireLock = lockType == Octree::Lock;
    bool lockResult = withReadLock([&]{
        recurseTreeWithOperation(findRayIntersectionOp, &args);
    }, requireLock);

    if (accurateResult) {
        *accurateResult = lockResult; // if user asked to accuracy or result, let them know this is accurate
    }

    return args.found;
}


EntityItemPointer EntityTree::findClosestEntity(glm::vec3 position, float targetRadius) {
    FindNearPointArgs args = { position, targetRadius, false, NULL, FLT_MAX };
    withReadLock([&] {
        // NOTE: This should use recursion, since this is a spatial operation
        recurseTreeWithOperation(findNearPointOperation, &args);
    });
    return args.closestEntity;
}

class FindAllNearPointArgs {
public:
    glm::vec3 position;
    float targetRadius;
    QVector<EntityItemPointer> entities;
};


bool EntityTree::findInSphereOperation(OctreeElementPointer element, void* extraData) {
    FindAllNearPointArgs* args = static_cast<FindAllNearPointArgs*>(extraData);
    glm::vec3 penetration;
    bool sphereIntersection = element->getAACube().findSpherePenetration(args->position, args->targetRadius, penetration);

    // If this element contains the point, then search it...
    if (sphereIntersection) {
        EntityTreeElementPointer entityTreeElement = std::static_pointer_cast<EntityTreeElement>(element);
        entityTreeElement->getEntities(args->position, args->targetRadius, args->entities);
        return true; // keep searching in case children have closer entities
    }

    // if this element doesn't contain the point, then none of it's children can contain the point, so stop searching
    return false;
}

// NOTE: assumes caller has handled locking
void EntityTree::findEntities(const glm::vec3& center, float radius, QVector<EntityItemPointer>& foundEntities) {
    FindAllNearPointArgs args = { center, radius, QVector<EntityItemPointer>() };
    // NOTE: This should use recursion, since this is a spatial operation
    recurseTreeWithOperation(findInSphereOperation, &args);

    // swap the two lists of entity pointers instead of copy
    foundEntities.swap(args.entities);
}

class FindEntitiesInCubeArgs {
public:
    FindEntitiesInCubeArgs(const AACube& cube)
        : _cube(cube), _foundEntities() {
    }

    AACube _cube;
    QVector<EntityItemPointer> _foundEntities;
};

bool EntityTree::findInCubeOperation(OctreeElementPointer element, void* extraData) {
    FindEntitiesInCubeArgs* args = static_cast<FindEntitiesInCubeArgs*>(extraData);
    if (element->getAACube().touches(args->_cube)) {
        EntityTreeElementPointer entityTreeElement = std::static_pointer_cast<EntityTreeElement>(element);
        entityTreeElement->getEntities(args->_cube, args->_foundEntities);
        return true;
    }
    return false;
}

// NOTE: assumes caller has handled locking
void EntityTree::findEntities(const AACube& cube, QVector<EntityItemPointer>& foundEntities) {
    FindEntitiesInCubeArgs args(cube);
    // NOTE: This should use recursion, since this is a spatial operation
    recurseTreeWithOperation(findInCubeOperation, &args);
    // swap the two lists of entity pointers instead of copy
    foundEntities.swap(args._foundEntities);
}

class FindEntitiesInBoxArgs {
public:
    FindEntitiesInBoxArgs(const AABox& box)
    : _box(box), _foundEntities() {
    }

    AABox _box;
    QVector<EntityItemPointer> _foundEntities;
};

bool EntityTree::findInBoxOperation(OctreeElementPointer element, void* extraData) {
    FindEntitiesInBoxArgs* args = static_cast<FindEntitiesInBoxArgs*>(extraData);
    if (element->getAACube().touches(args->_box)) {
        EntityTreeElementPointer entityTreeElement = std::static_pointer_cast<EntityTreeElement>(element);
        entityTreeElement->getEntities(args->_box, args->_foundEntities);
        return true;
    }
    return false;
}

// NOTE: assumes caller has handled locking
void EntityTree::findEntities(const AABox& box, QVector<EntityItemPointer>& foundEntities) {
    FindEntitiesInBoxArgs args(box);
    // NOTE: This should use recursion, since this is a spatial operation
    recurseTreeWithOperation(findInBoxOperation, &args);
    // swap the two lists of entity pointers instead of copy
    foundEntities.swap(args._foundEntities);
}

EntityItemPointer EntityTree::findEntityByID(const QUuid& id) {
    EntityItemID entityID(id);
    return findEntityByEntityItemID(entityID);
}

EntityItemPointer EntityTree::findEntityByEntityItemID(const EntityItemID& entityID) /*const*/ {
    EntityItemPointer foundEntity = NULL;
    EntityTreeElementPointer containingElement = getContainingElement(entityID);
    if (containingElement) {
        foundEntity = containingElement->getEntityWithEntityItemID(entityID);
    }
    return foundEntity;
}

void EntityTree::fixupTerseEditLogging(EntityItemProperties& properties, QList<QString>& changedProperties) {
    if (properties.simulationOwnerChanged()) {
        int simIndex = changedProperties.indexOf("simulationOwner");
        if (simIndex >= 0) {
            SimulationOwner simOwner = properties.getSimulationOwner();
            changedProperties[simIndex] = QString("simulationOwner:") + QString::number((int)simOwner.getPriority());
        }
    }
}

int EntityTree::processEditPacketData(NLPacket& packet, const unsigned char* editData, int maxLength,
                                     const SharedNodePointer& senderNode) {

    if (!getIsServer()) {
        qCDebug(entities) << "UNEXPECTED!!! processEditPacketData() should only be called on a server tree.";
        return 0;
    }

    int processedBytes = 0;
    // we handle these types of "edit" packets
    switch (packet.getType()) {
        case PacketType::EntityErase: {
            QByteArray dataByteArray = QByteArray::fromRawData(reinterpret_cast<const char*>(editData), maxLength);
            processedBytes = processEraseMessageDetails(dataByteArray, senderNode);
            break;
        }

        case PacketType::EntityAdd:
        case PacketType::EntityEdit: {
            quint64 startDecode = 0, endDecode = 0;
            quint64 startLookup = 0, endLookup = 0;
            quint64 startUpdate = 0, endUpdate = 0;
            quint64 startCreate = 0, endCreate = 0;
            quint64 startLogging = 0, endLogging = 0;

            _totalEditMessages++;

            EntityItemID entityItemID;
            EntityItemProperties properties;
            startDecode = usecTimestampNow();
           
            bool validEditPacket = EntityItemProperties::decodeEntityEditPacket(editData, maxLength, processedBytes,
                                                                                entityItemID, properties);
            endDecode = usecTimestampNow();

            // If we got a valid edit packet, then it could be a new entity or it could be an update to
            // an existing entity... handle appropriately
            if (validEditPacket) {
                // search for the entity by EntityItemID
                startLookup = usecTimestampNow();
                EntityItemPointer existingEntity = findEntityByEntityItemID(entityItemID);
                endLookup = usecTimestampNow();
                if (existingEntity && packet.getType() == PacketType::EntityEdit) {
                    // if the EntityItem exists, then update it
                    startLogging = usecTimestampNow();
                    if (wantEditLogging()) {
                        qCDebug(entities) << "User [" << senderNode->getUUID() << "] editing entity. ID:" << entityItemID;
                        qCDebug(entities) << "   properties:" << properties;
                    }
                    if (wantTerseEditLogging()) {
                        QList<QString> changedProperties = properties.listChangedProperties();
                        fixupTerseEditLogging(properties, changedProperties);
                        qCDebug(entities) << "edit" << entityItemID.toString() << changedProperties;
                    }
                    endLogging = usecTimestampNow();

                    startUpdate = usecTimestampNow();
                    updateEntity(entityItemID, properties, senderNode);
                    existingEntity->markAsChangedOnServer();
                    endUpdate = usecTimestampNow();
                    _totalUpdates++;
                } else if (packet.getType() == PacketType::EntityAdd) {
                    if (senderNode->getCanRez()) {
                        // this is a new entity... assign a new entityID
                        properties.setCreated(properties.getLastEdited());
                        startCreate = usecTimestampNow();
                        EntityItemPointer newEntity = addEntity(entityItemID, properties);
                        endCreate = usecTimestampNow();
                        _totalCreates++;
                        if (newEntity) {
                            newEntity->markAsChangedOnServer();
                            notifyNewlyCreatedEntity(*newEntity, senderNode);

                            startLogging = usecTimestampNow();
                            if (wantEditLogging()) {
                                qCDebug(entities) << "User [" << senderNode->getUUID() << "] added entity. ID:"
                                                << newEntity->getEntityItemID();
                                qCDebug(entities) << "   properties:" << properties;
                            }
                            if (wantTerseEditLogging()) {
                                QList<QString> changedProperties = properties.listChangedProperties();
                                fixupTerseEditLogging(properties, changedProperties);
                                qCDebug(entities) << "add" << entityItemID.toString() << changedProperties;
                            }
                            endLogging = usecTimestampNow();

                        }
                    } else {
                        qCDebug(entities) << "User without 'rez rights' [" << senderNode->getUUID()
                                          << "] attempted to add an entity.";
                    }
                } else {
                    static QString repeatedMessage =
                        LogHandler::getInstance().addRepeatedMessageRegex("^Edit failed.*");
                    qCDebug(entities) << "Edit failed. [" << packet.getType() <<"] " <<
                            "entity id:" << entityItemID << 
                            "existingEntity pointer:" << existingEntity.get();
                }
            }


            _totalDecodeTime += endDecode - startDecode;
            _totalLookupTime += endLookup - startLookup;
            _totalUpdateTime += endUpdate - startUpdate;
            _totalCreateTime += endCreate - startCreate;
            _totalLoggingTime += endLogging - startLogging;

            break;
        }

        default:
            processedBytes = 0;
            break;
    }
    return processedBytes;
}


void EntityTree::notifyNewlyCreatedEntity(const EntityItem& newEntity, const SharedNodePointer& senderNode) {
    _newlyCreatedHooksLock.lockForRead();
    for (int i = 0; i < _newlyCreatedHooks.size(); i++) {
        _newlyCreatedHooks[i]->entityCreated(newEntity, senderNode);
    }
    _newlyCreatedHooksLock.unlock();
}

void EntityTree::addNewlyCreatedHook(NewlyCreatedEntityHook* hook) {
    _newlyCreatedHooksLock.lockForWrite();
    _newlyCreatedHooks.push_back(hook);
    _newlyCreatedHooksLock.unlock();
}

void EntityTree::removeNewlyCreatedHook(NewlyCreatedEntityHook* hook) {
    _newlyCreatedHooksLock.lockForWrite();
    for (int i = 0; i < _newlyCreatedHooks.size(); i++) {
        if (_newlyCreatedHooks[i] == hook) {
            _newlyCreatedHooks.erase(_newlyCreatedHooks.begin() + i);
            break;
        }
    }
    _newlyCreatedHooksLock.unlock();
}


void EntityTree::releaseSceneEncodeData(OctreeElementExtraEncodeData* extraEncodeData) const {
    for (auto extraData : extraEncodeData->values()) {
        EntityTreeElementExtraEncodeData* thisExtraEncodeData = static_cast<EntityTreeElementExtraEncodeData*>(extraData);
        delete thisExtraEncodeData;
    }
    extraEncodeData->clear();
}

void EntityTree::entityChanged(EntityItemPointer entity) {
    if (_simulation) {
        _simulation->changeEntity(entity);
    }
}

void EntityTree::update() {
    if (_simulation) {
        withWriteLock([&] {
            _simulation->updateEntities();
            VectorOfEntities pendingDeletes;
            _simulation->getEntitiesToDelete(pendingDeletes);

            if (pendingDeletes.size() > 0) {
                // translate into list of ID's
                QSet<EntityItemID> idsToDelete;

                for (auto entity : pendingDeletes) {
                    idsToDelete.insert(entity->getEntityItemID());
                }

                // delete these things the roundabout way
                deleteEntities(idsToDelete, true);
            }
        });
    }
}

quint64 EntityTree::getAdjustedConsiderSince(quint64 sinceTime) {
    return (sinceTime - DELETED_ENTITIES_EXTRA_USECS_TO_CONSIDER);
}


bool EntityTree::hasEntitiesDeletedSince(quint64 sinceTime) {
    quint64 considerEntitiesSince = getAdjustedConsiderSince(sinceTime);

    // we can probably leverage the ordered nature of QMultiMap to do this quickly...
    bool hasSomethingNewer = false;

    QReadLocker locker(&_recentlyDeletedEntitiesLock);
    QMultiMap<quint64, QUuid>::const_iterator iterator = _recentlyDeletedEntityItemIDs.constBegin();
    while (iterator != _recentlyDeletedEntityItemIDs.constEnd()) {
        if (iterator.key() > considerEntitiesSince) {
            hasSomethingNewer = true;
            break; // if we have at least one item, we don't need to keep searching
        }
        ++iterator;
    }

#ifdef EXTRA_ERASE_DEBUGGING
    if (hasSomethingNewer) {
        int elapsed = usecTimestampNow() - considerEntitiesSince;
        int difference = considerEntitiesSince - sinceTime;
        qDebug() << "EntityTree::hasEntitiesDeletedSince() sinceTime:" << sinceTime 
                    << "considerEntitiesSince:" << considerEntitiesSince << "elapsed:" << elapsed << "difference:" << difference;
    }
#endif

    return hasSomethingNewer;
}

// called by the server when it knows all nodes have been sent deleted packets
void EntityTree::forgetEntitiesDeletedBefore(quint64 sinceTime) {
    quint64 considerSinceTime = sinceTime - DELETED_ENTITIES_EXTRA_USECS_TO_CONSIDER;
    QSet<quint64> keysToRemove;
    QWriteLocker locker(&_recentlyDeletedEntitiesLock);
    QMultiMap<quint64, QUuid>::iterator iterator = _recentlyDeletedEntityItemIDs.begin();

    // First find all the keys in the map that are older and need to be deleted
    while (iterator != _recentlyDeletedEntityItemIDs.end()) {
        if (iterator.key() <= considerSinceTime) {
            keysToRemove << iterator.key();
        }
        ++iterator;
    }

    // Now run through the keysToRemove and remove them
    foreach (quint64 value, keysToRemove) {
        _recentlyDeletedEntityItemIDs.remove(value);
    }
}


// TODO: consider consolidating processEraseMessageDetails() and processEraseMessage()
int EntityTree::processEraseMessage(NLPacket& packet, const SharedNodePointer& sourceNode) {
    #ifdef EXTRA_ERASE_DEBUGGING
        qDebug() << "EntityTree::processEraseMessage()";
    #endif
    withWriteLock([&] {
        packet.seek(sizeof(OCTREE_PACKET_FLAGS) + sizeof(OCTREE_PACKET_SEQUENCE) + sizeof(OCTREE_PACKET_SENT_TIME));

        uint16_t numberOfIDs = 0; // placeholder for now
        packet.readPrimitive(&numberOfIDs);

        if (numberOfIDs > 0) {
            QSet<EntityItemID> entityItemIDsToDelete;

            for (size_t i = 0; i < numberOfIDs; i++) {

                if (NUM_BYTES_RFC4122_UUID > packet.bytesLeftToRead()) {
                    qCDebug(entities) << "EntityTree::processEraseMessage().... bailing because not enough bytes in buffer";
                    break; // bail to prevent buffer overflow
                }

                QUuid entityID = QUuid::fromRfc4122(packet.readWithoutCopy(NUM_BYTES_RFC4122_UUID));
                #ifdef EXTRA_ERASE_DEBUGGING
                    qDebug() << "    ---- EntityTree::processEraseMessage() contained ID:" << entityID;
                #endif

                EntityItemID entityItemID(entityID);
                entityItemIDsToDelete << entityItemID;

                if (wantEditLogging() || wantTerseEditLogging()) {
                    qCDebug(entities) << "User [" << sourceNode->getUUID() << "] deleting entity. ID:" << entityItemID;
                }

            }
            deleteEntities(entityItemIDsToDelete, true, true);
        }
    });
    return packet.pos();
}

// This version skips over the header
// NOTE: Caller must lock the tree before calling this.
// TODO: consider consolidating processEraseMessageDetails() and processEraseMessage()
int EntityTree::processEraseMessageDetails(const QByteArray& dataByteArray, const SharedNodePointer& sourceNode) {
    #ifdef EXTRA_ERASE_DEBUGGING
        qDebug() << "EntityTree::processEraseMessageDetails()";
    #endif
    const unsigned char* packetData = (const unsigned char*)dataByteArray.constData();
    const unsigned char* dataAt = packetData;
    size_t packetLength = dataByteArray.size();
    size_t processedBytes = 0;

    uint16_t numberOfIds = 0; // placeholder for now
    memcpy(&numberOfIds, dataAt, sizeof(numberOfIds));
    dataAt += sizeof(numberOfIds);
    processedBytes += sizeof(numberOfIds);

    if (numberOfIds > 0) {
        QSet<EntityItemID> entityItemIDsToDelete;

        for (size_t i = 0; i < numberOfIds; i++) {


            if (processedBytes + NUM_BYTES_RFC4122_UUID > packetLength) {
                qCDebug(entities) << "EntityTree::processEraseMessageDetails().... bailing because not enough bytes in buffer";
                break; // bail to prevent buffer overflow
            }

            QByteArray encodedID = dataByteArray.mid(processedBytes, NUM_BYTES_RFC4122_UUID);
            QUuid entityID = QUuid::fromRfc4122(encodedID);
            dataAt += encodedID.size();
            processedBytes += encodedID.size();

            #ifdef EXTRA_ERASE_DEBUGGING
                qDebug() << "    ---- EntityTree::processEraseMessageDetails() contains id:" << entityID;
            #endif

            EntityItemID entityItemID(entityID);
            entityItemIDsToDelete << entityItemID;

            if (wantEditLogging() || wantTerseEditLogging()) {
                qCDebug(entities) << "User [" << sourceNode->getUUID() << "] deleting entity. ID:" << entityItemID;
            }

        }
        deleteEntities(entityItemIDsToDelete, true, true);
    }
    return processedBytes;
}

EntityTreeElementPointer EntityTree::getContainingElement(const EntityItemID& entityItemID)  /*const*/ {
    // TODO: do we need to make this thread safe? Or is it acceptable as is
    EntityTreeElementPointer element = _entityToElementMap.value(entityItemID);
    return element;
}

void EntityTree::setContainingElement(const EntityItemID& entityItemID, EntityTreeElementPointer element) {
    // TODO: do we need to make this thread safe? Or is it acceptable as is
    if (element) {
        _entityToElementMap[entityItemID] = element;
    } else {
        _entityToElementMap.remove(entityItemID);
    }
}

void EntityTree::debugDumpMap() {
    qCDebug(entities) << "EntityTree::debugDumpMap() --------------------------";
    QHashIterator<EntityItemID, EntityTreeElementPointer> i(_entityToElementMap);
    while (i.hasNext()) {
        i.next();
        qCDebug(entities) << i.key() << ": " << i.value().get();
    }
    qCDebug(entities) << "-----------------------------------------------------";
}

class ContentsDimensionOperator : public RecurseOctreeOperator {
public:
    virtual bool preRecursion(OctreeElementPointer element);
    virtual bool postRecursion(OctreeElementPointer element) { return true; }
    float getLargestDimension() const { return _contentExtents.largestDimension(); }
private:
    Extents _contentExtents;
};

bool ContentsDimensionOperator::preRecursion(OctreeElementPointer element) {
    EntityTreeElementPointer entityTreeElement = std::static_pointer_cast<EntityTreeElement>(element);
    entityTreeElement->expandExtentsToContents(_contentExtents);
    return true;
}

float EntityTree::getContentsLargestDimension() {
    ContentsDimensionOperator theOperator;
    recurseTreeWithOperator(&theOperator);
    return theOperator.getLargestDimension();
}

class DebugOperator : public RecurseOctreeOperator {
public:
    virtual bool preRecursion(OctreeElementPointer element);
    virtual bool postRecursion(OctreeElementPointer element) { return true; }
};

bool DebugOperator::preRecursion(OctreeElementPointer element) {
    EntityTreeElementPointer entityTreeElement = std::static_pointer_cast<EntityTreeElement>(element);
    qCDebug(entities) << "EntityTreeElement [" << entityTreeElement.get() << "]";
    entityTreeElement->debugDump();
    return true;
}

void EntityTree::dumpTree() {
    DebugOperator theOperator;
    recurseTreeWithOperator(&theOperator);
}

class PruneOperator : public RecurseOctreeOperator {
public:
    virtual bool preRecursion(OctreeElementPointer element) { return true; }
    virtual bool postRecursion(OctreeElementPointer element);
};

bool PruneOperator::postRecursion(OctreeElementPointer element) {
    EntityTreeElementPointer entityTreeElement = std::static_pointer_cast<EntityTreeElement>(element);
    entityTreeElement->pruneChildren();
    return true;
}

void EntityTree::pruneTree() {
    PruneOperator theOperator;
    recurseTreeWithOperator(&theOperator);
}

QVector<EntityItemID> EntityTree::sendEntities(EntityEditPacketSender* packetSender, EntityTreePointer localTree,
                                               float x, float y, float z) {
    SendEntitiesOperationArgs args;
    args.packetSender = packetSender;
    args.localTree = localTree;
    args.root = glm::vec3(x, y, z);
    QVector<EntityItemID> newEntityIDs;
    args.newEntityIDs = &newEntityIDs;
    recurseTreeWithOperation(sendEntitiesOperation, &args);
    packetSender->releaseQueuedMessages();

    return newEntityIDs;
}

bool EntityTree::sendEntitiesOperation(OctreeElementPointer element, void* extraData) {
    SendEntitiesOperationArgs* args = static_cast<SendEntitiesOperationArgs*>(extraData);
    EntityTreeElementPointer entityTreeElement = std::static_pointer_cast<EntityTreeElement>(element);
    entityTreeElement->forEachEntity([&](EntityItemPointer entityItem) {
        EntityItemID newID(QUuid::createUuid());
        args->newEntityIDs->append(newID);
        EntityItemProperties properties = entityItem->getProperties();
        properties.setPosition(properties.getPosition() + args->root);
        properties.markAllChanged(); // so the entire property set is considered new, since we're making a new entity

        // queue the packet to send to the server
        args->packetSender->queueEditEntityMessage(PacketType::EntityAdd, newID, properties);

        // also update the local tree instantly (note: this is not our tree, but an alternate tree)
        if (args->localTree) {
            args->localTree->withWriteLock([&] {
                args->localTree->addEntity(newID, properties);
            });
        }
    });
    return true;
}

bool EntityTree::writeToMap(QVariantMap& entityDescription, OctreeElementPointer element, bool skipDefaultValues) {
    if (! entityDescription.contains("Entities")) {
        entityDescription["Entities"] = QVariantList();
    }
    QScriptEngine scriptEngine;
    RecurseOctreeToMapOperator theOperator(entityDescription, element, &scriptEngine, skipDefaultValues);
    recurseTreeWithOperator(&theOperator);
    return true;
}

bool EntityTree::readFromMap(QVariantMap& map) {
    // map will have a top-level list keyed as "Entities".  This will be extracted
    // and iterated over.  Each member of this list is converted to a QVariantMap, then
    // to a QScriptValue, and then to EntityItemProperties.  These properties are used
    // to add the new entity to the EnitytTree.
    QVariantList entitiesQList = map["Entities"].toList();
    QScriptEngine scriptEngine;

    foreach (QVariant entityVariant, entitiesQList) {
        // QVariantMap --> QScriptValue --> EntityItemProperties --> Entity
        QVariantMap entityMap = entityVariant.toMap();
        QScriptValue entityScriptValue = variantMapToScriptValue(entityMap, scriptEngine);
        EntityItemProperties properties;
        EntityItemPropertiesFromScriptValueIgnoreReadOnly(entityScriptValue, properties);

        EntityItemID entityItemID;
        if (entityMap.contains("id")) {
            entityItemID = EntityItemID(QUuid(entityMap["id"].toString()));
        } else {
            entityItemID = EntityItemID(QUuid::createUuid());
        }

        EntityItemPointer entity = addEntity(entityItemID, properties);
        if (!entity) {
            qCDebug(entities) << "adding Entity failed:" << entityItemID << properties.getType();
        }
    }

    return true;
}

void EntityTree::resetClientEditStats() {
    _treeResetTime = usecTimestampNow();
    _maxEditDelta = 0;
    _totalEditDeltas = 0;
    _totalTrackedEdits = 0;
}



void EntityTree::trackIncomingEntityLastEdited(quint64 lastEditedTime, int bytesRead) {
    // we don't want to track all edit deltas, just those edits that have happend
    // since we connected to this domain. This will filter out all previously created
    // content and only track new edits
    if (lastEditedTime > _treeResetTime) {
        quint64 now = usecTimestampNow();
        quint64 sinceEdit = now - lastEditedTime;

        _totalEditDeltas += sinceEdit;
        _totalEditBytes += bytesRead;
        _totalTrackedEdits++;
        if (sinceEdit > _maxEditDelta) {
            _maxEditDelta = sinceEdit;
        }
    }
}
