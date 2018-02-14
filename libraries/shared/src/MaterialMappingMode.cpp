//
//  Created by Sam Gondelman on 1/17/2018
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "MaterialMappingMode.h"

const char* materialMappingModeNames[] = {
    "uv",
    "projected"
};

static const size_t MATERIAL_MODE_NAMES = (sizeof(materialMappingModeNames) / sizeof((materialMappingModeNames)[0]));

QString MaterialMappingModeHelpers::getNameForMaterialMappingMode(MaterialMappingMode mode) {
    if (((int)mode <= 0) || ((int)mode >= (int)MATERIAL_MODE_NAMES)) {
        mode = (MaterialMappingMode)0;
    }

    return materialMappingModeNames[(int)mode];
}