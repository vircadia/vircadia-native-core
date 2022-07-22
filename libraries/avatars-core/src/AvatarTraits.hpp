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

#ifndef LIBRARIES_AVATARS_CORE_SRC_AVATARTRAITS_HPP
#define LIBRARIES_AVATARS_CORE_SRC_AVATARTRAITS_HPP

#include "AvatarTraits.h"

#include <ExtendedIODevice.h>

namespace AvatarTraits {

    template <typename AvatarDataStream>
    qint64 packTrait(TraitType traitType, ExtendedIODevice& destination, const AvatarDataStream& avatar) {
        // Call packer function
        auto traitBinaryData = avatar.packTrait(traitType);
        auto traitBinaryDataSize = traitBinaryData.size();

        // Verify packed data
        if (traitBinaryDataSize > MAXIMUM_TRAIT_SIZE) {
            qWarning() << "Refusing to pack simple trait" << traitType << "of size" << traitBinaryDataSize
                        << "bytes since it exceeds the maximum size" << MAXIMUM_TRAIT_SIZE << "bytes";
            return 0;
        }

        // Write packed data to stream
        qint64 bytesWritten = 0;
        bytesWritten += destination.writePrimitive((TraitType)traitType);
        bytesWritten += destination.writePrimitive((TraitWireSize)traitBinaryDataSize);
        bytesWritten += destination.write(traitBinaryData);
        return bytesWritten;
    }

    template <typename AvatarDataStream>
    qint64 packVersionedTrait(TraitType traitType, ExtendedIODevice& destination,
                              TraitVersion traitVersion, const AvatarDataStream& avatar) {
        // Call packer function
        auto traitBinaryData = avatar.packTrait(traitType);
        auto traitBinaryDataSize = traitBinaryData.size();

        // Verify packed data
        if (traitBinaryDataSize > MAXIMUM_TRAIT_SIZE) {
            qWarning() << "Refusing to pack simple trait" << traitType << "of size" << traitBinaryDataSize
                        << "bytes since it exceeds the maximum size" << MAXIMUM_TRAIT_SIZE << "bytes";
            return 0;
        }

        // Write packed data to stream
        qint64 bytesWritten = 0;
        bytesWritten += destination.writePrimitive((TraitType)traitType);
        bytesWritten += destination.writePrimitive((TraitVersion)traitVersion);
        bytesWritten += destination.writePrimitive((TraitWireSize)traitBinaryDataSize);
        bytesWritten += destination.write(traitBinaryData);
        return bytesWritten;
    }


    template <typename AvatarDataStream>
    qint64 packTraitInstance(TraitType traitType, TraitInstanceID traitInstanceID,
                             ExtendedIODevice& destination, AvatarDataStream& avatar) {
        // Call packer function
        auto traitBinaryData = avatar.packTraitInstance(traitType, traitInstanceID);
        auto traitBinaryDataSize = traitBinaryData.size();


        // Verify packed data
        if (traitBinaryDataSize > AvatarTraits::MAXIMUM_TRAIT_SIZE) {
            qWarning() << "Refusing to pack instanced trait" << traitType << "of size" << traitBinaryDataSize
                        << "bytes since it exceeds the maximum size " << AvatarTraits::MAXIMUM_TRAIT_SIZE << "bytes";
            return 0;
        }

        // Write packed data to stream
        qint64 bytesWritten = 0;
        bytesWritten += destination.writePrimitive((TraitType)traitType);
        bytesWritten += destination.write(traitInstanceID.toRfc4122());

        if (!traitBinaryData.isNull()) {
            bytesWritten += destination.writePrimitive((TraitWireSize)traitBinaryDataSize);
            bytesWritten += destination.write(traitBinaryData);
        } else {
            bytesWritten += destination.writePrimitive(AvatarTraits::DELETED_TRAIT_SIZE);
        }

        return bytesWritten;
    }

    template <typename AvatarDataStream>
    qint64 packVersionedTraitInstance(TraitType traitType, TraitInstanceID traitInstanceID,
                                      ExtendedIODevice& destination, TraitVersion traitVersion,
                                      AvatarDataStream& avatar) {
        // Call packer function
        auto traitBinaryData = avatar.packTraitInstance(traitType, traitInstanceID);
        auto traitBinaryDataSize = traitBinaryData.size();


        // Verify packed data
        if (traitBinaryDataSize > AvatarTraits::MAXIMUM_TRAIT_SIZE) {
            qWarning() << "Refusing to pack instanced trait" << traitType << "of size" << traitBinaryDataSize
                        << "bytes since it exceeds the maximum size " << AvatarTraits::MAXIMUM_TRAIT_SIZE << "bytes";
            return 0;
        }

        // Write packed data to stream
        qint64 bytesWritten = 0;
        bytesWritten += destination.writePrimitive((TraitType)traitType);
        bytesWritten += destination.writePrimitive((TraitVersion)traitVersion);
        bytesWritten += destination.write(traitInstanceID.toRfc4122());

        if (!traitBinaryData.isNull()) {
            bytesWritten += destination.writePrimitive((TraitWireSize)traitBinaryDataSize);
            bytesWritten += destination.write(traitBinaryData);
        } else {
            bytesWritten += destination.writePrimitive(AvatarTraits::DELETED_TRAIT_SIZE);
        }

        return bytesWritten;
    }
};

#endif /* end of include guard */
