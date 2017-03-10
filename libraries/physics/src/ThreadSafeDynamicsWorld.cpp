/*
 * Bullet Continuous Collision Detection and Physics Library
 * Copyright (c) 2003-2009 Erwin Coumans  http://bulletphysics.org
 *
 * This software is provided 'as-is', without any express or implied warranty.
 * In no event will the authors be held liable for any damages arising from the use of this software.
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it freely,
 * subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 * Copied and modified from btDiscreteDynamicsWorld.cpp by AndrewMeadows on 2014.11.12.
 * */

#include <LinearMath/btQuickprof.h>

#include "ThreadSafeDynamicsWorld.h"

ThreadSafeDynamicsWorld::ThreadSafeDynamicsWorld(
        btDispatcher* dispatcher,
        btBroadphaseInterface* pairCache,
        btConstraintSolver* constraintSolver,
        btCollisionConfiguration* collisionConfiguration)
    :   btDiscreteDynamicsWorld(dispatcher, pairCache, constraintSolver, collisionConfiguration) {
}

int ThreadSafeDynamicsWorld::stepSimulationWithSubstepCallback(btScalar timeStep, int maxSubSteps,
                                                               btScalar fixedTimeStep, SubStepCallback onSubStep) {
    BT_PROFILE("stepSimulationWithSubstepCallback");
    int subSteps = 0;
    if (maxSubSteps) {
        //fixed timestep with interpolation
        m_fixedTimeStep = fixedTimeStep;
        m_localTime += timeStep;
        if (m_localTime >= fixedTimeStep)
        {
            subSteps = int( m_localTime / fixedTimeStep);
            m_localTime -= subSteps * fixedTimeStep;
        }
    } else {
        //variable timestep
        fixedTimeStep = timeStep;
        m_localTime = m_latencyMotionStateInterpolation ? 0 : timeStep;
        m_fixedTimeStep = 0;
        if (btFuzzyZero(timeStep))
        {
            subSteps = 0;
            maxSubSteps = 0;
        } else
        {
            subSteps = 1;
            maxSubSteps = 1;
        }
    }

    /*//process some debugging flags
    if (getDebugDrawer()) {
        btIDebugDraw* debugDrawer = getDebugDrawer();
        gDisableDeactivation = (debugDrawer->getDebugMode() & btIDebugDraw::DBG_NoDeactivation) != 0;
    }*/
    if (subSteps) {
        //clamp the number of substeps, to prevent simulation grinding spiralling down to a halt
        int clampedSimulationSteps = (subSteps > maxSubSteps)? maxSubSteps : subSteps;

        saveKinematicState(fixedTimeStep*clampedSimulationSteps);

        {
            BT_PROFILE("applyGravity");
            applyGravity();
        }

        for (int i=0;i<clampedSimulationSteps;i++) {
            internalSingleStepSimulation(fixedTimeStep);
            onSubStep();
        }
    }

    // NOTE: We do NOT call synchronizeMotionStates() after each substep (to avoid multiple locks on the
    // object data outside of the physics engine).  A consequence of this is that the transforms of the
    // external objects only ever update at the end of the full step.

    // NOTE: We do NOT call synchronizeMotionStates() here.  Instead it is called by an external class
    // that knows how to lock threads correctly.

    clearForces();

    return subSteps;
}

// call this instead of non-virtual btDiscreteDynamicsWorld::synchronizeSingleMotionState()
void ThreadSafeDynamicsWorld::synchronizeMotionState(btRigidBody* body) {
    btAssert(body);
    if (body->getMotionState() && !body->isStaticObject()) {
        //we need to call the update at least once, even for sleeping objects
        //otherwise the 'graphics' transform never updates properly
        ///@todo: add 'dirty' flag
        //if (body->getActivationState() != ISLAND_SLEEPING)
        {
            if (body->isKinematicObject()) {
                ObjectMotionState* objectMotionState = static_cast<ObjectMotionState*>(body->getMotionState());
                if (objectMotionState->hasInternalKinematicChanges()) {
                    objectMotionState->clearInternalKinematicChanges();
                    body->getMotionState()->setWorldTransform(body->getWorldTransform());
                }
                return;
            }
            btTransform interpolatedTransform;
            btTransformUtil::integrateTransform(body->getInterpolationWorldTransform(),
                body->getInterpolationLinearVelocity(),body->getInterpolationAngularVelocity(),
                (m_latencyMotionStateInterpolation && m_fixedTimeStep) ? m_localTime - m_fixedTimeStep : m_localTime*body->getHitFraction(),
                interpolatedTransform);
            body->getMotionState()->setWorldTransform(interpolatedTransform);
        }
    }
}

void ThreadSafeDynamicsWorld::synchronizeMotionStates() {
    BT_PROFILE("synchronizeMotionStates");
    _changedMotionStates.clear();

    // NOTE: m_synchronizeAllMotionStates is 'false' by default for optimization.
    // See PhysicsEngine::init() where we call _dynamicsWorld->setForceUpdateAllAabbs(false)
    if (m_synchronizeAllMotionStates) {
        //iterate  over all collision objects
        for (int i=0;i<m_collisionObjects.size();i++) {
            btCollisionObject* colObj = m_collisionObjects[i];
            btRigidBody* body = btRigidBody::upcast(colObj);
            if (body && body->getMotionState()) {
                synchronizeMotionState(body);
                _changedMotionStates.push_back(static_cast<ObjectMotionState*>(body->getMotionState()));
            }
        }
    } else  {
        //iterate over all active rigid bodies
        // TODO? if this becomes a performance bottleneck we could derive our own SimulationIslandManager
        // that remembers a list of objects deactivated last step
        _activeStates.clear();
        _deactivatedStates.clear();
        for (int i=0;i<m_nonStaticRigidBodies.size();i++) {
            btRigidBody* body = m_nonStaticRigidBodies[i];
            ObjectMotionState* motionState = static_cast<ObjectMotionState*>(body->getMotionState());
            if (motionState) {
                if (body->isActive()) {
                    synchronizeMotionState(body);
                    _changedMotionStates.push_back(motionState);
                    _activeStates.insert(motionState);
                } else if (_lastActiveStates.find(motionState) != _lastActiveStates.end()) {
                    // this object was active last frame but is no longer
                    _deactivatedStates.push_back(motionState);
                }
            }
        }
    }
    _activeStates.swap(_lastActiveStates);
}

void ThreadSafeDynamicsWorld::saveKinematicState(btScalar timeStep) {
///would like to iterate over m_nonStaticRigidBodies, but unfortunately old API allows
///to switch status _after_ adding kinematic objects to the world
///fix it for Bullet 3.x release
    BT_PROFILE("saveKinematicState");
    for (int i=0;i<m_collisionObjects.size();i++)
    {
        btCollisionObject* colObj = m_collisionObjects[i];
        btRigidBody* body = btRigidBody::upcast(colObj);
        if (body && body->getActivationState() != ISLAND_SLEEPING)
        {
            if (body->isKinematicObject())
            {
                //to calculate velocities next frame
                body->saveKinematicState(timeStep);
            }
        }
    }
}


