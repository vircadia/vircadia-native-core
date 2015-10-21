//
//  Created by Bradley Austin Davis on 2015/10/18
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Input.h"

namespace controller {

    const Input Input::INVALID_INPUT = Input(UINT32_MAX);
    const uint16_t Input::INVALID_DEVICE = INVALID_INPUT.getDevice();
    const uint16_t Input::INVALID_CHANNEL = INVALID_INPUT.getChannel();
    const uint16_t Input::INVALID_TYPE = (uint16_t)INVALID_INPUT.getType();

}

