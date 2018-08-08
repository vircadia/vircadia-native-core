//
//  AvatarTraits.h
//  libraries/avatars/src
//
//  Created by Stephen Birarda on 7/30/18.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AvatarTraits_h
#define hifi_AvatarTraits_h

#include <cstdint>
#include <set>
#include <vector>

namespace AvatarTraits {
    enum TraitType : int8_t {
        NullTrait = -1,
        SkeletonModelURL,
        TotalTraitTypes
    };

    using TraitTypeSet = std::set<TraitType>;
    const TraitTypeSet SimpleTraitTypes = { SkeletonModelURL };

    using TraitVersion = uint32_t;
    const TraitVersion DEFAULT_TRAIT_VERSION = 0;

    using NullableTraitVersion = int64_t;
    const NullableTraitVersion NULL_TRAIT_VERSION = -1;

    using TraitWireSize = uint16_t;

    using SimpleTraitVersions = std::vector<TraitVersion>;
}

#endif // hifi_AvatarTraits_h
