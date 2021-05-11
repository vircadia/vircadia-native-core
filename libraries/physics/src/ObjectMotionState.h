//
//  ObjectMotionState.h
//  libraries/physics/src
//
//  Created by Andrew Meadows 2014.11.05
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ObjectMotionState_h
#define hifi_ObjectMotionState_h

#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>

#include <QSet>
#include <QVector>

#include <SimulationFlags.h>

#include "ContactInfo.h"
#include "ShapeManager.h"

enum PhysicsMotionType {
    MOTION_TYPE_STATIC,     // no motion
    MOTION_TYPE_DYNAMIC,    // motion according to physical laws
    MOTION_TYPE_KINEMATIC   // keyframed motion
};

/*@jsdoc
 * <p>An entity's physics motion type may be one of the following:</p>
 * <table>
 *   <thead>
 *     <tr><th>Value</th><th>Description</th></tr>
 *   </thead>
 *   <tbody>
 *     <tr><td><code>"static"</code></td><td>There is no motion because the entity is locked  &mdash; its <code>locked</code> 
 *         property is set to <code>true</code>.</td></tr>
 *     <tr><td><code>"kinematic"</code></td><td>Motion is applied without physical laws (e.g., damping) because the entity is 
 *         not locked and has its <code>dynamic</code> property set to <code>false</code>.</td></tr>
 *     <tr><td><code>"dynamic"</code></td><td>Motion is applied according to physical laws (e.g., damping) because the entity 
 *         is not locked and has its <code>dynamic</code> property set to <code>true</code>.</td></tr>
 *   </tbody>
 * </table>
 * @typedef {string} Entities.PhysicsMotionType
 */
inline QString motionTypeToString(PhysicsMotionType motionType) {
    switch(motionType) {
        case MOTION_TYPE_STATIC: return QString("static");
        case MOTION_TYPE_DYNAMIC: return QString("dynamic");
        case MOTION_TYPE_KINEMATIC: return QString("kinematic");
    }
    return QString("unknown");
}

enum MotionStateType {
    MOTIONSTATE_TYPE_INVALID,
    MOTIONSTATE_TYPE_ENTITY,
    MOTIONSTATE_TYPE_AVATAR,
    MOTIONSTATE_TYPE_DETAILED
};

// The update flags trigger two varieties of updates: "hard" which require the body to be pulled
// and re-added to the physics engine and "easy" which just updates the body properties.
const uint32_t HARD_DIRTY_PHYSICS_FLAGS = (uint32_t)(Simulation::DIRTY_MOTION_TYPE | Simulation::DIRTY_SHAPE |
                                                     Simulation::DIRTY_COLLISION_GROUP);
const uint32_t EASY_DIRTY_PHYSICS_FLAGS = (uint32_t)(Simulation::DIRTY_TRANSFORM | Simulation::DIRTY_VELOCITIES |
                                                     Simulation::DIRTY_MASS | Simulation::DIRTY_MATERIAL |
                                                     Simulation::DIRTY_SIMULATOR_ID | Simulation::DIRTY_SIMULATION_OWNERSHIP_PRIORITY |
                                                     Simulation::DIRTY_PHYSICS_ACTIVATION);


// These are the set of incoming flags that the PhysicsEngine needs to hear about:
const uint32_t DIRTY_PHYSICS_FLAGS = (uint32_t)(HARD_DIRTY_PHYSICS_FLAGS | EASY_DIRTY_PHYSICS_FLAGS);

// These are the outgoing flags that the PhysicsEngine can affect:
const uint32_t OUTGOING_DIRTY_PHYSICS_FLAGS = Simulation::DIRTY_TRANSFORM | Simulation::DIRTY_VELOCITIES;


class OctreeEditPacketSender;
class PhysicsEngine;

class ObjectMotionState : public btMotionState {
public:
    // These properties of the PhysicsEngine are "global" within the context of all ObjectMotionStates
    // (assuming just one PhysicsEngine).  They are cached as statics for fast calculations in the
    // ObjectMotionState context.
    static void setWorldOffset(const glm::vec3& offset);
    static const glm::vec3& getWorldOffset();

    static void setWorldSimulationStep(uint32_t step);
    static uint32_t getWorldSimulationStep();

    static void setShapeManager(ShapeManager* manager);
    static ShapeManager* getShapeManager();

