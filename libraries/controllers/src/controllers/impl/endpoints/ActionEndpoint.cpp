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
#include "../../InputRecorder.h"

using namespace controller;

void ActionEndpoint::apply(float newValue, const Pointer& source) {
    InputRecorder* inputRecorder = InputRecorder::getInstance();
    if(inputRecorder->isPlayingback()) {
        newValue = inputRecorder->getActionState(Action(_input.getChannel()));
    }
    
    _currentValue += newValue;
    if (_input != Input::INVALID_INPUT) {
        auto userInputMapper = DependencyManager::get<UserInputMapper>();
        userInputMapper->deltaActionState(Action(_input.getChannel()), newValue);
    }
    inputRecorder->setActionState(Action(_input.getChannel()), newValue);
}

void ActionEndpoint::apply(const Pose& value, const Pointer& source) {
    _currentPose = value;
    InputRecorder* inputRecorder = InputRecorder::getInstance();
    inputRecorder->setActionState(Action(_input.getChannel()), _currentPose);
    if (inputRecorder->isPlayingback()) {
        _currentPose = inputRecorder->getPoseState(Action(_input.getChannel()));
    }
    
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

