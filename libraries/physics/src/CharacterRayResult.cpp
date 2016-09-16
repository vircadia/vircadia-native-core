//
//  CharaterRayResult.cpp
//  libraries/physcis/src
//
//  Created by Andrew Meadows 2016.09.05
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "CharacterRayResult.h"

#include <assert.h>

#include "CharacterGhostObject.h"

CharacterRayResult::CharacterRayResult (const CharacterGhostObject* character) :
    btCollisionWorld::ClosestRayResultCallback(btVector3(0.0f, 0.0f, 0.0f), btVector3(0.0f, 0.0f, 0.0f)),
    _character(character)
{
    assert(_character);
    _character->getCollisionGroupAndMask(m_collisionFilterGroup, m_collisionFilterMask);
}

btScalar CharacterRayResult::addSingleResult(btCollisionWorld::LocalRayResult& rayResult, bool normalInWorldSpace) {
    if (rayResult.m_collisionObject == _character) {
        return 1.0f;
    }
    return ClosestRayResultCallback::addSingleResult (rayResult, normalInWorldSpace);
}
