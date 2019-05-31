//
//  Created by Sam Gondelman on 5/31/19
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ComponentMode.h"

const char* componentModeNames[] = {
    "inherit",
    "disabled",
    "enabled"
};

QString ComponentModeHelpers::getNameForComponentMode(ComponentMode mode) {
    if (((int)mode <= 0) || ((int)mode >= (int)COMPONENT_MODE_ITEM_COUNT)) {
        mode = (ComponentMode)0;
    }

    return componentModeNames[(int)mode];
}

const char* avatarPriorityModeNames[] = {
    "inherit",
    "crowd",
    "hero"
};

QString AvatarPriorityModeHelpers::getNameForAvatarPriorityMode(AvatarPriorityMode mode) {
    if (((int)mode <= 0) || ((int)mode >= (int)AVATAR_PRIORITY_ITEM_COUNT)) {
        mode = (AvatarPriorityMode)0;
    }

    return avatarPriorityModeNames[(int)mode];
}