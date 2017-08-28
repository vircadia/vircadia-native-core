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

#include <QtCore/QObject>
#include <QSet>
#include <QVector>

#include <PerfStat.h>

#include "EntityDynamicInterface.h"
#include "EntityItem.h"
#include "EntityTree.h"

using EntitySimulationPointer = std::shared_ptr<EntitySimulation>;
using SetOfEntities = QSet<EntityItemPointer>;
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
Q_OBJECT
public:
    EntitySimulation() : _mutex(QMutex::Recursive), _entityTree(NULL), _nextExpiry(quint64(-1)) { }
    virtual ~EntitySimulation() { setEntityTree(NULL); }

    inline EntitySimulationPointer getThisPointer() const {
        return std::const_pointer_cast<EntitySimulation>(shared_from_this());
    }

    /// \param tree pointer to EntityTree which is stored internally
    void setEntityTree(EntityTreePointer tree);

    void updateEntities();

//    friend class EntityTree;

    virtual void addDynamic(EntityDynamicPointer dynamic);
    virtual void removeDynamic(const QUuid dynamicID);
    virtual void removeDynamics(QList<QUuid> dynamicIDsToRemove);
    virtual void applyDynamicChanges();

    /// \param entity pointer to EntityItem to be added
    /// \sideeffect sets relevant backpointers in entity, but maybe later when appropriate data structures are locked
    void addEntity(EntityItemPointer entity);

    /// \param entity pointer to EntityItem that may have changed in a way that would affect its simulation
    /// call this whenever an entity was changed from some EXTERNAL event (NOT by the EntitySimulation itself)
    void changeEntity(EntityItemPointer entity);

    void clearEntities();

    void moveSimpleKinematics(const quint64& now);

    EntityTreePointer getEntityTree() { return _entityTree; }

    virtual void takeEntitiesToDelete(VectorOfEntities& entitiesToDelete);

    /// \param entity pointer to EntityItem that needs to be put on the entitiesToDelete list and removed from others.
    virtual void prepareEntityForDelete(EntityItemPointer entity);

signals:
    void entityCollisionWithEntity(const EntityItemID& idA, const EntityItemID& idB, const Collision& collision);

protected:
    // These pure virtual methods are protected because they are not to be called will-nilly. The base class
    // calls them in the right places.
    virtual void updateEntitiesInternal(const quint64& now) = 0;
    virtual void addEntityInternal(EntityItemPointer entity);
    virtual void removeEntityInternal(EntityItemPointer entity) = 0;
    virtual void changeEntityInternal(EntityItemPointer entity);
    virtual void clearEntitiesInternal() = 0;

    void expireMortalEntities(const quint64& now);
    void callUpdateOnEntitiesThatNeedIt(const quint64& now);
    virtual void sortEntitiesThatMoved();

    QMutex _mutex{ QMutex::Recursive };

    SetOfEntities _entitiesToSort; // entities moved by simulation (and might need resort in EntityTree)
    SetOfEntities _simpleKinematicEntities; // entities undergoing non-colliding kinematic motion
    QList<EntityDynamicPointer> _dynamicsToAdd;
    QSet<QUuid> _dynamicsToRemove;
    QMutex _dynamicsMutex { QMutex::Recursive };

protected:
    SetOfEntities _entitiesToDelete; // entities simulation decided needed to be deleted (EntityTree will actually delete)

private:
    void moveSimpleKinematics();

    // back pointer to EntityTree structure
    EntityTreePointer _entityTree;

    // We maintain multiple lists, each for its distinct purpose.
    // An entity may be in more than one list.
    SetOfEntities _allEntities; // tracks all entities added the simulation
    SetOfEntities _mortalEntities; // entities that have an expiry
    quint64 _nextExpiry;


    SetOfEntities _entitiesToUpdate; // entities that need to call EntityItem::update()

};

#endif // hifi_EntitySimulation_h
