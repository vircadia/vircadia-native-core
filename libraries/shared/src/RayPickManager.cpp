//
//  RayPickManager.cpp
//  libraries/shared/src
//
//  Created by Sam Gondelman 7/11/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "RayPickManager.h"
#include "RayPick.h"

RayPickManager& RayPickManager::getInstance() {
    static RayPickManager instance;
    return instance;
}

void RayPickManager::update() {
    /***
    - somehow calculate minimum number of intersection tests, update raypicks accordingly
        -one option :
            loop over all raypicks{ loop over all necessary intersection tests and take min distance less than rayPick->maxDistance, but keep cache of this frame's previous tests to prevent duplicate intersection tests }
    ***/
}

unsigned int RayPickManager::addRayPick(std::shared_ptr<RayPick> rayPick) {
    // TODO:
    // use lock and defer adding to prevent issues
    _rayPicks[_nextUID] = rayPick;
    return _nextUID++;
}

void RayPickManager::removeRayPick(const unsigned int uid) {
    // TODO:
    // use lock and defer removing to prevent issues
    _rayPicks.remove(uid);
}

void RayPickManager::enableRayPick(const unsigned int uid) {
    // TODO:
    // use lock and defer enabling to prevent issues
    if (_rayPicks.contains(uid)) {
        _rayPicks[uid]->enable();
    }
}

void RayPickManager::disableRayPick(const unsigned int uid) {
    // TODO:
    // use lock and defer disabling to prevent issues
    if (_rayPicks.contains(uid)) {
        _rayPicks[uid]->disable();
    }
}
