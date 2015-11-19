//
//  Created by Bradley Austin Davis 2015/10/23
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "CompositeEndpoint.h"

#include "../../UserInputMapper.h"

namespace controller {

CompositeEndpoint::CompositeEndpoint(Endpoint::Pointer first, Endpoint::Pointer second)
    : Endpoint(Input::INVALID_INPUT), Pair(first, second) {
    if (first->getInput().device == UserInputMapper::STANDARD_DEVICE &&
        second->getInput().device == UserInputMapper::STANDARD_DEVICE) {
        this->_input.device = UserInputMapper::STANDARD_DEVICE;
    }
}

bool CompositeEndpoint::readable() const {
    return first->readable() && second->readable();
}

float CompositeEndpoint::peek() const {
    float result = first->peek() * -1.0f + second->peek();
    return result;
}

// Fetching via value() must trigger any side effects of value() on the children
float CompositeEndpoint::value() {
    float result = first->value() * -1.0f + second->value();
    return result;
}

void CompositeEndpoint::apply(float newValue, const Pointer& source) {
    // Composites are read only
}

}
