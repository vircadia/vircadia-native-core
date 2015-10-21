//
//  Created by Bradley Austin Davis on 2015/10/18
//  (based on UserInputMapper inner class created by Sam Gateau on 4/27/15)
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "DeviceProxy.h"

namespace controller {

    float DeviceProxy::getValue(const Input& input, int timestamp) const {
        switch (input.getType()) {
        case ChannelType::BUTTON:
            return getButton(input, timestamp) ? 1.0f : 0.0f;

        case ChannelType::AXIS:
            return getAxis(input, timestamp);

        case ChannelType::POSE:
            return getPose(input, timestamp).valid ? 1.0f : 0.0f;

        default:
            return NAN;
        }
    }

}

