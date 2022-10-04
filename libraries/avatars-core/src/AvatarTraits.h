//
//  AvatarTraits.h
//  libraries/avatars-core/src
//
//  Created by Nshan G. on 21 Apr 2022.
//  Copyright 2018 High Fidelity, Inc.
//  Copyright 2022 Vircadia contributors.
//  Copyright 2022 DigiSomni LLC.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef LIBRARIES_AVATARS_CORE_SRC_AVATARTRAITS_H
#define LIBRARIES_AVATARS_CORE_SRC_AVATARTRAITS_H

#include <algorithm>
#include <cstdint>
#include <array>
#include <vector>

#include <QtCore/QUuid>

class ExtendedIODevice;

namespace AvatarTraits {
    enum TraitType : int8_t {
        // Null trait
        NullTrait = -1,

        // Simple traits
        SkeletonModelURL = 0,
        SkeletonData,
        // Instanced traits
        FirstInstancedTrait,
        AvatarEntity = FirstInstancedTrait,
        Grab,

        // Traits count
        TotalTraitTypes
    };

    const int NUM_SIMPLE_TRAITS = (int)FirstInstancedTrait;
    const int NUM_INSTANCED_TRAITS = (int)TotalTraitTypes - (int)FirstInstancedTrait;
    const int NUM_TRAITS = (int)TotalTraitTypes;

    using TraitInstanceID = QUuid;

    inline bool isSimpleTrait(TraitType traitType) {
        return traitType > NullTrait && traitType < FirstInstancedTrait;
    }

    using TraitVersion = int32_t;
    const TraitVersion DEFAULT_TRAIT_VERSION = 0;
    const TraitVersion NULL_TRAIT_VERSION = -1;

    using TraitWireSize = int16_t;
    const TraitWireSize DELETED_TRAIT_SIZE = -1;
    const TraitWireSize MAXIMUM_TRAIT_SIZE = INT16_MAX;

    using TraitMessageSequence = int64_t;
    const TraitMessageSequence FIRST_TRAIT_SEQUENCE = 0;
    const TraitMessageSequence MAX_TRAIT_SEQUENCE = INT64_MAX;


    template <typename AvatarDataStream>
    qint64 packTrait(TraitType traitType, ExtendedIODevice& destination, const AvatarDataStream& avatar);

    template <typename AvatarDataStream>
    qint64 packVersionedTrait(TraitType traitType, ExtendedIODevice& destination,
                              TraitVersion traitVersion, const AvatarDataStream& avatar);

    template <typename AvatarDataStream>
    qint64 packTraitInstance(TraitType traitType, TraitInstanceID traitInstanceID,

                             ExtendedIODevice& destination, AvatarDataStream& avatar);

    template <typename AvatarDataStream>
    qint64 packVersionedTraitInstance(TraitType traitType, TraitInstanceID traitInstanceID,
                                      ExtendedIODevice& destination, TraitVersion traitVersion,
                                      AvatarDataStream& avatar);

    qint64 packInstancedTraitDelete(TraitType traitType, TraitInstanceID instanceID, ExtendedIODevice& destination,
                                           TraitVersion traitVersion = NULL_TRAIT_VERSION);

};

#endif /* end of include guard */
