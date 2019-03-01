//
//  Created by Sam Gondelman on 11/30/18
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "BillboardMode.h"

const char* billboardModeNames[] = {
    "none",
    "yaw",
    "full"
};

static const size_t BILLBOARD_MODE_NAMES = (sizeof(billboardModeNames) / sizeof(billboardModeNames[0]));

QString BillboardModeHelpers::getNameForBillboardMode(BillboardMode mode) {
    if (((int)mode <= 0) || ((int)mode >= (int)BILLBOARD_MODE_NAMES)) {
        mode = (BillboardMode)0;
    }

    return billboardModeNames[(int)mode];
}