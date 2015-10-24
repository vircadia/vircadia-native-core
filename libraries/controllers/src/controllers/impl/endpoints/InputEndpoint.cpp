//
//  Created by Bradley Austin Davis 2015/10/23
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "InputEndpoint.h"

#include <DependencyManager.h>

#include "../../UserInputMapper.h"

using namespace controller;
float InputEndpoint::value(){
    _read = true;
    if (isPose()) {
        return pose().valid ? 1.0f : 0.0f;
    }
    auto userInputMapper = DependencyManager::get<UserInputMapper>();
    auto deviceProxy = userInputMapper->getDeviceProxy(_input);
    if (!deviceProxy) {
        return 0.0f;
    }
    return deviceProxy->getValue(_input, 0);
}

Pose InputEndpoint::pose() {
    _read = true;
    if (!isPose()) {
        return Pose();
    }
    auto userInputMapper = DependencyManager::get<UserInputMapper>();
    auto deviceProxy = userInputMapper->getDeviceProxy(_input);
    if (!deviceProxy) {
        return Pose();
    }
    return deviceProxy->getPose(_input, 0);
}

