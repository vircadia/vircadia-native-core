//
//  StateController.cpp
//  controllers/src/controllers
//
//  Created by Sam Gateau on 2015-10-27.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "StateController.h"

#include <PathUtils.h>

#include "DeviceProxy.h"
#include "UserInputMapper.h"
#include "impl/Endpoint.h"

namespace controller {

StateController::StateController() : InputDevice("Application") {
}

StateController::~StateController() {
}

void StateController::update(float deltaTime, const InputCalibrationData& inputCalibrationData, bool jointsCaptured) {}

void StateController::focusOutEvent() {}

void StateController::addInputVariant(QString name, ReadLambda lambda) {
    _namedReadLambdas.push_back(NamedReadLambda(name, lambda));
}

Input::NamedVector StateController::getAvailableInputs() const {
    Input::NamedVector availableInputs;
    int i = 0;
    for (auto& pair : _namedReadLambdas) {
        availableInputs.push_back(Input::NamedPair(Input(_deviceID, i, ChannelType::BUTTON), pair.first));
        i++;
    }
    return availableInputs;
}

EndpointPointer StateController::createEndpoint(const Input& input) const {
    return std::make_shared<LambdaEndpoint>(_namedReadLambdas[input.getChannel()].second);
}

}