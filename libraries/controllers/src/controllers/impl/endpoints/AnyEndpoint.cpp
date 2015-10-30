//
//  Created by Bradley Austin Davis 2015/10/23
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AnyEndpoint.h"

#include "../../UserInputMapper.h"

using namespace controller;

AnyEndpoint::AnyEndpoint(Endpoint::List children) : Endpoint(Input::INVALID_INPUT), _children(children) {
    bool standard = true;
    // Ensure if we're building a composite of standard devices the composite itself
    // is treated as a standard device for rule processing order
    for (auto endpoint : children) {
        if (endpoint->getInput().device != UserInputMapper::STANDARD_DEVICE) {
            standard = false;
            break;
        }
    }
    if (standard) {
        this->_input.device = UserInputMapper::STANDARD_DEVICE;
    }
}

float AnyEndpoint::peek() const {
    for (auto& child : _children) {
        float childResult = child->peek();
        if (childResult != 0.0f) {
            return childResult;
        }
    }
    return 0.0f;
}

// Fetching the value must trigger any necessary side effects of value() on ALL the children.
float AnyEndpoint::value() {
    float result = 0;
    for (auto& child : _children) {
        float childResult = child->value();
        if (childResult != 0.0f) {
            result = childResult;
        }
    }
    return result;
}

void AnyEndpoint::apply(float newValue, const Endpoint::Pointer& source) {
    qFatal("AnyEndpoint is read only");
}

// AnyEndpoint is used for reading, so return false if any child returns false (has been written to)
bool AnyEndpoint::writeable() const {
    for (auto& child : _children) {
        if (!child->writeable()) {
            return false;
        }
    }
    return true;
}

bool AnyEndpoint::readable() const {
    for (auto& child : _children) {
        if (!child->readable()) {
            return false;
        }
    }
    return true;
}

