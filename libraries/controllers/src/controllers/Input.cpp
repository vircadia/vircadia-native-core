//
//  Created by Bradley Austin Davis on 2015/10/18
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Input.h"

namespace controller {

    const uint16_t Input::INVALID_DEVICE = 0xffff;
    const uint16_t Input::INVALID_CHANNEL = 0x1fff;
    const uint16_t Input::INVALID_TYPE = (uint16_t)ChannelType::INVALID;
    const Input Input::INVALID_INPUT = Input(INVALID_DEVICE, INVALID_CHANNEL, ChannelType::INVALID);
}

