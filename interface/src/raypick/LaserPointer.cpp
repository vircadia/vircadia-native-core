//
//  LaserPointer.cpp
//  interface/src/raypick
//
//  Created by Sam Gondelman 7/11/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "LaserPointer.h"

#include "RayPickManager.h"
#include "JointRayPick.h"

LaserPointer::LaserPointer(const QString& jointName, const glm::vec3& posOffset, const glm::vec3& dirOffset, const uint16_t filter, const float maxDistance, const bool enabled)
{
    _rayPickUID = RayPickManager::getInstance().addRayPick(std::make_shared<JointRayPick>(jointName, posOffset, dirOffset, filter, maxDistance, enabled));
}

LaserPointer::~LaserPointer() {
    RayPickManager::getInstance().removeRayPick(_rayPickUID);
}

void LaserPointer::enable() {
    RayPickManager::getInstance().enableRayPick(_rayPickUID);
    // TODO:
    // turn on rendering
}

void LaserPointer::disable() {
    RayPickManager::getInstance().disableRayPick(_rayPickUID);
    // TODO:
    // turn off rendering
}

const RayPickResult& LaserPointer::getPrevRayPickResult() {
    return RayPickManager::getInstance().getPrevRayPickResult(_rayPickUID);
}