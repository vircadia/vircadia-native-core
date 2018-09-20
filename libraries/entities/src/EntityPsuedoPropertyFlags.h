//
//  EntityPsuedoPropertyFlags.h
//  libraries/entities/src
//
//  Created by Thijs Wenker on 9/18/18.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#ifndef hifi_EntityPsuedoPropertyFlag_h
#define hifi_EntityPsuedoPropertyFlag_h

enum class EntityPsuedoPropertyFlag {
    none = 0,
    flagsActive = 1 << 0,
    id = 1 << 1,
    type = 1 << 2,
    created = 1 << 3,
    age = 1 << 4,
    ageAsText = 1 << 5,
    lastEdited = 1 << 6,
    boundingBox = 1 << 7,
    originalTextures = 1 << 8,
    renderInfo = 1 << 9,
    clientOnly = 1 << 10,
    owningAvatarID = 1 << 11,
    all = (1 << 12) - 1
};
Q_DECLARE_FLAGS(EntityPsuedoPropertyFlags, EntityPsuedoPropertyFlag)

#endif // hifi_EntityPsuedoPropertyFlag_h
