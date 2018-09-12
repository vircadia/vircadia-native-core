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

#include <QtCore/QUuid>

namespace AvatarTraits {
    enum TraitType : int8_t {
        NullTrait = -1,
        SkeletonModelURL,
        FirstInstancedTrait,
        AvatarEntity = FirstInstancedTrait,
        TotalTraitTypes
    };

    using TraitInstanceID = QUuid;

    inline bool isSimpleTrait(TraitType traitType) {
        return traitType > NullTrait && traitType < FirstInstancedTrait;
    }

    using TraitVersion = int32_t;
    const TraitVersion DEFAULT_TRAIT_VERSION = 0;
    const TraitVersion NULL_TRAIT_VERSION = -1;

    using TraitWireSize = int16_t;
    const TraitWireSize DELETED_TRAIT_SIZE = -1;

    inline qint64 packInstancedTraitDelete(TraitType traitType, TraitInstanceID instanceID, ExtendedIODevice& destination,
                                         TraitVersion traitVersion = NULL_TRAIT_VERSION) {
        qint64 bytesWritten = 0;

        bytesWritten += destination.writePrimitive(traitType);

        if (traitVersion > DEFAULT_TRAIT_VERSION) {
            bytesWritten += destination.writePrimitive(traitVersion);
        }

        bytesWritten += destination.write(instanceID.toRfc4122());

        bytesWritten += destination.writePrimitive(DELETED_TRAIT_SIZE);

        return bytesWritten;
    }
};

#endif // hifi_AvatarTraits_h
