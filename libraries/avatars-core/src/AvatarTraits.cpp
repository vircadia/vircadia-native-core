//
//  AvatarTraits.hpp
//  libraries/avatars-core/src
//
//  Created by Nshan G. on 21 Apr 2022.
//  Copyright 2019 High Fidelity, Inc.
//  Copyright 2022 Vircadia contributors.
//  Copyright 2022 DigiSomni LLC.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AvatarTraits.h"

#include <ExtendedIODevice.h>

namespace AvatarTraits {
    qint64 packInstancedTraitDelete(TraitType traitType, TraitInstanceID instanceID, ExtendedIODevice& destination,
                                         TraitVersion traitVersion) {
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
