//
//  CharaterSweepResult.cpp
//  libraries/physics/src
//
//  Created by Andrew Meadows 2016.09.01
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "CharacterSweepResult.h"

#include <assert.h>

#include "CharacterGhostObject.h"

CharacterSweepResult::CharacterSweepResult(const CharacterGhostObject* character)
    :   btCollisionWorld::ClosestConvexResultCallback(btVector3(0.0f, 0.0f, 0.0f), btVector3(0.0f, 0.0f, 0.0f)),
        _character(character)
{
    // set collision group and mask to match _character
    assert(_character);
    _character->getCollisionGroupAndMask(m_collisionFilterGroup, m_collisionFilterMask);
}

btScalar CharacterSweepResult::addSingleResult(btCollisionWorld::LocalConvexResult& convexResult, bool useWorldFrame) {
	// skip objects that we shouldn't collide with
    if (!convexResult.m_hitCollisionObject->hasContactResponse()) {
        return btScalar(1.0);
    }
    if (convexResult.m_hitCollisionObject == _character) {
        return btScalar(1.0);
    }

    return ClosestConvexResultCallback::addSingleResult(convexResult, useWorldFrame);
}

void CharacterSweepResult::resetHitHistory() {
	m_hitCollisionObject = nullptr;
	m_closestHitFraction = btScalar(1.0f);
}
