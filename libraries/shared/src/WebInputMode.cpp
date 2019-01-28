//
//  Created by Sam Gondelman on 1/9/19
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "WebInputMode.h"

const char* webInputModeNames[] = {
    "touch",
    "mouse"
};

static const size_t WEB_INPUT_MODE_NAMES = (sizeof(webInputModeNames) / sizeof(webInputModeNames[0]));

QString WebInputModeHelpers::getNameForWebInputMode(WebInputMode mode) {
    if (((int)mode <= 0) || ((int)mode >= (int)WEB_INPUT_MODE_NAMES)) {
        mode = (WebInputMode)0;
    }

    return webInputModeNames[(int)mode];
}