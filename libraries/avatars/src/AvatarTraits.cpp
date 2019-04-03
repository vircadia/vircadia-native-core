//
//  AvatarTraits.cpp
//  libraries/avatars/src
//
//  Created by Clement Brisset on 3/19/19.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AvatarTraits.h"

#include <ExtendedIODevice.h>

#include "AvatarData.h"

namespace AvatarTraits {

    qint64 packTrait(TraitType traitType, ExtendedIODevice& destination, const AvatarData& avatar) {
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

    qint64 packVersionedTrait(TraitType traitType, ExtendedIODevice& destination,
                              TraitVersion traitVersion, const AvatarData& avatar) {
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


    qint64 packTraitInstance(TraitType traitType, TraitInstanceID traitInstanceID,
                             ExtendedIODevice& destination, AvatarData& avatar) {
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

    qint64 packVersionedTraitInstance(TraitType traitType, TraitInstanceID traitInstanceID,
                                      ExtendedIODevice& destination, TraitVersion traitVersion,
                                      AvatarData& avatar) {
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
