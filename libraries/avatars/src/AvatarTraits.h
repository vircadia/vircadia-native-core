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

#include <algorithm>
#include <cstdint>
#include <vector>

namespace AvatarTraits {
    enum TraitType : int8_t {
        NullTrait = -1,
        SkeletonModelURL,
        TotalTraitTypes
    };

    class TraitTypeSet {
    public:
        TraitTypeSet() {};
        
        TraitTypeSet(std::initializer_list<TraitType> types) {
            for (auto type : types) {
                _types[type] = true;
            }
        };

        bool contains(TraitType type) const { return _types[type]; }

        bool hasAny() const { return std::find(_types.begin(), _types.end(), true) != _types.end(); }
        int size() const { return std::count(_types.begin(), _types.end(), true); }

        void insert(TraitType type) { _types[type] = true; }
        void erase(TraitType type) { _types[type] = false; }
        void clear() { std::fill(_types.begin(), _types.end(), false); }
    private:
        std::vector<bool> _types = { AvatarTraits::TotalTraitTypes, false };
    };

    const TraitTypeSet SimpleTraitTypes = { SkeletonModelURL };

    using TraitVersion = uint32_t;
    const TraitVersion DEFAULT_TRAIT_VERSION = 0;

    using NullableTraitVersion = int64_t;
    const NullableTraitVersion NULL_TRAIT_VERSION = -1;

    using TraitWireSize = uint16_t;

    using SimpleTraitVersions = std::vector<TraitVersion>;
};

#endif // hifi_AvatarTraits_h
