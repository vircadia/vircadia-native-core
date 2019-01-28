//
//  Created by Sam Gondelman on 1/15/19
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "PulseMode.h"

const char* pulseModeNames[] = {
    "none",
    "in",
    "out"
};

static const size_t PULSE_MODE_NAMES = (sizeof(pulseModeNames) / sizeof(pulseModeNames[0]));

QString PulseModeHelpers::getNameForPulseMode(PulseMode mode) {
    if (((int)mode <= 0) || ((int)mode >= (int)PULSE_MODE_NAMES)) {
        mode = (PulseMode)0;
    }

    return pulseModeNames[(int)mode];
}