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

#include <bitset>
#include <type_traits>

namespace EntityPsuedoPropertyFlag {
    enum {
        None = 0,
        FlagsActive,
        ID,
        Type,
        Created,
        Age,
        AgeAsText,
        LastEdited,
        BoundingBox,
        OriginalTextures,
        RenderInfo,
        ClientOnly,
        OwningAvatarID,

        NumFlags
    };
}
typedef std::bitset<EntityPsuedoPropertyFlag::NumFlags> EntityPsuedoPropertyFlags;

#endif // hifi_EntityPsuedoPropertyFlag_h
