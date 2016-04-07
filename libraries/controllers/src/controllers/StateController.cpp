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

static QStringList stateVariables;

void StateController::setStateVariables(const QStringList& newStateVariables) {
    stateVariables = newStateVariables;
}

StateController::StateController() : InputDevice("Application") {
    _deviceID = UserInputMapper::STATE_DEVICE;
    for (const auto& variable : stateVariables) {
        _namedReadLambdas[variable] = []()->float{ return 0; };
    }
}

void StateController::setInputVariant(const QString& name, ReadLambda lambda) {
    // All state variables must be predeclared;
    Q_ASSERT(_namedReadLambdas.contains(name));
    _namedReadLambdas[name] = lambda;
}

Input::NamedVector StateController::getAvailableInputs() const {
    Input::NamedVector availableInputs;
    int i = 0;
    for (const auto& name : stateVariables) {
        availableInputs.push_back(Input::NamedPair(Input(_deviceID, i, ChannelType::BUTTON), name));
        i++;
    }
    return availableInputs;
}

EndpointPointer StateController::createEndpoint(const Input& input) const {
    auto name = stateVariables[input.getChannel()];
    ReadLambda& readLambda = const_cast<QHash<QString, ReadLambda>&>(_namedReadLambdas)[name];
    return std::make_shared<LambdaRefEndpoint>(readLambda);
}

}