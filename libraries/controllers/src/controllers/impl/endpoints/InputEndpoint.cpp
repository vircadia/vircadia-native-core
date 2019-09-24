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

AxisValue InputEndpoint::peek() const {
    if (isPose()) {
        return peekPose().valid ? AxisValue(1.0f, 0) : AxisValue(0.0f, 0, false);
    }
    auto userInputMapper = DependencyManager::get<UserInputMapper>();
    auto deviceProxy = userInputMapper->getDevice(_input);
    if (!deviceProxy) {
        return AxisValue();
    }
    return deviceProxy->getValue(_input);
}

AxisValue InputEndpoint::value() {
    _read = true;
    return peek();
}

Pose InputEndpoint::peekPose() const {
    if (!isPose()) {
        return Pose();
    }
    auto userInputMapper = DependencyManager::get<UserInputMapper>();
    auto deviceProxy = userInputMapper->getDevice(_input);
    if (!deviceProxy) {
        return Pose();
    }
    return deviceProxy->getPose(_input.channel);
}

Pose InputEndpoint::pose() {
    _read = true;
    return peekPose();
}