    ObjectMotionState(const btCollisionShape* shape);
    virtual ~ObjectMotionState();

    virtual void handleEasyChanges(uint32_t& flags);

    void updateBodyMaterialProperties();
    void updateBodyVelocities();
    void updateLastKinematicStep();

    virtual void updateBodyMassProperties();

    MotionStateType getType() const { return _type; }
    virtual PhysicsMotionType getMotionType() const { return _motionType; }

    virtual void setMass(float mass);
    virtual float getMass() const;

    void setBodyLinearVelocity(const glm::vec3& velocity) const;
    void setBodyAngularVelocity(const glm::vec3& velocity) const;
    void setBodyGravity(const glm::vec3& gravity) const;

    glm::vec3 getBodyLinearVelocity() const;
    glm::vec3 getBodyLinearVelocityGTSigma() const;
    glm::vec3 getBodyAngularVelocity() const;
    virtual glm::vec3 getObjectLinearVelocityChange() const;

    virtual uint32_t getIncomingDirtyFlags() const = 0;
    virtual void clearIncomingDirtyFlags(uint32_t mask = DIRTY_PHYSICS_FLAGS) = 0;

    virtual PhysicsMotionType computePhysicsMotionType() const = 0;

    virtual bool needsNewShape() const { return _shape == nullptr || getIncomingDirtyFlags() & Simulation::DIRTY_SHAPE; }
    const btCollisionShape* getShape() const { return _shape; }
    btRigidBody* getRigidBody() const { return _body; }

    virtual bool isMoving() const = 0;

    // These pure virtual methods must be implemented for each MotionState type
    // and make it possible to implement more complicated methods in this base class.

    virtual float getObjectRestitution() const = 0;
    virtual float getObjectFriction() const = 0;
    virtual float getObjectLinearDamping() const = 0;
    virtual float getObjectAngularDamping() const = 0;

    virtual glm::vec3 getObjectPosition() const = 0;
    virtual glm::quat getObjectRotation() const = 0;
    virtual glm::vec3 getObjectLinearVelocity() const = 0;
    virtual glm::vec3 getObjectAngularVelocity() const = 0;
    virtual glm::vec3 getObjectGravity() const = 0;

    virtual const QUuid getObjectID() const = 0;

    virtual uint8_t getSimulationPriority() const { return 0; }
    virtual QUuid getSimulatorID() const = 0;
    virtual void bump(uint8_t priority) {}

    virtual QString getName() const { return ""; }
    virtual ShapeType getShapeType() const = 0;

    virtual void computeCollisionGroupAndMask(int32_t& group, int32_t& mask) const = 0;

    bool isActive() const { return _body ? _body->isActive() : false; }

    bool hasInternalKinematicChanges() const { return _hasInternalKinematicChanges; }

    // these methods are declared const so they can be called inside other const methods
    void dirtyInternalKinematicChanges() const { _hasInternalKinematicChanges = true; }
    void clearInternalKinematicChanges() const { _hasInternalKinematicChanges = false; }

    virtual bool isLocallyOwned() const { return false; }
    virtual bool isLocallyOwnedOrShouldBe() const { return false; } // aka shouldEmitCollisionEvents()
    virtual void saveKinematicState(btScalar timeStep);

    friend class PhysicsEngine;

protected:
    virtual void setMotionType(PhysicsMotionType motionType);
    void updateCCDConfiguration();

    virtual void setRigidBody(btRigidBody* body);
    virtual void setShape(const btCollisionShape* shape);

    MotionStateType _type { MOTIONSTATE_TYPE_INVALID }; // type of MotionState
    PhysicsMotionType _motionType { MOTION_TYPE_STATIC }; // type of motion: KINEMATIC, DYNAMIC, or STATIC

    const btCollisionShape* _shape { nullptr };
    btRigidBody* _body { nullptr };
    float _density { 1.0f };

    // ACTION_CAN_CONTROL_KINEMATIC_OBJECT_HACK: These data members allow an Action
    // to operate on a kinematic object without screwing up our default kinematic integration
    // which is done in the MotionState::getWorldTransform().
    mutable uint32_t _lastKinematicStep;
    mutable bool _hasInternalKinematicChanges { false };
};

using SetOfMotionStates = QSet<ObjectMotionState*>;
using VectorOfMotionStates = QVector<ObjectMotionState*>;

#endif // hifi_ObjectMotionState_h
