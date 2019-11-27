//
//  ComponentMode.h
//  libraries/entities/src
//
//  Created by Nissim Hadar on 9/21/17.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ComponentMode_h
#define hifi_ComponentMode_h

#include <QString>

enum ComponentMode {
    COMPONENT_MODE_INHERIT,
    COMPONENT_MODE_DISABLED,
    COMPONENT_MODE_ENABLED,

    COMPONENT_MODE_ITEM_COUNT
};

enum AvatarPriorityMode {
    AVATAR_PRIORITY_INHERIT,
    AVATAR_PRIORITY_CROWD,
    AVATAR_PRIORITY_HERO,

    AVATAR_PRIORITY_ITEM_COUNT
};

class ComponentModeHelpers {
public:
    static QString getNameForComponentMode(ComponentMode mode);
};

class AvatarPriorityModeHelpers {
public:
    static QString getNameForAvatarPriorityMode(AvatarPriorityMode mode);
};

#endif // hifi_ComponentMode_h

