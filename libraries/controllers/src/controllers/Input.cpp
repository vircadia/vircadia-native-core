//
//  Created by Bradley Austin Davis on 2015/10/18
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Input.h"

namespace controller {
    const Input Input::INVALID_INPUT = Input(0x7fffffff);
    const uint16_t Input::INVALID_DEVICE = Input::INVALID_INPUT.device;
    const uint16_t Input::INVALID_CHANNEL = Input::INVALID_INPUT.channel;
    const uint16_t Input::INVALID_TYPE = Input::INVALID_INPUT.type;
}

