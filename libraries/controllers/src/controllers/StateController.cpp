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

void StateController::update(float deltaTime, bool jointsCaptured) {}

void StateController::focusOutEvent() {}

void StateController::addInputVariant(QString name, ReadLambda& lambda) {
    namedReadLambdas.push_back(NamedReadLambda(name, lambda));
}
void StateController::buildDeviceProxy(DeviceProxy::Pointer proxy) {
    proxy->_name = _name;
    proxy->getButton = [this] (const Input& input, int timestamp) -> bool { return getButton(input.getChannel()); };
    proxy->getAxis = [this] (const Input& input, int timestamp) -> float { return getAxis(input.getChannel()); };
    proxy->getAvailabeInputs = [this] () -> QVector<Input::NamedPair> {
    
        
        QVector<Input::NamedPair> availableInputs;
        
        int i = 0;
        for (auto pair : namedReadLambdas) {
            availableInputs.push_back(Input::NamedPair(Input(_deviceID, i, ChannelType::BUTTON), pair.first));
            i++;
        }
        return availableInputs;
    };
    proxy->createEndpoint = [this] (const Input& input) -> Endpoint::Pointer {
        if (input.getChannel() < namedReadLambdas.size()) {
            return std::make_shared<LambdaEndpoint>(namedReadLambdas[input.getChannel()].second);
        }

        return Endpoint::Pointer();
    };
}


}
