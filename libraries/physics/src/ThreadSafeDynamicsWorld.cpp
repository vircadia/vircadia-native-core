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

#include <EntityTree.h>

#include "ThreadSafeDynamicsWorld.h"

#ifdef USE_BULLET_PHYSICS
ThreadSafeDynamicsWorld::ThreadSafeDynamicsWorld(
        btDispatcher* dispatcher,
        btBroadphaseInterface* pairCache,
        btConstraintSolver* constraintSolver,
        btCollisionConfiguration* collisionConfiguration,
        EntityTree* entities)
    :   btDiscreteDynamicsWorld(dispatcher, pairCache, constraintSolver, collisionConfiguration) {
    assert(entities);
    _entities = entities;
}

void ThreadSafeDynamicsWorld::synchronizeMotionStates() {
    _entities->lockForWrite();
    btDiscreteDynamicsWorld::synchronizeMotionStates();
    _entities->unlock();
}

int	ThreadSafeDynamicsWorld::stepSimulation( btScalar timeStep, int maxSubSteps, btScalar fixedTimeStep) {
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
		btIDebugDraw* debugDrawer = getDebugDrawer ();
		gDisableDeactivation = (debugDrawer->getDebugMode() & btIDebugDraw::DBG_NoDeactivation) != 0;
	}*/
	if (subSteps) {
		//clamp the number of substeps, to prevent simulation grinding spiralling down to a halt
		int clampedSimulationSteps = (subSteps > maxSubSteps)? maxSubSteps : subSteps;

		saveKinematicState(fixedTimeStep*clampedSimulationSteps);

		applyGravity();

		for (int i=0;i<clampedSimulationSteps;i++) {
			internalSingleStepSimulation(fixedTimeStep);
		}
	}

    // We only sync motion states once at the end of all substeps.  
    // This is to avoid placing multiple, repeated thread locks on _entities.
    synchronizeMotionStates();
	clearForces();
	return subSteps;
}
#endif // USE_BULLET_PHYSICS
