//
//  EntitySimulation.h
//  libraries/entities/src
//
//  Created by Andrew Meadows on 2014.11.24
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_EntitySimulation_h
#define hifi_EntitySimulation_h

#include <limits>
#include <unordered_set>

#include <QtCore/QObject>
#include <QVector>

#include <PerfStat.h>

#include "EntityItem.h"
#include "EntityTree.h"

using EntitySimulationPointer = std::shared_ptr<EntitySimulation>;
using VectorOfEntities = QVector<EntityItemPointer>;

// the EntitySimulation needs to know when these things change on an entity,
// so it can sort EntityItem or relay its state to the PhysicsEngine.
const int DIRTY_SIMULATION_FLAGS =
        Simulation::DIRTY_POSITION |
        Simulation::DIRTY_ROTATION |
        Simulation::DIRTY_LINEAR_VELOCITY |
        Simulation::DIRTY_ANGULAR_VELOCITY |
        Simulation::DIRTY_MASS |
        Simulation::DIRTY_COLLISION_GROUP |
        Simulation::DIRTY_MOTION_TYPE |
        Simulation::DIRTY_SHAPE |
        Simulation::DIRTY_LIFETIME |
        Simulation::DIRTY_UPDATEABLE |
        Simulation::DIRTY_MATERIAL |
        Simulation::DIRTY_SIMULATOR_ID;

class EntitySimulation : public QObject, public std::enable_shared_from_this<EntitySimulation> {
public:
    EntitySimulation() : _mutex(), _nextExpiry(std::numeric_limits<uint64_t>::max()), _entityTree(nullptr) { }
    virtual ~EntitySimulation() { setEntityTree(nullptr); }

    inline EntitySimulationPointer getThisPointer() const {
        return std::const_pointer_cast<EntitySimulation>(shared_from_this());
    }

    /// \param tree pointer to EntityTree which is stored internally
    void setEntityTree(EntityTreePointer tree);

    virtual void updateEntities();

    // FIXME: remove these
    virtual void addDynamic(EntityDynamicPointer dynamic) {}
    virtual void removeDynamic(const QUuid dynamicID) {}
    virtual void applyDynamicChanges() {};

    /// \param entity pointer to EntityItem to be added
    /// \sideeffect sets relevant backpointers in entity, but maybe later when appropriate data structures are locked
    void addEntity(EntityItemPointer entity);

    /// \param entity pointer to EntityItem that may have changed in a way that would affect its simulation
    /// call this whenever an entity was changed from some EXTERNAL event (NOT by the EntitySimulation itself)
    void changeEntity(EntityItemPointer entity);

    virtual void clearEntities();

    void moveSimpleKinematics(uint64_t now);

    EntityTreePointer getEntityTree() { return _entityTree; }

    virtual void prepareEntityForDelete(EntityItemPointer entity);

    void processChangedEntities();
    virtual void queueEraseDomainEntity(const QUuid& id) const { }

protected:
    virtual void addEntityToInternalLists(EntityItemPointer entity);
    virtual void removeEntityFromInternalLists(EntityItemPointer entity);
    virtual void processChangedEntity(const EntityItemPointer& entity);
    virtual void processDeadEntities();

    void expireMortalEntities(uint64_t now);
    void callUpdateOnEntitiesThatNeedIt(uint64_t now);
    virtual void sortEntitiesThatMoved();

    QRecursiveMutex _mutex;

    SetOfEntities _entitiesToSort; // entities moved by simulation (and might need resort in EntityTree)
    SetOfEntities _simpleKinematicEntities; // entities undergoing non-colliding kinematic motion
    SetOfEntities _deadEntitiesToRemoveFromTree;

private:
    void moveSimpleKinematics();

    // We maintain multiple lists, each for its distinct purpose.
    // An entity may be in more than one list.
    std::unordered_set<EntityItemPointer> _changedEntities; // all changes this frame
    SetOfEntities _allEntities; // tracks all entities added the simulation
    SetOfEntities _entitiesToUpdate; // entities that need to call EntityItem::update()
    SetOfEntities _mortalEntities; // entities that have an expiry
    uint64_t _nextExpiry;

    // back pointer to EntityTree structure
    EntityTreePointer _entityTree;
};

#endif // hifi_EntitySimulation_h
