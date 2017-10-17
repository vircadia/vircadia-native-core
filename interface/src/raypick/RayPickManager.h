//
//  RayPickManager.h
//  interface/src/raypick
//
//  Created by Sam Gondelman 7/11/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_RayPickManager_h
#define hifi_RayPickManager_h

#include "PickManager.h"

class RayPickResult;

class RayPickManager : public PickManager<PickRay> {

public:
    QUuid createRayPick(const std::string& jointName, const glm::vec3& posOffset, const glm::vec3& dirOffset, const PickFilter& filter, const float maxDistance, const bool enabled);
    QUuid createRayPick(const PickFilter& filter, const float maxDistance, const bool enabled);
    QUuid createRayPick(const glm::vec3& position, const glm::vec3& direction, const PickFilter& filter, const float maxDistance, const bool enabled);
};

#endif // hifi_RayPickManager_h