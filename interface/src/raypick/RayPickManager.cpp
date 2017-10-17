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
    auto newRayPick = std::make_shared<JointRayPick>(jointName, posOffset, dirOffset, filter, maxDistance, enabled);
    QUuid id = QUuid::createUuid();
    withWriteLock([&] {
        _picks[id] = newRayPick;
    });
    return id;
}

QUuid RayPickManager::createRayPick(const PickFilter& filter, float maxDistance, bool enabled) {
    QUuid id = QUuid::createUuid();
    auto newRayPick = std::make_shared<MouseRayPick>(filter, maxDistance, enabled);
    withWriteLock([&] {
        _picks[id] = newRayPick;
    });
    return id;
}

QUuid RayPickManager::createRayPick(const glm::vec3& position, const glm::vec3& direction, const PickFilter& filter, float maxDistance, bool enabled) {
    QUuid id = QUuid::createUuid();
    auto newRayPick = std::make_shared<StaticRayPick>(position, direction, filter, maxDistance, enabled);
    withWriteLock([&] {
        _picks[id] = newRayPick;
    });
    return id;
}