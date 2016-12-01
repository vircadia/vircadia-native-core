//
//  CharaterSweepResult.h
//  libraries/physics/src
//
//  Created by Andrew Meadows 2016.09.01
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_CharacterSweepResult_h
#define hifi_CharacterSweepResult_h

#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionDispatch/btCollisionWorld.h>


class CharacterGhostObject;

class CharacterSweepResult : public btCollisionWorld::ClosestConvexResultCallback {
public:
    CharacterSweepResult(const CharacterGhostObject* character);
    virtual btScalar addSingleResult(btCollisionWorld::LocalConvexResult& convexResult, bool useWorldFrame) override;
	void resetHitHistory();
protected:
    const CharacterGhostObject* _character;

    // NOTE: Public data members inherited from ClosestConvexResultCallback:
    //
    //	btVector3   m_convexFromWorld; // unused except by btClosestNotMeConvexResultCallback
    //  btVector3   m_convexToWorld;   // unused except by btClosestNotMeConvexResultCallback
    //  btVector3   m_hitNormalWorld;
    //  btVector3   m_hitPointWorld;
    //  const btCollisionObject*    m_hitCollisionObject;
    //
    // NOTE: Public data members inherited from ConvexResultCallback:
    //
    //  btScalar    m_closestHitFraction;
    //  short int   m_collisionFilterGroup;
    //  short int   m_collisionFilterMask;

};

#endif // hifi_CharacterSweepResult_h
