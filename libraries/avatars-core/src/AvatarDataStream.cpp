//
//  AvatarDataStream.cpp
//  libraries/avatars-core/src
//
//  Created by Nshan G. on 9 May 2022.
//  Copyright 2013 High Fidelity, Inc.
//  Copyright 2022 Vircadia contributors.
//  Copyright 2022 DigiSomni LLC.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AvatarDataStream.h"

#include <cstdio>
#include <cstring>
#include <stdint.h>

#include <QtCore/QDataStream>
#include <QtCore/QThread>
#include <QtCore/QUuid>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

#include <shared/QtHelpers.h>
#include <QVariantGLM.h>
#include <NetworkAccessManager.h>
#include <NodeList.h>
#include <udt/PacketHeaders.h>
#include <GLMHelpers.h>
#include <StreamUtils.h>
#include <UUID.h>
#include <ShapeInfo.h>
#include <AudioHelpers.h>
#include <Profile.h>
#include <VariantMapToScriptValue.h>
#include <BitVectorHelpers.h>

#include "AvatarLogging.h"
#include "AvatarTraits.h"
#include "ClientTraitsHandler.h"
#include "ResourceRequestObserver.h"

size_t AvatarDataPacket::maxFaceTrackerInfoSize(size_t numBlendshapeCoefficients) {
    return FACE_TRACKER_INFO_SIZE + numBlendshapeCoefficients * sizeof(float);
}

size_t AvatarDataPacket::maxJointDataSize(size_t numJoints) {
    const size_t validityBitsSize = calcBitVectorSize((int)numJoints);

    size_t totalSize = sizeof(uint8_t); // numJoints

    totalSize += validityBitsSize; // Orientations mask
    totalSize += numJoints * sizeof(SixByteQuat); // Orientations
    totalSize += validityBitsSize; // Translations mask
    totalSize += sizeof(float); // maxTranslationDimension
    totalSize += numJoints * sizeof(SixByteTrans); // Translations
    return totalSize;
}

size_t AvatarDataPacket::minJointDataSize(size_t numJoints) {
    const size_t validityBitsSize = calcBitVectorSize((int)numJoints);

    size_t totalSize = sizeof(uint8_t); // numJoints

    totalSize += validityBitsSize; // Orientations mask
    // assume no valid rotations
    totalSize += validityBitsSize; // Translations mask
    totalSize += sizeof(float); // maxTranslationDimension
    // assume no valid translations

    return totalSize;
}

size_t AvatarDataPacket::maxJointDefaultPoseFlagsSize(size_t numJoints) {
    const size_t bitVectorSize = calcBitVectorSize((int)numJoints);
    size_t totalSize = sizeof(uint8_t); // numJoints

    // one set of bits for rotation and one for translation
    const size_t NUM_BIT_VECTORS_IN_DEFAULT_POSE_FLAGS_SECTION = 2;
    totalSize += NUM_BIT_VECTORS_IN_DEFAULT_POSE_FLAGS_SECTION * bitVectorSize;

    return totalSize;
}

/*@jsdoc
 * Information on an attachment worn by the avatar.
 * @typedef {object} AttachmentData
 * @property {string} modelUrl - The URL of the glTF, FBX, or OBJ model file. glTF models may be in JSON or binary format
 *     (".gltf" or ".glb" URLs respectively).
 * @property {string} jointName - The name of the joint that the attachment is parented to.
 * @property {Vec3} translation - The offset from the joint that the attachment is positioned at.
 * @property {Vec3} rotation - The rotation applied to the model relative to the joint orientation.
 * @property {number} scale - The scale applied to the attachment model.
 * @property {boolean} soft - If <code>true</code> and the model has a skeleton, the bones of the attached model's skeleton are
 *   rotated to fit the avatar's current pose. If <code>true</code>, the <code>translation</code>, <code>rotation</code>, and
 *   <code>scale</code> parameters are ignored.
 */
QVariant AttachmentData::toVariant() const {
    QVariantMap result;
    result["modelUrl"] = modelURL;
    result["jointName"] = jointName;
    result["translation"] = vec3ToQMap(translation);
    result["rotation"] = vec3ToQMap(glm::degrees(safeEulerAngles(rotation)));
    result["scale"] = scale;
    result["soft"] = isSoft;
    return result;
}

glm::vec3 variantToVec3(const QVariant& var) {
    auto map = var.toMap();
    glm::vec3 result;
    result.x = map["x"].toFloat();
    result.y = map["y"].toFloat();
    result.z = map["z"].toFloat();
    return result;
}

bool AttachmentData::fromVariant(const QVariant& variant) {
    bool isValid = false;
    auto map = variant.toMap();
    if (map.contains("modelUrl")) {
        auto urlString = map["modelUrl"].toString();
        modelURL = urlString;
        isValid = true;
    }
    if (map.contains("jointName")) {
        jointName = map["jointName"].toString();
    }
    if (map.contains("translation")) {
        translation = variantToVec3(map["translation"]);
    }
    if (map.contains("rotation")) {
        rotation = glm::quat(glm::radians(variantToVec3(map["rotation"])));
    }
    if (map.contains("scale")) {
        scale = map["scale"].toFloat();
    }
    if (map.contains("soft")) {
        isSoft = map["soft"].toBool();
    }
    return isValid;
}
