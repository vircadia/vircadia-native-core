//
//  Created by Sam Gondelman on 1/22/19.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "GizmoType.h"

const char* gizmoTypeNames[] = {
    "ring",
};

static const size_t GIZMO_TYPE_NAMES = (sizeof(gizmoTypeNames) / sizeof(gizmoTypeNames[0]));

QString GizmoTypeHelpers::getNameForGizmoType(GizmoType mode) {
    if (((int)mode <= 0) || ((int)mode >= (int)GIZMO_TYPE_NAMES)) {
        mode = (GizmoType)0;
    }

    return gizmoTypeNames[(int)mode];
}