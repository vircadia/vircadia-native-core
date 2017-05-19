//
//  CharaterRayResult.h
//  libraries/physics/src
//
//  Created by Andrew Meadows 2016.09.05
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_CharacterRayResult_h
#define hifi_CharacterRayResult_h

#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionDispatch/btCollisionWorld.h>

class CharacterGhostObject;

class CharacterRayResult : public btCollisionWorld::ClosestRayResultCallback {
public:
    CharacterRayResult (const CharacterGhostObject* character);

    virtual btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult, bool normalInWorldSpace) override;

protected:
    const CharacterGhostObject* _character;

	// Note: Public data members inherited from ClosestRayResultCallback
	//
	//   btVector3 m_rayFromWorld;//used to calculate hitPointWorld from hitFraction
	//   btVector3 m_rayToWorld;
	//   btVector3 m_hitNormalWorld;
	//   btVector3 m_hitPointWorld;
	//
	// Note: Public data members inherited from RayResultCallback
	//
	//   btScalar m_closestHitFraction;
	//   const btCollisionObject* m_collisionObject;
	//   short int m_collisionFilterGroup;
	//   short int m_collisionFilterMask;
};

#endif // hifi_CharacterRayResult_h
