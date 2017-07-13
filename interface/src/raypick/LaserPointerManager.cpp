//
//  LaserPointerManager.cpp
//  interface/src/raypick
//
//  Created by Sam Gondelman 7/11/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "LaserPointerManager.h"
#include "LaserPointer.h"
#include "RayPick.h"

LaserPointerManager& LaserPointerManager::getInstance() {
    static LaserPointerManager instance;
    return instance;
}

unsigned int LaserPointerManager::createLaserPointer(const QString& jointName, const glm::vec3& posOffset, const glm::vec3& dirOffset, const uint16_t filter, const float maxDistance, bool enabled) {
    std::shared_ptr<LaserPointer> laserPointer = std::make_shared<LaserPointer>(jointName, posOffset, dirOffset, filter, maxDistance, enabled);
    unsigned int uid = laserPointer->getUID();
    _laserPointers[uid] = laserPointer;
    return uid;
}

void LaserPointerManager::enableLaserPointer(const unsigned int uid) {
    if (_laserPointers.contains(uid)) {
        _laserPointers[uid]->enable();
    }
}

void LaserPointerManager::disableLaserPointer(const unsigned int uid) {
    if (_laserPointers.contains(uid)) {
        _laserPointers[uid]->disable();
    }
}

const RayPickResult& LaserPointerManager::getPrevRayPickResult(const unsigned int uid) {
    if (_laserPointers.contains(uid)) {
        return _laserPointers[uid]->getPrevRayPickResult();
    }
    return RayPickResult();
}
