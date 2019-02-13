//
//  Created by Sam Gondelman on 1/7/19
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "PrimitiveMode.h"

const char* primitiveModeNames[] = {
    "solid",
    "lines"
};

static const size_t PRIMITIVE_MODE_NAMES = (sizeof(primitiveModeNames) / sizeof(primitiveModeNames[0]));

QString PrimitiveModeHelpers::getNameForPrimitiveMode(PrimitiveMode mode) {
    if (((int)mode <= 0) || ((int)mode >= (int)PRIMITIVE_MODE_NAMES)) {
        mode = (PrimitiveMode)0;
    }

    return primitiveModeNames[(int)mode];
}