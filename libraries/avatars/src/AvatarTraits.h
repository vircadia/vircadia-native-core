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

namespace AvatarTraits {
    enum Trait : uint8_t {
        SkeletonModelURL,
        TotalTraits
    };

    using TraitVersion = uint32_t;
    const TraitVersion DEFAULT_TRAIT_VERSION = 0;

    using TraitVersions = std::vector<TraitVersion>;
}

#endif // hifi_AvatarTraits_h
