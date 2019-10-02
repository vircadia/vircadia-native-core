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

void ActionEndpoint::apply(AxisValue newValue, const Pointer& source) {
    auto userInputMapper = DependencyManager::get<UserInputMapper>();
    InputRecorder* inputRecorder = InputRecorder::getInstance();
    if (inputRecorder->isPlayingback() || inputRecorder->isRecording()) {
        QString actionName = userInputMapper->getActionName(Action(_input.getChannel()));
        inputRecorder->setActionState(actionName, newValue.value);
    }
    
    _currentValue.value += newValue.value;

    if (_input != Input::INVALID_INPUT) {
        userInputMapper->deltaActionState(Action(_input.getChannel()), newValue.value, newValue.valid);
    }
}

void ActionEndpoint::apply(const Pose& value, const Pointer& source) {
    _currentPose = value;
    auto userInputMapper = DependencyManager::get<UserInputMapper>();
    InputRecorder* inputRecorder = InputRecorder::getInstance();
    if (inputRecorder->isRecording()) {
        QString actionName = userInputMapper->getActionName(Action(_input.getChannel()));
        inputRecorder->setActionState(actionName, _currentPose);
    }
    
    if (!_currentPose.isValid()) {
        return;
    }
    if (_input != Input::INVALID_INPUT) {
        userInputMapper->setActionState(Action(_input.getChannel()), _currentPose);
    }
}

void ActionEndpoint::reset() {
    _currentValue = AxisValue();
    _currentPose = Pose();
}

