//
//  Created by Bradley Austin Davis 2015/10/23
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ActionEndpoint.h"

#include <DependencyManager.h>

#include "../../UserInputMapper.h"

using namespace controller;

void ActionEndpoint::apply(float newValue, const Pointer& source) {
    _currentValue += newValue;
    if (_input != Input::INVALID_INPUT) {
        auto userInputMapper = DependencyManager::get<UserInputMapper>();
        userInputMapper->deltaActionState(Action(_input.getChannel()), newValue);
    }
}

void ActionEndpoint::apply(const Pose& value, const Pointer& source) {
    _currentPose = value;
    if (!_currentPose.isValid()) {
        return;
    }
    if (_input != Input::INVALID_INPUT) {
        auto userInputMapper = DependencyManager::get<UserInputMapper>();
        userInputMapper->setActionState(Action(_input.getChannel()), _currentPose);
    }
}

void ActionEndpoint::reset() {
    _currentValue = 0.0f;
    _currentPose = Pose();
}

