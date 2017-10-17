//
//  RayPickManager.cpp
//  interface/src/raypick
//
//  Created by Sam Gondelman 7/11/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "RayPickManager.h"

#include "StaticRayPick.h"
#include "JointRayPick.h"
#include "MouseRayPick.h"

QUuid RayPickManager::createRayPick(const std::string& jointName, const glm::vec3& posOffset, const glm::vec3& dirOffset, const PickFilter& filter, float maxDistance, bool enabled) {
    return addPick(std::make_shared<JointRayPick>(jointName, posOffset, dirOffset, filter, maxDistance, enabled));
}

QUuid RayPickManager::createRayPick(const PickFilter& filter, float maxDistance, bool enabled) {
    return addPick(std::make_shared<MouseRayPick>(filter, maxDistance, enabled));
}

QUuid RayPickManager::createRayPick(const glm::vec3& position, const glm::vec3& direction, const PickFilter& filter, float maxDistance, bool enabled) {
    return addPick(std::make_shared<StaticRayPick>(position, direction, filter, maxDistance, enabled));
}