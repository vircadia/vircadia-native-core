//
//  AvatarData.cpp
//  libraries/avatars/src
//
//  Created by Stephen Birarda on 4/9/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#include "AvatarData.h"

#include <cstdio>
#include <cstring>
#include <stdint.h>

#include <QtCore/QDataStream>
#include <QtCore/QThread>
#include <QtCore/QUuid>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

#include <shared/QtHelpers.h>
#include <QVariantGLM.h>
#include <Transform.h>
#include <NetworkAccessManager.h>
#include <NodeList.h>
#include <udt/PacketHeaders.h>
#include <GLMHelpers.h>
#include <StreamUtils.h>
#include <UUID.h>
#include <shared/JSONHelpers.h>
#include <ShapeInfo.h>
#include <AudioHelpers.h>
#include <Profile.h>
#include <VariantMapToScriptValue.h>
#include <BitVectorHelpers.h>

#include "AvatarLogging.h"
#include "AvatarTraits.h"
#include "ClientTraitsHandler.h"

//#define WANT_DEBUG

quint64 DEFAULT_FILTERED_LOG_EXPIRY = 2 * USECS_PER_SECOND;

using namespace std;

const QString AvatarData::FRAME_NAME = "com.highfidelity.recording.AvatarData";

static const int TRANSLATION_COMPRESSION_RADIX = 12;
static const int SENSOR_TO_WORLD_SCALE_RADIX = 10;
static const float AUDIO_LOUDNESS_SCALE = 1024.0f;
static const float DEFAULT_AVATAR_DENSITY = 1000.0f; // density of water

#define ASSERT(COND)  do { if (!(COND)) { abort(); } } while(0)

size_t AvatarDataPacket::maxFaceTrackerInfoSize(size_t numBlendshapeCoefficients) {
    return FACE_TRACKER_INFO_SIZE + numBlendshapeCoefficients * sizeof(float);
}

size_t AvatarDataPacket::maxJointDataSize(size_t numJoints, bool hasGrabJoints) {
    const size_t validityBitsSize = (size_t)std::ceil(numJoints / (float)BITS_IN_BYTE);

    size_t totalSize = sizeof(uint8_t); // numJoints

    totalSize += validityBitsSize; // Orientations mask
    totalSize += numJoints * sizeof(SixByteQuat); // Orientations
    totalSize += validityBitsSize; // Translations mask
    totalSize += numJoints * sizeof(SixByteTrans); // Translations

    size_t NUM_FAUX_JOINT = 2;
    size_t num_grab_joints = (hasGrabJoints ? 2 : 0);
    totalSize += (NUM_FAUX_JOINT + num_grab_joints) * (sizeof(SixByteQuat) + sizeof(SixByteTrans)); // faux joints

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

AvatarData::AvatarData() :
    SpatiallyNestable(NestableType::Avatar, QUuid()),
    _handPosition(0.0f),
    _targetScale(1.0f),
    _handState(0),
    _keyState(NO_KEY_DOWN),
    _forceFaceTrackerConnected(false),
    _headData(NULL),
    _errorLogExpiry(0),
    _owningAvatarMixer(),
    _targetVelocity(0.0f),
    _density(DEFAULT_AVATAR_DENSITY)
{
    connect(this, &AvatarData::lookAtSnappingChanged, this, &AvatarData::markIdentityDataChanged);
}

AvatarData::~AvatarData() {
    delete _headData;
}

// We cannot have a file-level variable (const or otherwise) in the AvatarInfo if it uses PathUtils, because that references Application, which will not yet initialized.
// Thus we have a static class getter, referencing a static class var.
QUrl AvatarData::_defaultFullAvatarModelUrl = {}; // In C++, if this initialization were in the AvatarInfo, every file would have it's own copy, even for class vars.
const QUrl& AvatarData::defaultFullAvatarModelUrl() {
    if (_defaultFullAvatarModelUrl.isEmpty()) {
        _defaultFullAvatarModelUrl = PathUtils::resourcesUrl("/meshes/defaultAvatar_full.fst");
    }
    return _defaultFullAvatarModelUrl;
}

void AvatarData::setTargetScale(float targetScale) {
    auto newValue = glm::clamp(targetScale, MIN_AVATAR_SCALE, MAX_AVATAR_SCALE);
    if (_targetScale != newValue) {
        _targetScale = newValue;
        _scaleChanged = usecTimestampNow();
        _avatarScaleChanged = _scaleChanged;
    }
}

float AvatarData::getDomainLimitedScale() const {
    if (canMeasureEyeHeight()) {
        const float minScale = getDomainMinScale();
        const float maxScale = getDomainMaxScale();
        return glm::clamp(_targetScale, minScale, maxScale);
    } else {
        // We can't make a good estimate.
        return _targetScale;
    }
}

void AvatarData::setDomainMinimumHeight(float domainMinimumHeight) {
    _domainMinimumHeight = glm::clamp(domainMinimumHeight, MIN_AVATAR_HEIGHT, MAX_AVATAR_HEIGHT);
}

void AvatarData::setDomainMaximumHeight(float domainMaximumHeight) {
    _domainMaximumHeight = glm::clamp(domainMaximumHeight, MIN_AVATAR_HEIGHT, MAX_AVATAR_HEIGHT);
}

float AvatarData::getDomainMinScale() const {
    float unscaledHeight = getUnscaledHeight();
    const float EPSILON = 1.0e-4f;
    if (unscaledHeight <= EPSILON) {
        unscaledHeight = DEFAULT_AVATAR_HEIGHT;
    }
    return _domainMinimumHeight / unscaledHeight;
}

float AvatarData::getDomainMaxScale() const {
    float unscaledHeight = getUnscaledHeight();
    const float EPSILON = 1.0e-4f;
    if (unscaledHeight <= EPSILON) {
        unscaledHeight = DEFAULT_AVATAR_HEIGHT;
    }
    return _domainMaximumHeight / unscaledHeight;
}

float AvatarData::getUnscaledHeight() const {
    const float eyeHeight = getUnscaledEyeHeight();
    const float ratio = eyeHeight / DEFAULT_AVATAR_HEIGHT;
    return eyeHeight + ratio * DEFAULT_AVATAR_EYE_TO_TOP_OF_HEAD;
}

float AvatarData::getHeight() const {
    const float eyeHeight = getEyeHeight();
    const float ratio = eyeHeight / DEFAULT_AVATAR_HEIGHT;
    return eyeHeight + ratio * DEFAULT_AVATAR_EYE_TO_TOP_OF_HEAD;
}

glm::vec3 AvatarData::getHandPosition() const {
    return getWorldOrientation() * _handPosition + getWorldPosition();
}

void AvatarData::setHandPosition(const glm::vec3& handPosition) {
    // store relative to position/orientation
    _handPosition = glm::inverse(getWorldOrientation()) * (handPosition - getWorldPosition());
}

void AvatarData::lazyInitHeadData() const {
    // lazily allocate memory for HeadData in case we're not an Avatar instance
    if (!_headData) {
        _headData = new HeadData(const_cast<AvatarData*>(this));
    }
    if (_forceFaceTrackerConnected) {
        _headData->_isFaceTrackerConnected = true;
    }
}


float AvatarData::getDistanceBasedMinRotationDOT(glm::vec3 viewerPosition) const {
    auto distance = glm::distance(_globalPosition, viewerPosition);
    float result = ROTATION_CHANGE_179D; // assume worst
    if (distance < AVATAR_DISTANCE_LEVEL_1) {
        result = AVATAR_MIN_ROTATION_DOT;
    } else if (distance < AVATAR_DISTANCE_LEVEL_2) {
        result = ROTATION_CHANGE_15D;
    } else if (distance < AVATAR_DISTANCE_LEVEL_3) {
        result = ROTATION_CHANGE_45D;
    } else if (distance < AVATAR_DISTANCE_LEVEL_4) {
        result = ROTATION_CHANGE_90D;
    }
    return result;
}

float AvatarData::getDistanceBasedMinTranslationDistance(glm::vec3 viewerPosition) const {
    return AVATAR_MIN_TRANSLATION; // Eventually make this distance sensitive as well
}


// we want to track outbound data in this case...
QByteArray AvatarData::toByteArrayStateful(AvatarDataDetail dataDetail, bool dropFaceTracking) {
    AvatarDataPacket::HasFlags hasFlagsOut;
    auto lastSentTime = _lastToByteArray;
    _lastToByteArray = usecTimestampNow();
    return AvatarData::toByteArray(dataDetail, lastSentTime, getLastSentJointData(),
                        hasFlagsOut, dropFaceTracking, false, glm::vec3(0), nullptr,
                        &_outboundDataRate);
}

QByteArray AvatarData::toByteArray(AvatarDataDetail dataDetail, quint64 lastSentTime,
                                   const QVector<JointData>& lastSentJointData,
    AvatarDataPacket::HasFlags& hasFlagsOut, bool dropFaceTracking, bool distanceAdjust,
    glm::vec3 viewerPosition, QVector<JointData>* sentJointDataOut, AvatarDataRate* outboundDataRateOut) const {

    bool cullSmallChanges = (dataDetail == CullSmallData);
    bool sendAll = (dataDetail == SendAllData);
    bool sendMinimum = (dataDetail == MinimumData);
    bool sendPALMinimum = (dataDetail == PALMinimum);

    lazyInitHeadData();

    // special case, if we were asked for no data, then just include the flags all set to nothing
    if (dataDetail == NoData) {
        AvatarDataPacket::HasFlags packetStateFlags = 0;
        QByteArray avatarDataByteArray(reinterpret_cast<char*>(&packetStateFlags), sizeof(packetStateFlags));
        return avatarDataByteArray;
    }

    // FIXME -
    //
    //    BUG -- if you enter a space bubble, and then back away, the avatar has wrong orientation until "send all" happens...
    //      this is an iFrame issue... what to do about that?
    //
    //    BUG -- Resizing avatar seems to "take too long"... the avatar doesn't redraw at smaller size right away
    //
    // TODO consider these additional optimizations in the future
    // 1) SensorToWorld - should we only send this for avatars with attachments?? - 20 bytes - 7.20 kbps
    // 2) GUIID for the session change to 2byte index                   (savings) - 14 bytes - 5.04 kbps
    // 3) Improve Joints -- currently we use rotational tolerances, but if we had skeleton/bone length data
    //    we could do a better job of determining if the change in joints actually translates to visible
    //    changes at distance.
    //
    //    Potential savings:
    //              63 rotations   * 6 bytes = 136kbps
    //              3 translations * 6 bytes = 6.48kbps
    //

    auto parentID = getParentID();

    bool hasAvatarGlobalPosition = true; // always include global position
    bool hasAvatarOrientation = false;
    bool hasAvatarBoundingBox = false;
    bool hasAvatarScale = false;
    bool hasLookAtPosition = false;
    bool hasAudioLoudness = false;
    bool hasSensorToWorldMatrix = false;
    bool hasAdditionalFlags = false;

    // local position, and parent info only apply to avatars that are parented. The local position
    // and the parent info can change independently though, so we track their "changed since"
    // separately
    bool hasParentInfo = false;
    bool hasAvatarLocalPosition = false;

    bool hasFaceTrackerInfo = false;
    bool hasJointData = false;
    bool hasJointDefaultPoseFlags = false;
    bool hasGrabJoints = false;

    glm::mat4 leftFarGrabMatrix;
    glm::mat4 rightFarGrabMatrix;
    glm::mat4 mouseFarGrabMatrix;

    if (sendPALMinimum) {
        hasAudioLoudness = true;
    } else {
        hasAvatarOrientation = sendAll || rotationChangedSince(lastSentTime);
        hasAvatarBoundingBox = sendAll || avatarBoundingBoxChangedSince(lastSentTime);
        hasAvatarScale = sendAll || avatarScaleChangedSince(lastSentTime);
        hasLookAtPosition = sendAll || lookAtPositionChangedSince(lastSentTime);
        hasAudioLoudness = sendAll || audioLoudnessChangedSince(lastSentTime);
        hasSensorToWorldMatrix = sendAll || sensorToWorldMatrixChangedSince(lastSentTime);
        hasAdditionalFlags = sendAll || additionalFlagsChangedSince(lastSentTime);
        hasParentInfo = sendAll || parentInfoChangedSince(lastSentTime);
        hasAvatarLocalPosition = hasParent() && (sendAll ||
            tranlationChangedSince(lastSentTime) ||
            parentInfoChangedSince(lastSentTime));

        hasFaceTrackerInfo = !dropFaceTracking && (hasFaceTracker() || getHasScriptedBlendshapes()) &&
            (sendAll || faceTrackerInfoChangedSince(lastSentTime));
        hasJointData = sendAll || !sendMinimum;
        hasJointDefaultPoseFlags = hasJointData;
        if (hasJointData) {
            bool leftValid;
            leftFarGrabMatrix = _farGrabLeftMatrixCache.get(leftValid);
            if (!leftValid) {
                leftFarGrabMatrix = glm::mat4();
            }
            bool rightValid;
            rightFarGrabMatrix = _farGrabRightMatrixCache.get(rightValid);
            if (!rightValid) {
                rightFarGrabMatrix = glm::mat4();
            }
            bool mouseValid;
            mouseFarGrabMatrix = _farGrabMouseMatrixCache.get(mouseValid);
            if (!mouseValid) {
                mouseFarGrabMatrix = glm::mat4();
            }
            hasGrabJoints = (leftValid || rightValid || mouseValid);
        }
    }


    const size_t byteArraySize = AvatarDataPacket::MAX_CONSTANT_HEADER_SIZE +
        (hasFaceTrackerInfo ? AvatarDataPacket::maxFaceTrackerInfoSize(_headData->getBlendshapeCoefficients().size()) : 0) +
        (hasJointData ? AvatarDataPacket::maxJointDataSize(_jointData.size(), hasGrabJoints) : 0) +
        (hasJointDefaultPoseFlags ? AvatarDataPacket::maxJointDefaultPoseFlagsSize(_jointData.size()) : 0);

    QByteArray avatarDataByteArray((int)byteArraySize, 0);
    unsigned char* destinationBuffer = reinterpret_cast<unsigned char*>(avatarDataByteArray.data());
    unsigned char* startPosition = destinationBuffer;

    // Leading flags, to indicate how much data is actually included in the packet...
    AvatarDataPacket::HasFlags packetStateFlags =
        (hasAvatarGlobalPosition ? AvatarDataPacket::PACKET_HAS_AVATAR_GLOBAL_POSITION : 0)
        | (hasAvatarBoundingBox ? AvatarDataPacket::PACKET_HAS_AVATAR_BOUNDING_BOX : 0)
        | (hasAvatarOrientation ? AvatarDataPacket::PACKET_HAS_AVATAR_ORIENTATION : 0)
        | (hasAvatarScale ? AvatarDataPacket::PACKET_HAS_AVATAR_SCALE : 0)
        | (hasLookAtPosition ? AvatarDataPacket::PACKET_HAS_LOOK_AT_POSITION : 0)
        | (hasAudioLoudness ? AvatarDataPacket::PACKET_HAS_AUDIO_LOUDNESS : 0)
        | (hasSensorToWorldMatrix ? AvatarDataPacket::PACKET_HAS_SENSOR_TO_WORLD_MATRIX : 0)
        | (hasAdditionalFlags ? AvatarDataPacket::PACKET_HAS_ADDITIONAL_FLAGS : 0)
        | (hasParentInfo ? AvatarDataPacket::PACKET_HAS_PARENT_INFO : 0)
        | (hasAvatarLocalPosition ? AvatarDataPacket::PACKET_HAS_AVATAR_LOCAL_POSITION : 0)
        | (hasFaceTrackerInfo ? AvatarDataPacket::PACKET_HAS_FACE_TRACKER_INFO : 0)
        | (hasJointData ? AvatarDataPacket::PACKET_HAS_JOINT_DATA : 0)
        | (hasJointDefaultPoseFlags ? AvatarDataPacket::PACKET_HAS_JOINT_DEFAULT_POSE_FLAGS : 0)
        | (hasGrabJoints ? AvatarDataPacket::PACKET_HAS_GRAB_JOINTS : 0);

    memcpy(destinationBuffer, &packetStateFlags, sizeof(packetStateFlags));
    destinationBuffer += sizeof(packetStateFlags);

#define AVATAR_MEMCPY(src)                          \
    memcpy(destinationBuffer, &(src), sizeof(src)); \
    destinationBuffer += sizeof(src);

    if (hasAvatarGlobalPosition) {
        auto startSection = destinationBuffer;
        AVATAR_MEMCPY(_globalPosition);

        int numBytes = destinationBuffer - startSection;

        if (outboundDataRateOut) {
            outboundDataRateOut->globalPositionRate.increment(numBytes);
        }
    }

    if (hasAvatarBoundingBox) {
        auto startSection = destinationBuffer;
        AVATAR_MEMCPY(_globalBoundingBoxDimensions);
        AVATAR_MEMCPY(_globalBoundingBoxOffset);

        int numBytes = destinationBuffer - startSection;
        if (outboundDataRateOut) {
            outboundDataRateOut->avatarBoundingBoxRate.increment(numBytes);
        }
    }

    if (hasAvatarOrientation) {
        auto startSection = destinationBuffer;
        auto localOrientation = getOrientationOutbound();
        destinationBuffer += packOrientationQuatToSixBytes(destinationBuffer, localOrientation);

        int numBytes = destinationBuffer - startSection;
        if (outboundDataRateOut) {
            outboundDataRateOut->avatarOrientationRate.increment(numBytes);
        }
    }

    if (hasAvatarScale) {
        auto startSection = destinationBuffer;
        auto data = reinterpret_cast<AvatarDataPacket::AvatarScale*>(destinationBuffer);
        auto scale = getDomainLimitedScale();
        packFloatRatioToTwoByte((uint8_t*)(&data->scale), scale);
        destinationBuffer += sizeof(AvatarDataPacket::AvatarScale);

        int numBytes = destinationBuffer - startSection;
        if (outboundDataRateOut) {
            outboundDataRateOut->avatarScaleRate.increment(numBytes);
        }
    }

    if (hasLookAtPosition) {
        auto startSection = destinationBuffer;
        AVATAR_MEMCPY(_headData->getLookAtPosition());
        int numBytes = destinationBuffer - startSection;
        if (outboundDataRateOut) {
            outboundDataRateOut->lookAtPositionRate.increment(numBytes);
        }
    }

    if (hasAudioLoudness) {
        auto startSection = destinationBuffer;
        auto data = reinterpret_cast<AvatarDataPacket::AudioLoudness*>(destinationBuffer);
        data->audioLoudness = packFloatGainToByte(getAudioLoudness() / AUDIO_LOUDNESS_SCALE);
        destinationBuffer += sizeof(AvatarDataPacket::AudioLoudness);

        int numBytes = destinationBuffer - startSection;
        if (outboundDataRateOut) {
            outboundDataRateOut->audioLoudnessRate.increment(numBytes);
        }
    }

    if (hasSensorToWorldMatrix) {
        auto startSection = destinationBuffer;
        auto data = reinterpret_cast<AvatarDataPacket::SensorToWorldMatrix*>(destinationBuffer);
        glm::mat4 sensorToWorldMatrix = getSensorToWorldMatrix();
        packOrientationQuatToSixBytes(data->sensorToWorldQuat, glmExtractRotation(sensorToWorldMatrix));
        glm::vec3 scale = extractScale(sensorToWorldMatrix);
        packFloatScalarToSignedTwoByteFixed((uint8_t*)&data->sensorToWorldScale, scale.x, SENSOR_TO_WORLD_SCALE_RADIX);
        data->sensorToWorldTrans[0] = sensorToWorldMatrix[3][0];
        data->sensorToWorldTrans[1] = sensorToWorldMatrix[3][1];
        data->sensorToWorldTrans[2] = sensorToWorldMatrix[3][2];
        destinationBuffer += sizeof(AvatarDataPacket::SensorToWorldMatrix);

        int numBytes = destinationBuffer - startSection;
        if (outboundDataRateOut) {
            outboundDataRateOut->sensorToWorldRate.increment(numBytes);
        }
    }

    if (hasAdditionalFlags) {
        auto startSection = destinationBuffer;
        auto data = reinterpret_cast<AvatarDataPacket::AdditionalFlags*>(destinationBuffer);

        uint16_t flags { 0 };

        setSemiNibbleAt(flags, KEY_STATE_START_BIT, _keyState);

        // hand state
        bool isFingerPointing = _handState & IS_FINGER_POINTING_FLAG;
        setSemiNibbleAt(flags, HAND_STATE_START_BIT, _handState & ~IS_FINGER_POINTING_FLAG);
        if (isFingerPointing) {
            setAtBit16(flags, HAND_STATE_FINGER_POINTING_BIT);
        }
        // face tracker state
        if (_headData->_isFaceTrackerConnected) {
            setAtBit16(flags, IS_FACE_TRACKER_CONNECTED);
        }
        // eye tracker state
        if (_headData->_isEyeTrackerConnected) {
            setAtBit16(flags, IS_EYE_TRACKER_CONNECTED);
        }
        // referential state
        if (!parentID.isNull()) {
            setAtBit16(flags, HAS_REFERENTIAL);
        }
        // audio face movement
        if (_headData->getHasAudioEnabledFaceMovement()) {
            setAtBit16(flags, AUDIO_ENABLED_FACE_MOVEMENT);
        }
        // procedural eye face movement
        if (_headData->getHasProceduralEyeFaceMovement()) {
            setAtBit16(flags, PROCEDURAL_EYE_FACE_MOVEMENT);
        }
        // procedural blink face movement
        if (_headData->getHasProceduralBlinkFaceMovement()) {
            setAtBit16(flags, PROCEDURAL_BLINK_FACE_MOVEMENT);
        }

        data->flags = flags;
        destinationBuffer += sizeof(AvatarDataPacket::AdditionalFlags);

        int numBytes = destinationBuffer - startSection;
        if (outboundDataRateOut) {
            outboundDataRateOut->additionalFlagsRate.increment(numBytes);
        }
    }

    if (hasParentInfo) {
        auto startSection = destinationBuffer;
        auto parentInfo = reinterpret_cast<AvatarDataPacket::ParentInfo*>(destinationBuffer);
        QByteArray referentialAsBytes = parentID.toRfc4122();
        memcpy(parentInfo->parentUUID, referentialAsBytes.data(), referentialAsBytes.size());
        parentInfo->parentJointIndex = getParentJointIndex();
        destinationBuffer += sizeof(AvatarDataPacket::ParentInfo);

        int numBytes = destinationBuffer - startSection;
        if (outboundDataRateOut) {
            outboundDataRateOut->parentInfoRate.increment(numBytes);
        }
    }

    if (hasAvatarLocalPosition) {
        auto startSection = destinationBuffer;
        const auto localPosition = getLocalPosition();
        AVATAR_MEMCPY(localPosition);

        int numBytes = destinationBuffer - startSection;
        if (outboundDataRateOut) {
            outboundDataRateOut->localPositionRate.increment(numBytes);
        }
    }

    // If it is connected, pack up the data
    if (hasFaceTrackerInfo) {
        auto startSection = destinationBuffer;
        auto faceTrackerInfo = reinterpret_cast<AvatarDataPacket::FaceTrackerInfo*>(destinationBuffer);
        const auto& blendshapeCoefficients = _headData->getBlendshapeCoefficients();
        // note: we don't use the blink and average loudness, we just use the numBlendShapes and
        // compute the procedural info on the client side.
        faceTrackerInfo->leftEyeBlink = _headData->_leftEyeBlink;
        faceTrackerInfo->rightEyeBlink = _headData->_rightEyeBlink;
        faceTrackerInfo->averageLoudness = _headData->_averageLoudness;
        faceTrackerInfo->browAudioLift = _headData->_browAudioLift;
        faceTrackerInfo->numBlendshapeCoefficients = blendshapeCoefficients.size();
        destinationBuffer += sizeof(AvatarDataPacket::FaceTrackerInfo);

        memcpy(destinationBuffer, blendshapeCoefficients.data(), blendshapeCoefficients.size() * sizeof(float));
        destinationBuffer += blendshapeCoefficients.size() * sizeof(float);

        int numBytes = destinationBuffer - startSection;
        if (outboundDataRateOut) {
            outboundDataRateOut->faceTrackerRate.increment(numBytes);
        }
    }

    QVector<JointData> jointData;
    if (hasJointData || hasJointDefaultPoseFlags) {
        QReadLocker readLock(&_jointDataLock);
        jointData = _jointData;
    }

    // If it is connected, pack up the data
    if (hasJointData) {
        auto startSection = destinationBuffer;

        // joint rotation data
        int numJoints = jointData.size();
        *destinationBuffer++ = (uint8_t)numJoints;

        unsigned char* validityPosition = destinationBuffer;
        unsigned char validity = 0;
        int validityBit = 0;
        int numValidityBytes = calcBitVectorSize(numJoints);

#ifdef WANT_DEBUG
        int rotationSentCount = 0;
        unsigned char* beforeRotations = destinationBuffer;
#endif

        destinationBuffer += numValidityBytes; // Move pointer past the validity bytes

        // sentJointDataOut and lastSentJointData might be the same vector
        if (sentJointDataOut) {
            sentJointDataOut->resize(numJoints); // Make sure the destination is resized before using it
        }

        float minRotationDOT = (distanceAdjust && cullSmallChanges) ? getDistanceBasedMinRotationDOT(viewerPosition) : AVATAR_MIN_ROTATION_DOT;

        for (int i = 0; i < jointData.size(); i++) {
            const JointData& data = jointData[i];
            const JointData& last = lastSentJointData[i];

            if (!data.rotationIsDefaultPose) {
                // The dot product for larger rotations is a lower number.
                // So if the dot() is less than the value, then the rotation is a larger angle of rotation
                if (sendAll || last.rotationIsDefaultPose || (!cullSmallChanges && last.rotation != data.rotation)
                    || (cullSmallChanges && glm::dot(last.rotation, data.rotation) < minRotationDOT) ) {
                    validity |= (1 << validityBit);
#ifdef WANT_DEBUG
                    rotationSentCount++;
#endif
                    destinationBuffer += packOrientationQuatToSixBytes(destinationBuffer, data.rotation);

                    if (sentJointDataOut) {
                        (*sentJointDataOut)[i].rotation = data.rotation;
                    }
                }
            }

            if (sentJointDataOut) {
                (*sentJointDataOut)[i].rotationIsDefaultPose = data.rotationIsDefaultPose;
            }

            if (++validityBit == BITS_IN_BYTE) {
                *validityPosition++ = validity;
                validityBit = validity = 0;
            }
        }
        if (validityBit != 0) {
            *validityPosition++ = validity;
        }

        // joint translation data
        validityPosition = destinationBuffer;
        validity = 0;
        validityBit = 0;

#ifdef WANT_DEBUG
        int translationSentCount = 0;
        unsigned char* beforeTranslations = destinationBuffer;
#endif

        destinationBuffer += numValidityBytes; // Move pointer past the validity bytes

        float minTranslation = (distanceAdjust && cullSmallChanges) ? getDistanceBasedMinTranslationDistance(viewerPosition) : AVATAR_MIN_TRANSLATION;

        float maxTranslationDimension = 0.0;
        for (int i = 0; i < jointData.size(); i++) {
            const JointData& data = jointData[i];
            const JointData& last = lastSentJointData[i];

            if (!data.translationIsDefaultPose) {
                if (sendAll || last.translationIsDefaultPose || (!cullSmallChanges && last.translation != data.translation)
                    || (cullSmallChanges && glm::distance(data.translation, lastSentJointData[i].translation) > minTranslation)) {
                   
                    validity |= (1 << validityBit);
#ifdef WANT_DEBUG
                    translationSentCount++;
#endif
                    maxTranslationDimension = glm::max(fabsf(data.translation.x), maxTranslationDimension);
                    maxTranslationDimension = glm::max(fabsf(data.translation.y), maxTranslationDimension);
                    maxTranslationDimension = glm::max(fabsf(data.translation.z), maxTranslationDimension);

                    destinationBuffer +=
                        packFloatVec3ToSignedTwoByteFixed(destinationBuffer, data.translation, TRANSLATION_COMPRESSION_RADIX);

                    if (sentJointDataOut) {
                        (*sentJointDataOut)[i].translation = data.translation;
                    }
                }
            }

            if (sentJointDataOut) {
                (*sentJointDataOut)[i].translationIsDefaultPose = data.translationIsDefaultPose;
            }

            if (++validityBit == BITS_IN_BYTE) {
                *validityPosition++ = validity;
                validityBit = validity = 0;
            }
        }

        if (validityBit != 0) {
            *validityPosition++ = validity;
        }

        // faux joints
        Transform controllerLeftHandTransform = Transform(getControllerLeftHandMatrix());
        destinationBuffer += packOrientationQuatToSixBytes(destinationBuffer, controllerLeftHandTransform.getRotation());
        destinationBuffer += packFloatVec3ToSignedTwoByteFixed(destinationBuffer, controllerLeftHandTransform.getTranslation(),
            TRANSLATION_COMPRESSION_RADIX);

        Transform controllerRightHandTransform = Transform(getControllerRightHandMatrix());
        destinationBuffer += packOrientationQuatToSixBytes(destinationBuffer, controllerRightHandTransform.getRotation());
        destinationBuffer += packFloatVec3ToSignedTwoByteFixed(destinationBuffer, controllerRightHandTransform.getTranslation(),
            TRANSLATION_COMPRESSION_RADIX);

        if (hasGrabJoints) {
            // the far-grab joints may range further than 3 meters, so we can't use packFloatVec3ToSignedTwoByteFixed etc
            auto startSection = destinationBuffer;
            auto data = reinterpret_cast<AvatarDataPacket::FarGrabJoints*>(destinationBuffer);
            glm::vec3 leftFarGrabPosition = extractTranslation(leftFarGrabMatrix);
            glm::quat leftFarGrabRotation = extractRotation(leftFarGrabMatrix);
            glm::vec3 rightFarGrabPosition = extractTranslation(rightFarGrabMatrix);
            glm::quat rightFarGrabRotation = extractRotation(rightFarGrabMatrix);
            glm::vec3 mouseFarGrabPosition = extractTranslation(mouseFarGrabMatrix);
            glm::quat mouseFarGrabRotation = extractRotation(mouseFarGrabMatrix);

            AVATAR_MEMCPY(leftFarGrabPosition);
            // Can't do block copy as struct order is x, y, z, w.
            data->leftFarGrabRotation[0] = leftFarGrabRotation.w;
            data->leftFarGrabRotation[1] = leftFarGrabRotation.x;
            data->leftFarGrabRotation[2] = leftFarGrabRotation.y;
            data->leftFarGrabRotation[3] = leftFarGrabRotation.z;
            destinationBuffer += sizeof(data->leftFarGrabPosition);

            AVATAR_MEMCPY(rightFarGrabPosition);
            data->rightFarGrabRotation[0] = rightFarGrabRotation.w;
            data->rightFarGrabRotation[1] = rightFarGrabRotation.x;
            data->rightFarGrabRotation[2] = rightFarGrabRotation.y;
            data->rightFarGrabRotation[3] = rightFarGrabRotation.z;
            destinationBuffer += sizeof(data->rightFarGrabRotation);

            AVATAR_MEMCPY(mouseFarGrabPosition);
            data->mouseFarGrabRotation[0] = mouseFarGrabRotation.w;
            data->mouseFarGrabRotation[1] = mouseFarGrabRotation.x;
            data->mouseFarGrabRotation[2] = mouseFarGrabRotation.y;
            data->mouseFarGrabRotation[3] = mouseFarGrabRotation.z;
            destinationBuffer += sizeof(data->mouseFarGrabRotation);

            int numBytes = destinationBuffer - startSection;

            if (outboundDataRateOut) {
                outboundDataRateOut->farGrabJointRate.increment(numBytes);
            }
        }

#ifdef WANT_DEBUG
        if (sendAll) {
            qCDebug(avatars) << "AvatarData::toByteArray" << cullSmallChanges << sendAll
                << "rotations:" << rotationSentCount << "translations:" << translationSentCount
                << "largest:" << maxTranslationDimension
                << "size:"
                << (beforeRotations - startPosition) << "+"
                << (beforeTranslations - beforeRotations) << "+"
                << (destinationBuffer - beforeTranslations) << "="
                << (destinationBuffer - startPosition);
        }
#endif

        int numBytes = destinationBuffer - startSection;
        if (outboundDataRateOut) {
            outboundDataRateOut->jointDataRate.increment(numBytes);
        }

    }

    if (hasJointDefaultPoseFlags) {
        auto startSection = destinationBuffer;

        // write numJoints
        int numJoints = jointData.size();
        *destinationBuffer++ = (uint8_t)numJoints;

        // write rotationIsDefaultPose bits
        destinationBuffer += writeBitVector(destinationBuffer, numJoints, [&](int i) {
            return jointData[i].rotationIsDefaultPose;
        });

        // write translationIsDefaultPose bits
        destinationBuffer += writeBitVector(destinationBuffer, numJoints, [&](int i) {
            return jointData[i].translationIsDefaultPose;
        });

        if (outboundDataRateOut) {
            size_t numBytes = destinationBuffer - startSection;
            outboundDataRateOut->jointDefaultPoseFlagsRate.increment(numBytes);
        }
    }

    int avatarDataSize = destinationBuffer - startPosition;

    if (avatarDataSize > (int)byteArraySize) {
        qCCritical(avatars) << "AvatarData::toByteArray buffer overflow"; // We've overflown into the heap
        ASSERT(false);
    }

    return avatarDataByteArray.left(avatarDataSize);
}

// NOTE: This is never used in a "distanceAdjust" mode, so it's ok that it doesn't use a variable minimum rotation/translation
void AvatarData::doneEncoding(bool cullSmallChanges) {
    // The server has finished sending this version of the joint-data to other nodes.  Update _lastSentJointData.
    QReadLocker readLock(&_jointDataLock);
    _lastSentJointData.resize(_jointData.size());
    for (int i = 0; i < _jointData.size(); i ++) {
        const JointData& data = _jointData[ i ];
        if (_lastSentJointData[i].rotation != data.rotation) {
            if (!cullSmallChanges ||
                fabsf(glm::dot(data.rotation, _lastSentJointData[i].rotation)) <= AVATAR_MIN_ROTATION_DOT) {
                if (!data.rotationIsDefaultPose) {
                    _lastSentJointData[i].rotation = data.rotation;
                }
            }
        }
        if (_lastSentJointData[i].translation != data.translation) {
            if (!cullSmallChanges ||
                glm::distance(data.translation, _lastSentJointData[i].translation) > AVATAR_MIN_TRANSLATION) {
                if (!data.translationIsDefaultPose) {
                    _lastSentJointData[i].translation = data.translation;
                }
            }
        }
    }
}

bool AvatarData::shouldLogError(const quint64& now) {
#ifdef WANT_DEBUG
    if (now > 0) {
        return true;
    }
#endif

    if (now > _errorLogExpiry) {
        _errorLogExpiry = now + DEFAULT_FILTERED_LOG_EXPIRY;
        return true;
    }
    return false;
}


const unsigned char* unpackFauxJoint(const unsigned char* sourceBuffer, ThreadSafeValueCache<glm::mat4>& matrixCache) {
    glm::quat orientation;
    glm::vec3 position;
    Transform transform;
    sourceBuffer += unpackOrientationQuatFromSixBytes(sourceBuffer, orientation);
    sourceBuffer += unpackFloatVec3FromSignedTwoByteFixed(sourceBuffer, position, TRANSLATION_COMPRESSION_RADIX);
    transform.setTranslation(position);
    transform.setRotation(orientation);
    matrixCache.set(transform.getMatrix());
    return sourceBuffer;
}


#define PACKET_READ_CHECK(ITEM_NAME, SIZE_TO_READ)                                        \
    if ((endPosition - sourceBuffer) < (int)SIZE_TO_READ) {                               \
        if (shouldLogError(now)) {                                                        \
            qCWarning(avatars) << "AvatarData packet too small, attempting to read " <<   \
                #ITEM_NAME << ", only " << (endPosition - sourceBuffer) <<                \
                " bytes left, " << getSessionUUID();                                      \
        }                                                                                 \
        return buffer.size();                                                             \
    }

// read data in packet starting at byte offset and return number of bytes parsed
int AvatarData::parseDataFromBuffer(const QByteArray& buffer) {
    // lazily allocate memory for HeadData in case we're not an Avatar instance
    lazyInitHeadData();

    AvatarDataPacket::HasFlags packetStateFlags;

    const unsigned char* startPosition = reinterpret_cast<const unsigned char*>(buffer.data());
    const unsigned char* endPosition = startPosition + buffer.size();
    const unsigned char* sourceBuffer = startPosition;

    // read the packet flags
    memcpy(&packetStateFlags, sourceBuffer, sizeof(packetStateFlags));
    sourceBuffer += sizeof(packetStateFlags);

    #define HAS_FLAG(B,F) ((B & F) == F)

    bool hasAvatarGlobalPosition  = HAS_FLAG(packetStateFlags, AvatarDataPacket::PACKET_HAS_AVATAR_GLOBAL_POSITION);
    bool hasAvatarBoundingBox     = HAS_FLAG(packetStateFlags, AvatarDataPacket::PACKET_HAS_AVATAR_BOUNDING_BOX);
    bool hasAvatarOrientation     = HAS_FLAG(packetStateFlags, AvatarDataPacket::PACKET_HAS_AVATAR_ORIENTATION);
    bool hasAvatarScale           = HAS_FLAG(packetStateFlags, AvatarDataPacket::PACKET_HAS_AVATAR_SCALE);
    bool hasLookAtPosition        = HAS_FLAG(packetStateFlags, AvatarDataPacket::PACKET_HAS_LOOK_AT_POSITION);
    bool hasAudioLoudness         = HAS_FLAG(packetStateFlags, AvatarDataPacket::PACKET_HAS_AUDIO_LOUDNESS);
    bool hasSensorToWorldMatrix   = HAS_FLAG(packetStateFlags, AvatarDataPacket::PACKET_HAS_SENSOR_TO_WORLD_MATRIX);
    bool hasAdditionalFlags       = HAS_FLAG(packetStateFlags, AvatarDataPacket::PACKET_HAS_ADDITIONAL_FLAGS);
    bool hasParentInfo            = HAS_FLAG(packetStateFlags, AvatarDataPacket::PACKET_HAS_PARENT_INFO);
    bool hasAvatarLocalPosition   = HAS_FLAG(packetStateFlags, AvatarDataPacket::PACKET_HAS_AVATAR_LOCAL_POSITION);
    bool hasFaceTrackerInfo       = HAS_FLAG(packetStateFlags, AvatarDataPacket::PACKET_HAS_FACE_TRACKER_INFO);
    bool hasJointData             = HAS_FLAG(packetStateFlags, AvatarDataPacket::PACKET_HAS_JOINT_DATA);
    bool hasJointDefaultPoseFlags = HAS_FLAG(packetStateFlags, AvatarDataPacket::PACKET_HAS_JOINT_DEFAULT_POSE_FLAGS);
    bool hasGrabJoints            = HAS_FLAG(packetStateFlags, AvatarDataPacket::PACKET_HAS_GRAB_JOINTS);

    quint64 now = usecTimestampNow();

    if (hasAvatarGlobalPosition) {
        auto startSection = sourceBuffer;

        PACKET_READ_CHECK(AvatarGlobalPosition, sizeof(AvatarDataPacket::AvatarGlobalPosition));
        auto data = reinterpret_cast<const AvatarDataPacket::AvatarGlobalPosition*>(sourceBuffer);

        glm::vec3 offset = glm::vec3(0.0f, 0.0f, 0.0f);

        if (_replicaIndex > 0) {
            const float SPACE_BETWEEN_AVATARS = 2.0f;
            const int AVATARS_PER_ROW = 3;
            int row = _replicaIndex % AVATARS_PER_ROW;
            int col = floor(_replicaIndex / AVATARS_PER_ROW);
            offset = glm::vec3(row * SPACE_BETWEEN_AVATARS, 0.0f, col * SPACE_BETWEEN_AVATARS);
        }

        auto newValue = glm::vec3(data->globalPosition[0], data->globalPosition[1], data->globalPosition[2]) + offset;
        if (_globalPosition != newValue) {
            _globalPosition = newValue;
            _globalPositionChanged = now;
        }
        sourceBuffer += sizeof(AvatarDataPacket::AvatarGlobalPosition);
        int numBytesRead = sourceBuffer - startSection;
        _globalPositionRate.increment(numBytesRead);
        _globalPositionUpdateRate.increment();

        // if we don't have a parent, make sure to also set our local position
        if (!hasParent()) {
            setLocalPosition(newValue);
        }
    }

    if (hasAvatarBoundingBox) {
        auto startSection = sourceBuffer;

        PACKET_READ_CHECK(AvatarBoundingBox, sizeof(AvatarDataPacket::AvatarBoundingBox));
        auto data = reinterpret_cast<const AvatarDataPacket::AvatarBoundingBox*>(sourceBuffer);
        auto newDimensions = glm::vec3(data->avatarDimensions[0], data->avatarDimensions[1], data->avatarDimensions[2]);
        auto newOffset = glm::vec3(data->boundOriginOffset[0], data->boundOriginOffset[1], data->boundOriginOffset[2]);


        if (_globalBoundingBoxDimensions != newDimensions) {
            _globalBoundingBoxDimensions = newDimensions;
            _avatarBoundingBoxChanged = now;
        }
        if (_globalBoundingBoxOffset != newOffset) {
            _globalBoundingBoxOffset = newOffset;
            _avatarBoundingBoxChanged = now;
        }

        sourceBuffer += sizeof(AvatarDataPacket::AvatarBoundingBox);
        int numBytesRead = sourceBuffer - startSection;
        _avatarBoundingBoxRate.increment(numBytesRead);
        _avatarBoundingBoxUpdateRate.increment();
    }

    if (hasAvatarOrientation) {
        auto startSection = sourceBuffer;
        PACKET_READ_CHECK(AvatarOrientation, sizeof(AvatarDataPacket::AvatarOrientation));
        glm::quat newOrientation;
        sourceBuffer += unpackOrientationQuatFromSixBytes(sourceBuffer, newOrientation);
        glm::quat currentOrientation = getLocalOrientation();

        if (currentOrientation != newOrientation) {
            _hasNewJointData = true;
            setLocalOrientation(newOrientation);
        }
        int numBytesRead = sourceBuffer - startSection;
        _avatarOrientationRate.increment(numBytesRead);
        _avatarOrientationUpdateRate.increment();
    }

    if (hasAvatarScale) {
        auto startSection = sourceBuffer;

        PACKET_READ_CHECK(AvatarScale, sizeof(AvatarDataPacket::AvatarScale));
        auto data = reinterpret_cast<const AvatarDataPacket::AvatarScale*>(sourceBuffer);
        float scale;
        unpackFloatRatioFromTwoByte((uint8_t*)&data->scale, scale);
        if (isNaN(scale)) {
            if (shouldLogError(now)) {
                qCWarning(avatars) << "Discard AvatarData packet: scale NaN, uuid " << getSessionUUID();
            }
            return buffer.size();
        }
        setTargetScale(scale);
        sourceBuffer += sizeof(AvatarDataPacket::AvatarScale);
        int numBytesRead = sourceBuffer - startSection;
        _avatarScaleRate.increment(numBytesRead);
        _avatarScaleUpdateRate.increment();
    }

    if (hasLookAtPosition) {
        auto startSection = sourceBuffer;

        PACKET_READ_CHECK(LookAtPosition, sizeof(AvatarDataPacket::LookAtPosition));
        auto data = reinterpret_cast<const AvatarDataPacket::LookAtPosition*>(sourceBuffer);
        glm::vec3 lookAt = glm::vec3(data->lookAtPosition[0], data->lookAtPosition[1], data->lookAtPosition[2]);
        if (isNaN(lookAt)) {
            if (shouldLogError(now)) {
                qCWarning(avatars) << "Discard AvatarData packet: lookAtPosition is NaN, uuid " << getSessionUUID();
            }
            return buffer.size();
        }
        _headData->setLookAtPosition(lookAt);
        sourceBuffer += sizeof(AvatarDataPacket::LookAtPosition);
        int numBytesRead = sourceBuffer - startSection;
        _lookAtPositionRate.increment(numBytesRead);
        _lookAtPositionUpdateRate.increment();
    }

    if (hasAudioLoudness) {
        auto startSection = sourceBuffer;

        PACKET_READ_CHECK(AudioLoudness, sizeof(AvatarDataPacket::AudioLoudness));
        auto data = reinterpret_cast<const AvatarDataPacket::AudioLoudness*>(sourceBuffer);
        float audioLoudness;
        audioLoudness = unpackFloatGainFromByte(data->audioLoudness) * AUDIO_LOUDNESS_SCALE;
        sourceBuffer += sizeof(AvatarDataPacket::AudioLoudness);

        if (isNaN(audioLoudness)) {
            if (shouldLogError(now)) {
                qCWarning(avatars) << "Discard AvatarData packet: audioLoudness is NaN, uuid " << getSessionUUID();
            }
            return buffer.size();
        }
        setAudioLoudness(audioLoudness);
        int numBytesRead = sourceBuffer - startSection;
        _audioLoudnessRate.increment(numBytesRead);
        _audioLoudnessUpdateRate.increment();
    }

    if (hasSensorToWorldMatrix) {
        auto startSection = sourceBuffer;

        PACKET_READ_CHECK(SensorToWorldMatrix, sizeof(AvatarDataPacket::SensorToWorldMatrix));
        auto data = reinterpret_cast<const AvatarDataPacket::SensorToWorldMatrix*>(sourceBuffer);
        glm::quat sensorToWorldQuat;
        unpackOrientationQuatFromSixBytes(data->sensorToWorldQuat, sensorToWorldQuat);
        float sensorToWorldScale;
        // Grab a local copy of sensorToWorldScale to be able to use the unpack function with a pointer on it,
        // a direct pointer on the struct attribute triggers warnings because of potential misalignement.
        auto srcSensorToWorldScale = data->sensorToWorldScale;
        unpackFloatScalarFromSignedTwoByteFixed((int16_t*)&srcSensorToWorldScale, &sensorToWorldScale, SENSOR_TO_WORLD_SCALE_RADIX);
        glm::vec3 sensorToWorldTrans(data->sensorToWorldTrans[0], data->sensorToWorldTrans[1], data->sensorToWorldTrans[2]);
        glm::mat4 sensorToWorldMatrix = createMatFromScaleQuatAndPos(glm::vec3(sensorToWorldScale), sensorToWorldQuat, sensorToWorldTrans);
        if (_sensorToWorldMatrixCache.get() != sensorToWorldMatrix) {
            _sensorToWorldMatrixCache.set(sensorToWorldMatrix);
            _sensorToWorldMatrixChanged = now;
        }
        sourceBuffer += sizeof(AvatarDataPacket::SensorToWorldMatrix);
        int numBytesRead = sourceBuffer - startSection;
        _sensorToWorldRate.increment(numBytesRead);
        _sensorToWorldUpdateRate.increment();
    }

    if (hasAdditionalFlags) {
        auto startSection = sourceBuffer;

        PACKET_READ_CHECK(AdditionalFlags, sizeof(AvatarDataPacket::AdditionalFlags));
        auto data = reinterpret_cast<const AvatarDataPacket::AdditionalFlags*>(sourceBuffer);
        uint16_t bitItems = data->flags;

        // key state, stored as a semi-nibble in the bitItems
        auto newKeyState = (KeyState)getSemiNibbleAt(bitItems, KEY_STATE_START_BIT);

        // hand state, stored as a semi-nibble plus a bit in the bitItems
        // we store the hand state as well as other items in a shared bitset. The hand state is an octal, but is split
        // into two sections to maintain backward compatibility. The bits are ordered as such (0-7 left to right).
        // AA 6/1/18 added three more flags bits 8,9, and 10 for procedural audio, blink, and eye saccade enabled
        //     +---+-----+-----+--+--+--+--+-----+
        //     |x,x|H0,H1|x,x,x|H2|Au|Bl|Ey|xxxxx|
        //     +---+-----+-----+--+--+--+--+-----+
        // Hand state - H0,H1,H2 is found in the 3rd, 4th, and 8th bits
        auto newHandState = getSemiNibbleAt(bitItems, HAND_STATE_START_BIT)
            + (oneAtBit16(bitItems, HAND_STATE_FINGER_POINTING_BIT) ? IS_FINGER_POINTING_FLAG : 0);

        auto newFaceTrackerConnected = oneAtBit16(bitItems, IS_FACE_TRACKER_CONNECTED);
        auto newEyeTrackerConnected = oneAtBit16(bitItems, IS_EYE_TRACKER_CONNECTED);

        auto newHasAudioEnabledFaceMovement = oneAtBit16(bitItems, AUDIO_ENABLED_FACE_MOVEMENT);
        auto newHasProceduralEyeFaceMovement = oneAtBit16(bitItems, PROCEDURAL_EYE_FACE_MOVEMENT);
        auto newHasProceduralBlinkFaceMovement = oneAtBit16(bitItems, PROCEDURAL_BLINK_FACE_MOVEMENT);

        
        bool keyStateChanged = (_keyState != newKeyState);
        bool handStateChanged = (_handState != newHandState);
        bool faceStateChanged = (_headData->_isFaceTrackerConnected != newFaceTrackerConnected);
        bool eyeStateChanged = (_headData->_isEyeTrackerConnected != newEyeTrackerConnected);
        bool audioEnableFaceMovementChanged = (_headData->getHasAudioEnabledFaceMovement() != newHasAudioEnabledFaceMovement);
        bool proceduralEyeFaceMovementChanged = (_headData->getHasProceduralEyeFaceMovement() != newHasProceduralEyeFaceMovement);
        bool proceduralBlinkFaceMovementChanged = (_headData->getHasProceduralBlinkFaceMovement() != newHasProceduralBlinkFaceMovement);
        bool somethingChanged = keyStateChanged || handStateChanged || faceStateChanged || eyeStateChanged || audioEnableFaceMovementChanged || proceduralEyeFaceMovementChanged || proceduralBlinkFaceMovementChanged;

        _keyState = newKeyState;
        _handState = newHandState;
        _headData->_isFaceTrackerConnected = newFaceTrackerConnected;
        _headData->_isEyeTrackerConnected = newEyeTrackerConnected;
        _headData->setHasAudioEnabledFaceMovement(newHasAudioEnabledFaceMovement);
        _headData->setHasProceduralEyeFaceMovement(newHasProceduralEyeFaceMovement);
        _headData->setHasProceduralBlinkFaceMovement(newHasProceduralBlinkFaceMovement);

        sourceBuffer += sizeof(AvatarDataPacket::AdditionalFlags);

        if (somethingChanged) {
            _additionalFlagsChanged = now;
        }
        int numBytesRead = sourceBuffer - startSection;
        _additionalFlagsRate.increment(numBytesRead);
        _additionalFlagsUpdateRate.increment();
    }

    if (hasParentInfo) {
        auto startSection = sourceBuffer;
        PACKET_READ_CHECK(ParentInfo, sizeof(AvatarDataPacket::ParentInfo));
        auto parentInfo = reinterpret_cast<const AvatarDataPacket::ParentInfo*>(sourceBuffer);
        sourceBuffer += sizeof(AvatarDataPacket::ParentInfo);

        QByteArray byteArray((const char*)parentInfo->parentUUID, NUM_BYTES_RFC4122_UUID);

        auto newParentID = QUuid::fromRfc4122(byteArray);

        if ((getParentID() != newParentID) || (getParentJointIndex() != parentInfo->parentJointIndex)) {
            SpatiallyNestable::setParentID(newParentID);
            SpatiallyNestable::setParentJointIndex(parentInfo->parentJointIndex);
            _parentChanged = now;
        }

        int numBytesRead = sourceBuffer - startSection;
        _parentInfoRate.increment(numBytesRead);
        _parentInfoUpdateRate.increment();
    }

    if (hasAvatarLocalPosition) {
        auto startSection = sourceBuffer;

        PACKET_READ_CHECK(AvatarLocalPosition, sizeof(AvatarDataPacket::AvatarLocalPosition));
        auto data = reinterpret_cast<const AvatarDataPacket::AvatarLocalPosition*>(sourceBuffer);
        glm::vec3 position = glm::vec3(data->localPosition[0], data->localPosition[1], data->localPosition[2]);
        if (isNaN(position)) {
            if (shouldLogError(now)) {
                qCWarning(avatars) << "Discard AvatarData packet: position NaN, uuid " << getSessionUUID();
            }
            return buffer.size();
        }
        if (hasParent()) {
            setLocalPosition(position);
        } else {
            qCWarning(avatars) << "received localPosition for avatar with no parent";
        }
        sourceBuffer += sizeof(AvatarDataPacket::AvatarLocalPosition);
        int numBytesRead = sourceBuffer - startSection;
        _localPositionRate.increment(numBytesRead);
        _localPositionUpdateRate.increment();
    }

    if (hasFaceTrackerInfo) {
        auto startSection = sourceBuffer;

        PACKET_READ_CHECK(FaceTrackerInfo, sizeof(AvatarDataPacket::FaceTrackerInfo));
        auto faceTrackerInfo = reinterpret_cast<const AvatarDataPacket::FaceTrackerInfo*>(sourceBuffer);
        int numCoefficients = faceTrackerInfo->numBlendshapeCoefficients;
        const int coefficientsSize = sizeof(float) * numCoefficients;
        sourceBuffer += sizeof(AvatarDataPacket::FaceTrackerInfo);

        PACKET_READ_CHECK(FaceTrackerCoefficients, coefficientsSize);
        _headData->_blendshapeCoefficients.resize(numCoefficients);  // make sure there's room for the copy!
        //only copy the blendshapes to headData, not the procedural face info
        memcpy(_headData->_blendshapeCoefficients.data(), sourceBuffer, coefficientsSize);
        sourceBuffer += coefficientsSize;

        int numBytesRead = sourceBuffer - startSection;
        _faceTrackerRate.increment(numBytesRead);
        _faceTrackerUpdateRate.increment();
    }

    if (hasJointData) {
        auto startSection = sourceBuffer;

        PACKET_READ_CHECK(NumJoints, sizeof(uint8_t));
        int numJoints = *sourceBuffer++;
        const int bytesOfValidity = (int)ceil((float)numJoints / (float)BITS_IN_BYTE);
        PACKET_READ_CHECK(JointRotationValidityBits, bytesOfValidity);

        int numValidJointRotations = 0;
        QVector<bool> validRotations;
        validRotations.resize(numJoints);
        { // rotation validity bits
            unsigned char validity = 0;
            int validityBit = 0;
            for (int i = 0; i < numJoints; i++) {
                if (validityBit == 0) {
                    validity = *sourceBuffer++;
                }
                bool valid = (bool)(validity & (1 << validityBit));
                if (valid) {
                    ++numValidJointRotations;
                }
                validRotations[i] = valid;
                validityBit = (validityBit + 1) % BITS_IN_BYTE;
            }
        }

        // each joint rotation is stored in 6 bytes.
        QWriteLocker writeLock(&_jointDataLock);
        _jointData.resize(numJoints);

        const int COMPRESSED_QUATERNION_SIZE = 6;
        PACKET_READ_CHECK(JointRotations, numValidJointRotations * COMPRESSED_QUATERNION_SIZE);
        for (int i = 0; i < numJoints; i++) {
            JointData& data = _jointData[i];
            if (validRotations[i]) {
                sourceBuffer += unpackOrientationQuatFromSixBytes(sourceBuffer, data.rotation);
                _hasNewJointData = true;
                data.rotationIsDefaultPose = false;
            }
        }

        PACKET_READ_CHECK(JointTranslationValidityBits, bytesOfValidity);

        // get translation validity bits -- these indicate which translations were packed
        int numValidJointTranslations = 0;
        QVector<bool> validTranslations;
        validTranslations.resize(numJoints);
        { // translation validity bits
            unsigned char validity = 0;
            int validityBit = 0;
            for (int i = 0; i < numJoints; i++) {
                if (validityBit == 0) {
                    validity = *sourceBuffer++;
                }
                bool valid = (bool)(validity & (1 << validityBit));
                if (valid) {
                    ++numValidJointTranslations;
                }
                validTranslations[i] = valid;
                validityBit = (validityBit + 1) % BITS_IN_BYTE;
            }
        } // 1 + bytesOfValidity bytes

        // each joint translation component is stored in 6 bytes.
        const int COMPRESSED_TRANSLATION_SIZE = 6;
        PACKET_READ_CHECK(JointTranslation, numValidJointTranslations * COMPRESSED_TRANSLATION_SIZE);

        for (int i = 0; i < numJoints; i++) {
            JointData& data = _jointData[i];
            if (validTranslations[i]) {
                sourceBuffer += unpackFloatVec3FromSignedTwoByteFixed(sourceBuffer, data.translation, TRANSLATION_COMPRESSION_RADIX);
                _hasNewJointData = true;
                data.translationIsDefaultPose = false;
            }
        }

#ifdef WANT_DEBUG
        if (numValidJointRotations > 15) {
            qCDebug(avatars) << "RECEIVING -- rotations:" << numValidJointRotations
                << "translations:" << numValidJointTranslations
                << "size:" << (int)(sourceBuffer - startPosition);
        }
#endif
        // faux joints
        sourceBuffer = unpackFauxJoint(sourceBuffer, _controllerLeftHandMatrixCache);
        sourceBuffer = unpackFauxJoint(sourceBuffer, _controllerRightHandMatrixCache);

        int numBytesRead = sourceBuffer - startSection;
        _jointDataRate.increment(numBytesRead);
        _jointDataUpdateRate.increment();

        if (hasGrabJoints) {
            auto startSection = sourceBuffer;

            PACKET_READ_CHECK(FarGrabJoints, sizeof(AvatarDataPacket::FarGrabJoints));
            auto data = reinterpret_cast<const AvatarDataPacket::FarGrabJoints*>(sourceBuffer);
            glm::vec3 leftFarGrabPosition = glm::vec3(data->leftFarGrabPosition[0], data->leftFarGrabPosition[1],
                                                      data->leftFarGrabPosition[2]);
            glm::quat leftFarGrabRotation = glm::quat(data->leftFarGrabRotation[0], data->leftFarGrabRotation[1],
                                                      data->leftFarGrabRotation[2], data->leftFarGrabRotation[3]);
            glm::vec3 rightFarGrabPosition = glm::vec3(data->rightFarGrabPosition[0], data->rightFarGrabPosition[1],
                                                       data->rightFarGrabPosition[2]);
            glm::quat rightFarGrabRotation = glm::quat(data->rightFarGrabRotation[0], data->rightFarGrabRotation[1],
                                                       data->rightFarGrabRotation[2], data->rightFarGrabRotation[3]);
            glm::vec3 mouseFarGrabPosition = glm::vec3(data->mouseFarGrabPosition[0], data->mouseFarGrabPosition[1],
                                                       data->mouseFarGrabPosition[2]);
            glm::quat mouseFarGrabRotation = glm::quat(data->mouseFarGrabRotation[0], data->mouseFarGrabRotation[1],
                                                       data->mouseFarGrabRotation[2], data->mouseFarGrabRotation[3]);

            _farGrabLeftMatrixCache.set(createMatFromQuatAndPos(leftFarGrabRotation, leftFarGrabPosition));
            _farGrabRightMatrixCache.set(createMatFromQuatAndPos(rightFarGrabRotation, rightFarGrabPosition));
            _farGrabMouseMatrixCache.set(createMatFromQuatAndPos(mouseFarGrabRotation, mouseFarGrabPosition));

            sourceBuffer += sizeof(AvatarDataPacket::AvatarGlobalPosition);
            int numBytesRead = sourceBuffer - startSection;
            _farGrabJointRate.increment(numBytesRead);
            _farGrabJointUpdateRate.increment();
        }
    }

    if (hasJointDefaultPoseFlags) {
        auto startSection = sourceBuffer;

        QWriteLocker writeLock(&_jointDataLock);

        PACKET_READ_CHECK(JointDefaultPoseFlagsNumJoints, sizeof(uint8_t));
        int numJoints = (int)*sourceBuffer++;

        _jointData.resize(numJoints);

        size_t bitVectorSize = calcBitVectorSize(numJoints);
        PACKET_READ_CHECK(JointDefaultPoseFlagsRotationFlags, bitVectorSize);
        sourceBuffer += readBitVector(sourceBuffer, numJoints, [&](int i, bool value) {
            _jointData[i].rotationIsDefaultPose = value;
        });

        PACKET_READ_CHECK(JointDefaultPoseFlagsTranslationFlags, bitVectorSize);
        sourceBuffer += readBitVector(sourceBuffer, numJoints, [&](int i, bool value) {
            _jointData[i].translationIsDefaultPose = value;
        });

        int numBytesRead = sourceBuffer - startSection;
        _jointDefaultPoseFlagsRate.increment(numBytesRead);
        _jointDefaultPoseFlagsUpdateRate.increment();
    }

    int numBytesRead = sourceBuffer - startPosition;
    _averageBytesReceived.updateAverage(numBytesRead);

    _parseBufferRate.increment(numBytesRead);
    _parseBufferUpdateRate.increment();

    return numBytesRead;
}

float AvatarData::getDataRate(const QString& rateName) const {
    if (rateName == "") {
        return _parseBufferRate.rate() / BYTES_PER_KILOBIT;
    } else if (rateName == "globalPosition") {
        return _globalPositionRate.rate() / BYTES_PER_KILOBIT;
    } else if (rateName == "localPosition") {
        return _localPositionRate.rate() / BYTES_PER_KILOBIT;
    } else if (rateName == "avatarBoundingBox") {
        return _avatarBoundingBoxRate.rate() / BYTES_PER_KILOBIT;
    } else if (rateName == "avatarOrientation") {
        return _avatarOrientationRate.rate() / BYTES_PER_KILOBIT;
    } else if (rateName == "avatarScale") {
        return _avatarScaleRate.rate() / BYTES_PER_KILOBIT;
    } else if (rateName == "lookAtPosition") {
        return _lookAtPositionRate.rate() / BYTES_PER_KILOBIT;
    } else if (rateName == "audioLoudness") {
        return _audioLoudnessRate.rate() / BYTES_PER_KILOBIT;
    } else if (rateName == "sensorToWorkMatrix") {
        return _sensorToWorldRate.rate() / BYTES_PER_KILOBIT;
    } else if (rateName == "additionalFlags") {
        return _additionalFlagsRate.rate() / BYTES_PER_KILOBIT;
    } else if (rateName == "parentInfo") {
        return _parentInfoRate.rate() / BYTES_PER_KILOBIT;
    } else if (rateName == "faceTracker") {
        return _faceTrackerRate.rate() / BYTES_PER_KILOBIT;
    } else if (rateName == "jointData") {
        return _jointDataRate.rate() / BYTES_PER_KILOBIT;
    } else if (rateName == "jointDefaultPoseFlagsRate") {
        return _jointDefaultPoseFlagsRate.rate() / BYTES_PER_KILOBIT;
    } else if (rateName == "farGrabJointRate") {
        return _farGrabJointRate.rate() / BYTES_PER_KILOBIT;
    } else if (rateName == "globalPositionOutbound") {
        return _outboundDataRate.globalPositionRate.rate() / BYTES_PER_KILOBIT;
    } else if (rateName == "localPositionOutbound") {
        return _outboundDataRate.localPositionRate.rate() / BYTES_PER_KILOBIT;
    } else if (rateName == "avatarBoundingBoxOutbound") {
        return _outboundDataRate.avatarBoundingBoxRate.rate() / BYTES_PER_KILOBIT;
    } else if (rateName == "avatarOrientationOutbound") {
        return _outboundDataRate.avatarOrientationRate.rate() / BYTES_PER_KILOBIT;
    } else if (rateName == "avatarScaleOutbound") {
        return _outboundDataRate.avatarScaleRate.rate() / BYTES_PER_KILOBIT;
    } else if (rateName == "lookAtPositionOutbound") {
        return _outboundDataRate.lookAtPositionRate.rate() / BYTES_PER_KILOBIT;
    } else if (rateName == "audioLoudnessOutbound") {
        return _outboundDataRate.audioLoudnessRate.rate() / BYTES_PER_KILOBIT;
    } else if (rateName == "sensorToWorkMatrixOutbound") {
        return _outboundDataRate.sensorToWorldRate.rate() / BYTES_PER_KILOBIT;
    } else if (rateName == "additionalFlagsOutbound") {
        return _outboundDataRate.additionalFlagsRate.rate() / BYTES_PER_KILOBIT;
    } else if (rateName == "parentInfoOutbound") {
        return _outboundDataRate.parentInfoRate.rate() / BYTES_PER_KILOBIT;
    } else if (rateName == "faceTrackerOutbound") {
        return _outboundDataRate.faceTrackerRate.rate() / BYTES_PER_KILOBIT;
    } else if (rateName == "jointDataOutbound") {
        return _outboundDataRate.jointDataRate.rate() / BYTES_PER_KILOBIT;
    } else if (rateName == "jointDefaultPoseFlagsOutbound") {
        return _outboundDataRate.jointDefaultPoseFlagsRate.rate() / BYTES_PER_KILOBIT;
    }
    return 0.0f;
}

float AvatarData::getUpdateRate(const QString& rateName) const {
    if (rateName == "") {
        return _parseBufferUpdateRate.rate();
    } else if (rateName == "globalPosition") {
        return _globalPositionUpdateRate.rate();
    } else if (rateName == "localPosition") {
        return _localPositionUpdateRate.rate();
    } else if (rateName == "avatarBoundingBox") {
        return _avatarBoundingBoxUpdateRate.rate();
    } else if (rateName == "avatarOrientation") {
        return _avatarOrientationUpdateRate.rate();
    } else if (rateName == "avatarScale") {
        return _avatarScaleUpdateRate.rate();
    } else if (rateName == "lookAtPosition") {
        return _lookAtPositionUpdateRate.rate();
    } else if (rateName == "audioLoudness") {
        return _audioLoudnessUpdateRate.rate();
    } else if (rateName == "sensorToWorkMatrix") {
        return _sensorToWorldUpdateRate.rate();
    } else if (rateName == "additionalFlags") {
        return _additionalFlagsUpdateRate.rate();
    } else if (rateName == "parentInfo") {
        return _parentInfoUpdateRate.rate();
    } else if (rateName == "faceTracker") {
        return _faceTrackerUpdateRate.rate();
    } else if (rateName == "jointData") {
        return _jointDataUpdateRate.rate();
    } else if (rateName == "farGrabJointData") {
        return _farGrabJointUpdateRate.rate();
    }
    return 0.0f;
}

int AvatarData::getAverageBytesReceivedPerSecond() const {
    return lrint(_averageBytesReceived.getAverageSampleValuePerSecond());
}

int AvatarData::getReceiveRate() const {
    return lrint(1.0f / _averageBytesReceived.getEventDeltaAverage());
}

std::shared_ptr<Transform> AvatarData::getRecordingBasis() const {
    return _recordingBasis;
}

void AvatarData::setRawJointData(QVector<JointData> data) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "setRawJointData", Q_ARG(QVector<JointData>, data));
        return;
    }
    QWriteLocker writeLock(&_jointDataLock);
    _jointData = data;
}

void AvatarData::setJointData(int index, const glm::quat& rotation, const glm::vec3& translation) {
    if (index < 0 || index >= LOWEST_PSEUDO_JOINT_INDEX) {
        return;
    }
    QWriteLocker writeLock(&_jointDataLock);
    if (_jointData.size() <= index) {
        _jointData.resize(index + 1);
    }
    JointData& data = _jointData[index];
    data.rotation = rotation;
    data.rotationIsDefaultPose = false;
    data.translation = translation;
    data.translationIsDefaultPose = false;
}

void AvatarData::clearJointData(int index) {
    if (index < 0 || index >= LOWEST_PSEUDO_JOINT_INDEX) {
        return;
    }
    QWriteLocker writeLock(&_jointDataLock);
    // FIXME: I don't understand how this "clears" the joint data at index
    if (_jointData.size() <= index) {
        _jointData.resize(index + 1);
    }
    _jointData[index] = {};
}

bool AvatarData::isJointDataValid(int index) const {
    switch (index) {
        case FARGRAB_RIGHTHAND_INDEX: {
            bool valid;
            _farGrabRightMatrixCache.get(valid);
            return valid;
        }
        case FARGRAB_LEFTHAND_INDEX: {
            bool valid;
            _farGrabLeftMatrixCache.get(valid);
            return valid;
        }
        case FARGRAB_MOUSE_INDEX: {
            bool valid;
            _farGrabMouseMatrixCache.get(valid);
            return valid;
        }
        default: {
            if (index < 0 || index >= LOWEST_PSEUDO_JOINT_INDEX) {
                return false;
            }
            QReadLocker readLock(&_jointDataLock);
            return index < _jointData.size();
        }
    }
}

glm::quat AvatarData::getJointRotation(int index) const {
    switch (index) {
        case FARGRAB_RIGHTHAND_INDEX: {
            return extractRotation(_farGrabRightMatrixCache.get());
        }
        case FARGRAB_LEFTHAND_INDEX: {
            return extractRotation(_farGrabLeftMatrixCache.get());
        }
        case FARGRAB_MOUSE_INDEX: {
            return extractRotation(_farGrabMouseMatrixCache.get());
        }
        default: {
            if (index < 0 || index >= LOWEST_PSEUDO_JOINT_INDEX) {
                return glm::quat();
            }
            QReadLocker readLock(&_jointDataLock);
            return index < _jointData.size() ? _jointData.at(index).rotation : glm::quat();
        }
    }
}

glm::vec3 AvatarData::getJointTranslation(int index) const {
    switch (index) {
        case FARGRAB_RIGHTHAND_INDEX: {
            return extractTranslation(_farGrabRightMatrixCache.get());
        }
        case FARGRAB_LEFTHAND_INDEX: {
            return extractTranslation(_farGrabLeftMatrixCache.get());
        }
        case FARGRAB_MOUSE_INDEX: {
            return extractTranslation(_farGrabMouseMatrixCache.get());
        }
        default: {
            if (index < 0 || index >= LOWEST_PSEUDO_JOINT_INDEX) {
                return glm::vec3();
            }
            QReadLocker readLock(&_jointDataLock);
            return index < _jointData.size() ? _jointData.at(index).translation : glm::vec3();
        }
    }
}

glm::vec3 AvatarData::getJointTranslation(const QString& name) const {
    // Can't do this, because the lock needs to cover the entire set of logic.  In theory, the joints could change 
    // on another thread in between the call to getJointIndex and getJointTranslation
    // return getJointTranslation(getJointIndex(name));
    return readLockWithNamedJointIndex<glm::vec3>(name, [this](int index) {
        return _jointData.at(index).translation;
        return getJointTranslation(index);
    });
}

void AvatarData::setJointData(const QString& name, const glm::quat& rotation, const glm::vec3& translation) {
    // Can't do this, not thread safe
    // setJointData(getJointIndex(name), rotation, translation);

    writeLockWithNamedJointIndex(name, [&](int index) {
        auto& jointData = _jointData[index];
        jointData.rotation = rotation;
        jointData.translation = translation;
        jointData.rotationIsDefaultPose = false;
        jointData.translationIsDefaultPose = false;
    });
}

void AvatarData::setJointRotation(const QString& name, const glm::quat& rotation) {
    // Can't do this, not thread safe
    // setJointRotation(getJointIndex(name), rotation);
    writeLockWithNamedJointIndex(name, [&](int index) {
        auto& data = _jointData[index];
        data.rotation = rotation;
        data.rotationIsDefaultPose = false;
    });
}

void AvatarData::setJointTranslation(const QString& name, const glm::vec3& translation) {
    // Can't do this, not thread safe
    // setJointTranslation(getJointIndex(name), translation);
    writeLockWithNamedJointIndex(name, [&](int index) {
        auto& data = _jointData[index];
        data.translation = translation;
        data.translationIsDefaultPose = false;
    });
}

void AvatarData::setJointRotation(int index, const glm::quat& rotation) {
    if (index < 0 || index >= LOWEST_PSEUDO_JOINT_INDEX) {
        return;
    }
    QWriteLocker writeLock(&_jointDataLock);
    if (_jointData.size() <= index) {
        _jointData.resize(index + 1);
    }
    JointData& data = _jointData[index];
    data.rotation = rotation;
    data.rotationIsDefaultPose = false;
}

void AvatarData::setJointTranslation(int index, const glm::vec3& translation) {
    if (index < 0 || index >= LOWEST_PSEUDO_JOINT_INDEX) {
        return;
    }
    QWriteLocker writeLock(&_jointDataLock);
    if (_jointData.size() <= index) {
        _jointData.resize(index + 1);
    }
    JointData& data = _jointData[index];
    data.translation = translation;
    data.translationIsDefaultPose = false;
}

void AvatarData::clearJointData(const QString& name) {
    // Can't do this, not thread safe
    // clearJointData(getJointIndex(name));
    writeLockWithNamedJointIndex(name, [&](int index) {
        _jointData[index] = {};
    });
}

bool AvatarData::isJointDataValid(const QString& name) const {
    // Can't do this, not thread safe
    // return isJointDataValid(getJointIndex(name));

    return readLockWithNamedJointIndex<bool>(name, false, [&](int index) {
        // This is technically superfluous....  the lambda is only called if index is a valid 
        // offset for _jointData.  Nevertheless, it would be confusing to leave the lamdba as
        // `return true`
        return index < _jointData.size();
    });
}

glm::quat AvatarData::getJointRotation(const QString& name) const {
    // Can't do this, not thread safe
    // return getJointRotation(getJointIndex(name));

    return readLockWithNamedJointIndex<glm::quat>(name, [&](int index) {
        return _jointData.at(index).rotation;
    });
}

QVector<glm::quat> AvatarData::getJointRotations() const {
    if (QThread::currentThread() != thread()) {
        QVector<glm::quat> result;
        BLOCKING_INVOKE_METHOD(const_cast<AvatarData*>(this), "getJointRotations",
                                  Q_RETURN_ARG(QVector<glm::quat>, result));
        return result;
    }
    QReadLocker readLock(&_jointDataLock);
    QVector<glm::quat> jointRotations(_jointData.size());
    for (int i = 0; i < _jointData.size(); ++i) {
        jointRotations[i] = _jointData[i].rotation;
    }
    return jointRotations;
}

void AvatarData::setJointRotations(const QVector<glm::quat>& jointRotations) {
    QWriteLocker writeLock(&_jointDataLock);
    auto size = jointRotations.size();
    if (_jointData.size() < size) {
        _jointData.resize(size);
    }
    for (int i = 0; i < size; ++i) {
        auto& data = _jointData[i];
        data.rotation = jointRotations[i];
        data.rotationIsDefaultPose = false;
    }
}

QVector<glm::vec3> AvatarData::getJointTranslations() const {
    QReadLocker readLock(&_jointDataLock);
    QVector<glm::vec3> jointTranslations(_jointData.size());
    for (int i = 0; i < _jointData.size(); ++i) {
        jointTranslations[i] = _jointData[i].translation;
    }
    return jointTranslations;
}

void AvatarData::setJointTranslations(const QVector<glm::vec3>& jointTranslations) {
    QWriteLocker writeLock(&_jointDataLock);
    auto size = jointTranslations.size();
    if (_jointData.size() < size) {
        _jointData.resize(size);
    }
    for (int i = 0; i < size; ++i) {
        auto& data = _jointData[i];
        data.translation = jointTranslations[i];
        data.translationIsDefaultPose = false;
    }
}

void AvatarData::clearJointsData() {
    QWriteLocker writeLock(&_jointDataLock);
    QVector<JointData> newJointData;
    newJointData.resize(_jointData.size());
    _jointData.swap(newJointData);
}

int AvatarData::getFauxJointIndex(const QString& name) const {
    if (name == "_SENSOR_TO_WORLD_MATRIX") {
        return SENSOR_TO_WORLD_MATRIX_INDEX;
    }
    if (name == "_CONTROLLER_LEFTHAND") {
        return CONTROLLER_LEFTHAND_INDEX;
    }
    if (name == "_CONTROLLER_RIGHTHAND") {
        return CONTROLLER_RIGHTHAND_INDEX;
    }
    if (name == "_CAMERA_RELATIVE_CONTROLLER_LEFTHAND") {
        return CAMERA_RELATIVE_CONTROLLER_LEFTHAND_INDEX;
    }
    if (name == "_CAMERA_RELATIVE_CONTROLLER_RIGHTHAND") {
        return CAMERA_RELATIVE_CONTROLLER_RIGHTHAND_INDEX;
    }
    if (name == "_CAMERA_MATRIX") {
        return CAMERA_MATRIX_INDEX;
    }
    if (name == "_FARGRAB_RIGHTHAND") {
        return FARGRAB_RIGHTHAND_INDEX;
    }
    if (name == "_FARGRAB_LEFTHAND") {
        return FARGRAB_LEFTHAND_INDEX;
    }
    if (name == "_FARGRAB_MOUSE") {
        return FARGRAB_MOUSE_INDEX;
    }
    return -1;
}

int AvatarData::getJointIndex(const QString& name) const {
    int result = getFauxJointIndex(name);
    if (result != -1) {
        return result;
    }
    QReadLocker readLock(&_jointDataLock);
    return _fstJointIndices.value(name) - 1;
}

QStringList AvatarData::getJointNames() const {
    QReadLocker readLock(&_jointDataLock);
    return _fstJointNames;
}

glm::quat AvatarData::getOrientationOutbound() const {
    return (getLocalOrientation());
}

void AvatarData::processAvatarIdentity(const QByteArray& identityData, bool& identityChanged,
                                       bool& displayNameChanged) {

    QDataStream packetStream(identityData);

    QUuid avatarSessionID;

    // peek the sequence number, this will tell us if we should be processing this identity packet at all
    udt::SequenceNumber::Type incomingSequenceNumberType;
    packetStream >> avatarSessionID >> incomingSequenceNumberType;
    udt::SequenceNumber incomingSequenceNumber(incomingSequenceNumberType);

    if (!_hasProcessedFirstIdentity) {
        _identitySequenceNumber = incomingSequenceNumber - 1;
        _hasProcessedFirstIdentity = true;
        qCDebug(avatars) << "Processing first identity packet for" << avatarSessionID << "-"
            << (udt::SequenceNumber::Type) incomingSequenceNumber;
    }

    if (incomingSequenceNumber > _identitySequenceNumber) {
        Identity identity;

        packetStream
            >> identity.attachmentData
            >> identity.displayName
            >> identity.sessionDisplayName
            >> identity.isReplicated
            >> identity.lookAtSnappingEnabled
        ;

        // set the store identity sequence number to match the incoming identity
        _identitySequenceNumber = incomingSequenceNumber;

        if (identity.displayName != _displayName) {
            _displayName = identity.displayName;
            identityChanged = true;
            displayNameChanged = true;
        }
        maybeUpdateSessionDisplayNameFromTransport(identity.sessionDisplayName);

        if (identity.isReplicated != _isReplicated) {
            _isReplicated = identity.isReplicated;
            identityChanged = true;
        }

        if (identity.attachmentData != _attachmentData) {
            setAttachmentData(identity.attachmentData);
            identityChanged = true;
        }

    if (identity.lookAtSnappingEnabled != _lookAtSnappingEnabled) {
        setProperty("lookAtSnappingEnabled", identity.lookAtSnappingEnabled);
        identityChanged = true;
    }

#ifdef WANT_DEBUG
        qCDebug(avatars) << __FUNCTION__
            << "identity.uuid:" << identity.uuid
            << "identity.displayName:" << identity.displayName
            << "identity.sessionDisplayName:" << identity.sessionDisplayName;
    } else {

        qCDebug(avatars) << "Refusing to process identity for" << uuidStringWithoutCurlyBraces(avatarSessionID) << "since"
            << (udt::SequenceNumber::Type) _identitySequenceNumber
            << "is >=" << (udt::SequenceNumber::Type) incomingSequenceNumber;
#endif
    }
}

QUrl AvatarData::getWireSafeSkeletonModelURL() const {
    if (_skeletonModelURL.scheme() != "file" && _skeletonModelURL.scheme() != "qrc") {
        return _skeletonModelURL;
    } else {
        return QUrl();
    }
}

qint64 AvatarData::packTrait(AvatarTraits::TraitType traitType, ExtendedIODevice& destination,
                           AvatarTraits::TraitVersion traitVersion) {
    qint64 bytesWritten = 0;
    bytesWritten += destination.writePrimitive(traitType);

    if (traitVersion > AvatarTraits::DEFAULT_TRAIT_VERSION) {
        bytesWritten += destination.writePrimitive(traitVersion);
    }

    if (traitType == AvatarTraits::SkeletonModelURL) {
        QByteArray encodedSkeletonURL = getWireSafeSkeletonModelURL().toEncoded();
        
        AvatarTraits::TraitWireSize encodedURLSize = encodedSkeletonURL.size();
        bytesWritten += destination.writePrimitive(encodedURLSize);

        bytesWritten += destination.write(encodedSkeletonURL);
    }

    return bytesWritten;
}

qint64 AvatarData::packTraitInstance(AvatarTraits::TraitType traitType, AvatarTraits::TraitInstanceID traitInstanceID,
                                   ExtendedIODevice& destination, AvatarTraits::TraitVersion traitVersion) {
    qint64 bytesWritten = 0;

    bytesWritten += destination.writePrimitive(traitType);

    if (traitVersion > AvatarTraits::DEFAULT_TRAIT_VERSION) {
        bytesWritten += destination.writePrimitive(traitVersion);
    }

    bytesWritten += destination.write(traitInstanceID.toRfc4122());

    if (traitType == AvatarTraits::AvatarEntity) {
        // grab a read lock on the avatar entities and check for entity data for the given ID
        QByteArray entityBinaryData;

        _avatarEntitiesLock.withReadLock([this, &entityBinaryData, &traitInstanceID] {
            if (_avatarEntityData.contains(traitInstanceID)) {
                entityBinaryData = _avatarEntityData[traitInstanceID];
            }
        });

        if (!entityBinaryData.isNull()) {
            AvatarTraits::TraitWireSize entityBinarySize = entityBinaryData.size();

            bytesWritten += destination.writePrimitive(entityBinarySize);
            bytesWritten += destination.write(entityBinaryData);
        } else {
            bytesWritten += destination.writePrimitive(AvatarTraits::DELETED_TRAIT_SIZE);
        }
    }

    return bytesWritten;
}

void AvatarData::prepareResetTraitInstances() {
    if (_clientTraitsHandler) {
        _avatarEntitiesLock.withReadLock([this]{
            foreach (auto entityID, _avatarEntityData.keys()) {
                _clientTraitsHandler->markInstancedTraitUpdated(AvatarTraits::AvatarEntity, entityID);
            }
        });
    }
}

void AvatarData::processTrait(AvatarTraits::TraitType traitType, QByteArray traitBinaryData) {
    if (traitType == AvatarTraits::SkeletonModelURL) {
        // get the URL from the binary data
        auto skeletonModelURL = QUrl::fromEncoded(traitBinaryData);
        setSkeletonModelURL(skeletonModelURL);
    }
}

void AvatarData::processTraitInstance(AvatarTraits::TraitType traitType,
                                      AvatarTraits::TraitInstanceID instanceID, QByteArray traitBinaryData) {
    if (traitType == AvatarTraits::AvatarEntity) {
        updateAvatarEntity(instanceID, traitBinaryData);
    }
}

void AvatarData::processDeletedTraitInstance(AvatarTraits::TraitType traitType, AvatarTraits::TraitInstanceID instanceID) {
    if (traitType == AvatarTraits::AvatarEntity) {
        clearAvatarEntity(instanceID);
    }
}

QByteArray AvatarData::identityByteArray(bool setIsReplicated) const {
    QByteArray identityData;
    QDataStream identityStream(&identityData, QIODevice::Append);

    // when mixers send identity packets to agents, they simply forward along the last incoming sequence number they received
    // whereas agents send a fresh outgoing sequence number when identity data has changed

    identityStream << getSessionUUID()
        << (udt::SequenceNumber::Type) _identitySequenceNumber
        << _attachmentData
        << _displayName
        << getSessionDisplayNameForTransport() // depends on _sessionDisplayName
        << (_isReplicated || setIsReplicated)
        << _lookAtSnappingEnabled;

    return identityData;
}

void AvatarData::setSkeletonModelURL(const QUrl& skeletonModelURL) {
    if (skeletonModelURL.isEmpty()) {
        qCDebug(avatars) << __FUNCTION__ << "caller called with empty URL.";
    }

    const QUrl& expanded = skeletonModelURL.isEmpty() ? AvatarData::defaultFullAvatarModelUrl() : skeletonModelURL;
    if (expanded == _skeletonModelURL) {
        return;
    }
    
    _skeletonModelURL = expanded;
    qCDebug(avatars) << "Changing skeleton model for avatar" << getSessionUUID() << "to" << _skeletonModelURL.toString();

    updateJointMappings();

    if (_clientTraitsHandler) {
        _clientTraitsHandler->markTraitUpdated(AvatarTraits::SkeletonModelURL);
    }

    emit skeletonModelURLChanged();
}

void AvatarData::setDisplayName(const QString& displayName) {
    _displayName = displayName;
    _sessionDisplayName = "";

    qCDebug(avatars) << "Changing display name for avatar to" << displayName;
    markIdentityDataChanged();
}

QVector<AttachmentData> AvatarData::getAttachmentData() const {
    if (QThread::currentThread() != thread()) {
        QVector<AttachmentData> result;
        BLOCKING_INVOKE_METHOD(const_cast<AvatarData*>(this), "getAttachmentData",
            Q_RETURN_ARG(QVector<AttachmentData>, result));
        return result;
    }
    return _attachmentData;
}

void AvatarData::setAttachmentData(const QVector<AttachmentData>& attachmentData) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "setAttachmentData", Q_ARG(const QVector<AttachmentData>&, attachmentData));
        return;
    }
    _attachmentData = attachmentData;
    markIdentityDataChanged();
}

void AvatarData::attach(const QString& modelURL, const QString& jointName,
                        const glm::vec3& translation, const glm::quat& rotation,
                        float scale, bool isSoft,
                        bool allowDuplicates, bool useSaved) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "attach", Q_ARG(const QString&, modelURL), Q_ARG(const QString&, jointName),
                                  Q_ARG(const glm::vec3&, translation), Q_ARG(const glm::quat&, rotation),
                                  Q_ARG(float, scale), Q_ARG(bool, isSoft),
                                  Q_ARG(bool, allowDuplicates), Q_ARG(bool, useSaved));
        return;
    }
    QVector<AttachmentData> attachmentData = getAttachmentData();
    if (!allowDuplicates) {
        foreach (const AttachmentData& data, attachmentData) {
            if (data.modelURL == modelURL && (jointName.isEmpty() || data.jointName == jointName)) {
                return;
            }
        }
    }
    AttachmentData data;
    data.modelURL = modelURL;
    data.jointName = jointName;
    data.translation = translation;
    data.rotation = rotation;
    data.scale = scale;
    data.isSoft = isSoft;
    attachmentData.append(data);
    setAttachmentData(attachmentData);
}

void AvatarData::detachOne(const QString& modelURL, const QString& jointName) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "detachOne", Q_ARG(const QString&, modelURL), Q_ARG(const QString&, jointName));
        return;
    }
    QVector<AttachmentData> attachmentData = getAttachmentData();
    for (QVector<AttachmentData>::iterator it = attachmentData.begin(); it != attachmentData.end(); ++it) {
        if (it->modelURL == modelURL && (jointName.isEmpty() || it->jointName == jointName)) {
            attachmentData.erase(it);
            setAttachmentData(attachmentData);
            return;
        }
    }
}

void AvatarData::detachAll(const QString& modelURL, const QString& jointName) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "detachAll", Q_ARG(const QString&, modelURL), Q_ARG(const QString&, jointName));
        return;
    }
    QVector<AttachmentData> attachmentData = getAttachmentData();
    for (QVector<AttachmentData>::iterator it = attachmentData.begin(); it != attachmentData.end(); ) {
        if (it->modelURL == modelURL && (jointName.isEmpty() || it->jointName == jointName)) {
            it = attachmentData.erase(it);
        } else {
            ++it;
        }
    }
    setAttachmentData(attachmentData);
}

void AvatarData::setJointMappingsFromNetworkReply() {

    QNetworkReply* networkReply = static_cast<QNetworkReply*>(sender());

    // before we process this update, make sure that the skeleton model URL hasn't changed
    // since we made the FST request
    if (networkReply->url() != _skeletonModelURL) {
        qCDebug(avatars) << "Refusing to set joint mappings for FST URL that does not match the current URL";
        return;
    }

    {
        QWriteLocker writeLock(&_jointDataLock);
        QByteArray line;
        while (!(line = networkReply->readLine()).isEmpty()) {
            line = line.trimmed();
            if (line.startsWith("filename")) {
                int filenameIndex = line.indexOf('=') + 1;
                    if (filenameIndex > 0) {
                        _skeletonFBXURL = _skeletonModelURL.resolved(QString(line.mid(filenameIndex).trimmed()));
                    }
                }
            if (!line.startsWith("jointIndex")) {
                continue;
            }
            int jointNameIndex = line.indexOf('=') + 1;
            if (jointNameIndex == 0) {
                continue;
            }
            int secondSeparatorIndex = line.indexOf('=', jointNameIndex);
            if (secondSeparatorIndex == -1) {
                continue;
            }
            QString jointName = line.mid(jointNameIndex, secondSeparatorIndex - jointNameIndex).trimmed();
            bool ok;
            int jointIndex = line.mid(secondSeparatorIndex + 1).trimmed().toInt(&ok);
            if (ok) {
                while (_fstJointNames.size() < jointIndex + 1) {
                    _fstJointNames.append(QString());
                }
                _fstJointNames[jointIndex] = jointName;
            }
        }
        for (int i = 0; i < _fstJointNames.size(); i++) {
            _fstJointIndices.insert(_fstJointNames.at(i), i + 1);
        }
    }

    networkReply->deleteLater();
}

void AvatarData::sendAvatarDataPacket(bool sendAll) {
    auto nodeList = DependencyManager::get<NodeList>();

    // about 2% of the time, we send a full update (meaning, we transmit all the joint data), even if nothing has changed.
    // this is to guard against a joint moving once, the packet getting lost, and the joint never moving again.

    bool cullSmallData = !sendAll && (randFloat() < AVATAR_SEND_FULL_UPDATE_RATIO);
    auto dataDetail = cullSmallData ? SendAllData : CullSmallData;
    QByteArray avatarByteArray = toByteArrayStateful(dataDetail);

    int maximumByteArraySize = NLPacket::maxPayloadSize(PacketType::AvatarData) - sizeof(AvatarDataSequenceNumber);

    if (avatarByteArray.size() > maximumByteArraySize) {
        qCWarning(avatars) << "toByteArrayStateful() resulted in very large buffer:" << avatarByteArray.size() << "... attempt to drop facial data";
        avatarByteArray = toByteArrayStateful(dataDetail, true);

        if (avatarByteArray.size() > maximumByteArraySize) {
            qCWarning(avatars) << "toByteArrayStateful() without facial data resulted in very large buffer:" << avatarByteArray.size() << "... reduce to MinimumData";
            avatarByteArray = toByteArrayStateful(MinimumData, true);

            if (avatarByteArray.size() > maximumByteArraySize) {
                qCWarning(avatars) << "toByteArrayStateful() MinimumData resulted in very large buffer:" << avatarByteArray.size() << "... FAIL!!";
                return;
            }
        }
    }

    doneEncoding(cullSmallData);

    static AvatarDataSequenceNumber sequenceNumber = 0;

    auto avatarPacket = NLPacket::create(PacketType::AvatarData, avatarByteArray.size() + sizeof(sequenceNumber));
    avatarPacket->writePrimitive(sequenceNumber++);
    avatarPacket->write(avatarByteArray);

    nodeList->broadcastToNodes(std::move(avatarPacket), NodeSet() << NodeType::AvatarMixer);
}

void AvatarData::sendIdentityPacket() {
    auto nodeList = DependencyManager::get<NodeList>();

    if (_identityDataChanged) {
        // if the identity data has changed, push the sequence number forwards
        ++_identitySequenceNumber;
    }

    QByteArray identityData = identityByteArray();

    auto packetList = NLPacketList::create(PacketType::AvatarIdentity, QByteArray(), true, true);
    packetList->write(identityData);
    nodeList->eachMatchingNode(
        [](const SharedNodePointer& node)->bool {
            return node->getType() == NodeType::AvatarMixer && node->getActiveSocket();
        },
        [&](const SharedNodePointer& node) {
            nodeList->sendPacketList(std::move(packetList), *node);
    });

    _identityDataChanged = false;
}

void AvatarData::updateJointMappings() {
    {
        QWriteLocker writeLock(&_jointDataLock);
        _fstJointIndices.clear();
        _fstJointNames.clear();
        _jointData.clear();
    }

    if (_skeletonModelURL.fileName().toLower().endsWith(".fst")) {
        QNetworkAccessManager& networkAccessManager = NetworkAccessManager::getInstance();
        QNetworkRequest networkRequest = QNetworkRequest(_skeletonModelURL);
        networkRequest.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
        networkRequest.setHeader(QNetworkRequest::UserAgentHeader, HIGH_FIDELITY_USER_AGENT);
        QNetworkReply* networkReply = networkAccessManager.get(networkRequest);
        connect(networkReply, &QNetworkReply::finished, this, &AvatarData::setJointMappingsFromNetworkReply);
    }
}

static const QString JSON_ATTACHMENT_URL = QStringLiteral("modelUrl");
static const QString JSON_ATTACHMENT_JOINT_NAME = QStringLiteral("jointName");
static const QString JSON_ATTACHMENT_TRANSFORM = QStringLiteral("transform");
static const QString JSON_ATTACHMENT_IS_SOFT = QStringLiteral("isSoft");

QJsonObject AttachmentData::toJson() const {
    QJsonObject result;
    if (modelURL.isValid() && !modelURL.isEmpty()) {
        result[JSON_ATTACHMENT_URL] = modelURL.toString();
    }
    if (!jointName.isEmpty()) {
        result[JSON_ATTACHMENT_JOINT_NAME] = jointName;
    }
    // FIXME the transform constructor that takes rot/scale/translation
    // doesn't return the correct value for isIdentity()
    Transform transform;
    transform.setRotation(rotation);
    transform.setScale(scale);
    transform.setTranslation(translation);
    if (!transform.isIdentity()) {
        result[JSON_ATTACHMENT_TRANSFORM] = Transform::toJson(transform);
    }
    result[JSON_ATTACHMENT_IS_SOFT] = isSoft;
    return result;
}

void AttachmentData::fromJson(const QJsonObject& json) {
    if (json.contains(JSON_ATTACHMENT_URL)) {
        const QString modelURLTemp = json[JSON_ATTACHMENT_URL].toString();
        if (modelURLTemp != modelURL.toString()) {
            modelURL = modelURLTemp;
        }
    }

    if (json.contains(JSON_ATTACHMENT_JOINT_NAME)) {
        const QString jointNameTemp = json[JSON_ATTACHMENT_JOINT_NAME].toString();
        if (jointNameTemp != jointName) {
            jointName = jointNameTemp;
        }
    }

    if (json.contains(JSON_ATTACHMENT_TRANSFORM)) {
        Transform transform = Transform::fromJson(json[JSON_ATTACHMENT_TRANSFORM]);
        translation = transform.getTranslation();
        rotation = transform.getRotation();
        scale = transform.getScale().x;
    }

    if (json.contains(JSON_ATTACHMENT_IS_SOFT)) {
        isSoft = json[JSON_ATTACHMENT_IS_SOFT].toBool();
    }
}

bool AttachmentData::operator==(const AttachmentData& other) const {
    return modelURL == other.modelURL && jointName == other.jointName && translation == other.translation &&
        rotation == other.rotation && scale == other.scale && isSoft == other.isSoft;
}

QDataStream& operator<<(QDataStream& out, const AttachmentData& attachment) {
    return out << attachment.modelURL << attachment.jointName <<
        attachment.translation << attachment.rotation << attachment.scale << attachment.isSoft;
}

QDataStream& operator>>(QDataStream& in, AttachmentData& attachment) {
    return in >> attachment.modelURL >> attachment.jointName >>
        attachment.translation >> attachment.rotation >> attachment.scale >> attachment.isSoft;
}

void AttachmentDataObject::setModelURL(const QString& modelURL) {
    AttachmentData data = qscriptvalue_cast<AttachmentData>(thisObject());
    data.modelURL = modelURL;
    thisObject() = engine()->toScriptValue(data);
}

QString AttachmentDataObject::getModelURL() const {
    return qscriptvalue_cast<AttachmentData>(thisObject()).modelURL.toString();
}

void AttachmentDataObject::setJointName(const QString& jointName) {
    AttachmentData data = qscriptvalue_cast<AttachmentData>(thisObject());
    data.jointName = jointName;
    thisObject() = engine()->toScriptValue(data);
}

QString AttachmentDataObject::getJointName() const {
    return qscriptvalue_cast<AttachmentData>(thisObject()).jointName;
}

void AttachmentDataObject::setTranslation(const glm::vec3& translation) {
    AttachmentData data = qscriptvalue_cast<AttachmentData>(thisObject());
    data.translation = translation;
    thisObject() = engine()->toScriptValue(data);
}

glm::vec3 AttachmentDataObject::getTranslation() const {
    return qscriptvalue_cast<AttachmentData>(thisObject()).translation;
}

void AttachmentDataObject::setRotation(const glm::quat& rotation) {
    AttachmentData data = qscriptvalue_cast<AttachmentData>(thisObject());
    data.rotation = rotation;
    thisObject() = engine()->toScriptValue(data);
}

glm::quat AttachmentDataObject::getRotation() const {
    return qscriptvalue_cast<AttachmentData>(thisObject()).rotation;
}

void AttachmentDataObject::setScale(float scale) {
    AttachmentData data = qscriptvalue_cast<AttachmentData>(thisObject());
    data.scale = scale;
    thisObject() = engine()->toScriptValue(data);
}

float AttachmentDataObject::getScale() const {
    return qscriptvalue_cast<AttachmentData>(thisObject()).scale;
}

void AttachmentDataObject::setIsSoft(bool isSoft) {
    AttachmentData data = qscriptvalue_cast<AttachmentData>(thisObject());
    data.isSoft = isSoft;
    thisObject() = engine()->toScriptValue(data);
}

bool AttachmentDataObject::getIsSoft() const {
    return qscriptvalue_cast<AttachmentData>(thisObject()).isSoft;
}

void registerAvatarTypes(QScriptEngine* engine) {
    qScriptRegisterSequenceMetaType<QVector<AttachmentData> >(engine);
    engine->setDefaultPrototype(qMetaTypeId<AttachmentData>(), engine->newQObject(
        new AttachmentDataObject(), QScriptEngine::ScriptOwnership));
}

void AvatarData::setRecordingBasis(std::shared_ptr<Transform> recordingBasis) {
    if (!recordingBasis) {
        recordingBasis = std::make_shared<Transform>();
        recordingBasis->setRotation(getWorldOrientation());
        recordingBasis->setTranslation(getWorldPosition());
        // TODO: find a  different way to record/playback the Scale of the avatar
        //recordingBasis->setScale(getTargetScale());
    }
    _recordingBasis = recordingBasis;
}

void AvatarData::createRecordingIDs() {
    _avatarEntitiesLock.withReadLock([&] {
        _avatarEntityForRecording.clear();
        for (int i = 0; i < _avatarEntityData.size(); i++) {
            _avatarEntityForRecording.insert(QUuid::createUuid());
        }
    });
}

void AvatarData::clearRecordingBasis() {
    _recordingBasis.reset();
}

static const QString JSON_AVATAR_BASIS = QStringLiteral("basisTransform");
static const QString JSON_AVATAR_RELATIVE = QStringLiteral("relativeTransform");
static const QString JSON_AVATAR_JOINT_ARRAY = QStringLiteral("jointArray");
static const QString JSON_AVATAR_HEAD = QStringLiteral("head");
static const QString JSON_AVATAR_HEAD_MODEL = QStringLiteral("headModel");
static const QString JSON_AVATAR_BODY_MODEL = QStringLiteral("bodyModel");
static const QString JSON_AVATAR_DISPLAY_NAME = QStringLiteral("displayName");
// It isn't meaningful to persist sessionDisplayName.
static const QString JSON_AVATAR_ATTACHMENTS = QStringLiteral("attachments");
static const QString JSON_AVATAR_ENTITIES = QStringLiteral("attachedEntities");
static const QString JSON_AVATAR_SCALE = QStringLiteral("scale");
static const QString JSON_AVATAR_VERSION = QStringLiteral("version");

enum class JsonAvatarFrameVersion : int {
    JointRotationsInRelativeFrame = 0,
    JointRotationsInAbsoluteFrame,
    JointDefaultPoseBits
};

QJsonValue toJsonValue(const JointData& joint) {
    QJsonArray result;
    result.push_back(toJsonValue(joint.rotation));
    result.push_back(toJsonValue(joint.translation));
    result.push_back(QJsonValue(joint.rotationIsDefaultPose));
    result.push_back(QJsonValue(joint.translationIsDefaultPose));
    return result;
}

JointData jointDataFromJsonValue(int version, const QJsonValue& json) {
    JointData result;
    if (json.isArray()) {
        QJsonArray array = json.toArray();
        result.rotation = quatFromJsonValue(array[0]);
        result.translation = vec3FromJsonValue(array[1]);
        if (version >= (int)JsonAvatarFrameVersion::JointDefaultPoseBits) {
            result.rotationIsDefaultPose = array[2].toBool();
            result.translationIsDefaultPose = array[3].toBool();
        } else {
            result.rotationIsDefaultPose = false;
            result.translationIsDefaultPose = false;
        }
    }
    return result;
}

QJsonObject AvatarData::toJson() const {
    QJsonObject root;

    root[JSON_AVATAR_VERSION] = (int)JsonAvatarFrameVersion::JointDefaultPoseBits;

    if (!getSkeletonModelURL().isEmpty()) {
        root[JSON_AVATAR_BODY_MODEL] = getSkeletonModelURL().toString();
    }
    if (!getDisplayName().isEmpty()) {
        root[JSON_AVATAR_DISPLAY_NAME] = getDisplayName();
    }
    _avatarEntitiesLock.withReadLock([&] {
        if (!_avatarEntityData.empty()) {
            QJsonArray avatarEntityJson;
            int entityCount = 0;
            for (auto entityID : _avatarEntityData.keys()) {
                QVariantMap entityData;
                QUuid newId = _avatarEntityForRecording.size() == _avatarEntityData.size() ? _avatarEntityForRecording.values()[entityCount++] : entityID;
                entityData.insert("id", newId);                
                entityData.insert("properties", _avatarEntityData.value(entityID).toBase64());
                avatarEntityJson.push_back(QVariant(entityData).toJsonObject());
            }
            root[JSON_AVATAR_ENTITIES] = avatarEntityJson;
        }
    });

    auto recordingBasis = getRecordingBasis();
    bool success;
    Transform avatarTransform = getTransform(success);
    if (!success) {
        qCWarning(avatars) << "Warning -- AvatarData::toJson couldn't get avatar transform";
    }
    avatarTransform.setScale(getDomainLimitedScale());
    if (recordingBasis) {
        root[JSON_AVATAR_BASIS] = Transform::toJson(*recordingBasis);
        // Find the relative transform
        auto relativeTransform = recordingBasis->relativeTransform(avatarTransform);
        if (!relativeTransform.isIdentity()) {
            root[JSON_AVATAR_RELATIVE] = Transform::toJson(relativeTransform);
        }
    } else {
        root[JSON_AVATAR_RELATIVE] = Transform::toJson(avatarTransform);
    }

    auto scale = getDomainLimitedScale();
    if (scale != 1.0f) {
        root[JSON_AVATAR_SCALE] = scale;
    }

    // Skeleton pose
    QJsonArray jointArray;
    for (const auto& joint : getRawJointData()) {
        jointArray.push_back(toJsonValue(joint));
    }
    root[JSON_AVATAR_JOINT_ARRAY] = jointArray;

    const HeadData* head = getHeadData();
    if (head) {
        auto headJson = head->toJson();
        if (!headJson.isEmpty()) {
            root[JSON_AVATAR_HEAD] = headJson;
        }
    }
    return root;
}

void AvatarData::fromJson(const QJsonObject& json, bool useFrameSkeleton) {
    int version;
    if (json.contains(JSON_AVATAR_VERSION)) {
        version = json[JSON_AVATAR_VERSION].toInt();
    } else {
        // initial data did not have a version field.
        version = (int)JsonAvatarFrameVersion::JointRotationsInRelativeFrame;
    }

    if (json.contains(JSON_AVATAR_BODY_MODEL)) {
        auto bodyModelURL = json[JSON_AVATAR_BODY_MODEL].toString();
        if (useFrameSkeleton && bodyModelURL != getSkeletonModelURL().toString()) {
            setSkeletonModelURL(bodyModelURL);
        }
    }

    QString newDisplayName = "";
    if (json.contains(JSON_AVATAR_DISPLAY_NAME)) {
        newDisplayName = json[JSON_AVATAR_DISPLAY_NAME].toString();
    }
    if (newDisplayName != getDisplayName()) {
        setDisplayName(newDisplayName);
    }

    auto currentBasis = getRecordingBasis();
    if (!currentBasis) {
        currentBasis = std::make_shared<Transform>(Transform::fromJson(json[JSON_AVATAR_BASIS]));
    }

    glm::quat orientation;
    if (json.contains(JSON_AVATAR_RELATIVE)) {
        // During playback you can either have the recording basis set to the avatar current state
        // meaning that all playback is relative to this avatars starting position, or
        // the basis can be loaded from the recording, meaning the playback is relative to the
        // original avatar location
        // The first is more useful for playing back recordings on your own avatar, while
        // the latter is more useful for playing back other avatars within your scene.

        auto relativeTransform = Transform::fromJson(json[JSON_AVATAR_RELATIVE]);
        auto worldTransform = currentBasis->worldTransform(relativeTransform);
        setWorldPosition(worldTransform.getTranslation());
        orientation = worldTransform.getRotation();
    } else {
        // We still set the position in the case that there is no movement.
        setWorldPosition(currentBasis->getTranslation());
        orientation = currentBasis->getRotation();
    }
    setWorldOrientation(orientation);
    updateAttitude(orientation);

    // Do after avatar orientation because head look-at needs avatar orientation.
    if (json.contains(JSON_AVATAR_HEAD)) {
        if (!_headData) {
            _headData = new HeadData(this);
        }
        _headData->fromJson(json[JSON_AVATAR_HEAD].toObject());
    }

    if (json.contains(JSON_AVATAR_SCALE)) {
        setTargetScale((float)json[JSON_AVATAR_SCALE].toDouble());
    }

    QVector<AttachmentData> attachments;
    if (json.contains(JSON_AVATAR_ATTACHMENTS) && json[JSON_AVATAR_ATTACHMENTS].isArray()) {
        QJsonArray attachmentsJson = json[JSON_AVATAR_ATTACHMENTS].toArray();
        for (auto attachmentJson : attachmentsJson) {
            AttachmentData attachment;
            attachment.fromJson(attachmentJson.toObject());
            attachments.push_back(attachment);
        }
    }
    if (attachments != getAttachmentData()) {
        setAttachmentData(attachments);
    }

    if (json.contains(JSON_AVATAR_ENTITIES) && json[JSON_AVATAR_ENTITIES].isArray()) {
        QJsonArray attachmentsJson = json[JSON_AVATAR_ENTITIES].toArray();
        for (auto attachmentJson : attachmentsJson) {
            if (attachmentJson.isObject()) {
                QVariantMap entityData = attachmentJson.toObject().toVariantMap();
                QUuid entityID = entityData.value("id").toUuid();
                QByteArray properties = QByteArray::fromBase64(entityData.value("properties").toByteArray());
                updateAvatarEntity(entityID, properties);
            }
        }
    }

    if (json.contains(JSON_AVATAR_JOINT_ARRAY)) {
        if (version == (int)JsonAvatarFrameVersion::JointRotationsInRelativeFrame) {
            // because we don't have the full joint hierarchy skeleton of the model,
            // we can't properly convert from relative rotations into absolute rotations.
            quint64 now = usecTimestampNow();
            if (shouldLogError(now)) {
                qCWarning(avatars) << "Version 0 avatar recordings not supported. using default rotations";
            }
        } else {
            QVector<JointData> jointArray;
            QJsonArray jointArrayJson = json[JSON_AVATAR_JOINT_ARRAY].toArray();
            jointArray.reserve(jointArrayJson.size());
            for (const auto& jointJson : jointArrayJson) {
                auto joint = jointDataFromJsonValue(version, jointJson);
                jointArray.push_back(joint);
            }
            setRawJointData(jointArray);
        }
    }
}

// Every frame will store both a basis for the recording and a relative transform
// This allows the application to decide whether playback should be relative to an avatar's
// transform at the start of playback, or relative to the transform of the recorded
// avatar
QByteArray AvatarData::toFrame(const AvatarData& avatar) {
    QJsonObject root = avatar.toJson();
#ifdef WANT_JSON_DEBUG
    {
        QJsonObject obj = root;
        obj.remove(JSON_AVATAR_JOINT_ARRAY);
        qCDebug(avatars).noquote() << QJsonDocument(obj).toJson(QJsonDocument::JsonFormat::Indented);
    }
#endif
    return QJsonDocument(root).toBinaryData();
}


void AvatarData::fromFrame(const QByteArray& frameData, AvatarData& result, bool useFrameSkeleton) {
    QJsonDocument doc = QJsonDocument::fromBinaryData(frameData);

#ifdef WANT_JSON_DEBUG
    {
        QJsonObject obj = doc.object();
        obj.remove(JSON_AVATAR_JOINT_ARRAY);
        qCDebug(avatars).noquote() << QJsonDocument(obj).toJson(QJsonDocument::JsonFormat::Indented);
    }
#endif
    result.fromJson(doc.object(), useFrameSkeleton);
}

float AvatarData::getBodyYaw() const {
    glm::vec3 eulerAngles = glm::degrees(safeEulerAngles(getWorldOrientation()));
    return eulerAngles.y;
}

void AvatarData::setBodyYaw(float bodyYaw) {
    glm::vec3 eulerAngles = glm::degrees(safeEulerAngles(getWorldOrientation()));
    eulerAngles.y = bodyYaw;
    setWorldOrientation(glm::quat(glm::radians(eulerAngles)));
}

float AvatarData::getBodyPitch() const {
    glm::vec3 eulerAngles = glm::degrees(safeEulerAngles(getWorldOrientation()));
    return eulerAngles.x;
}

void AvatarData::setBodyPitch(float bodyPitch) {
    glm::vec3 eulerAngles = glm::degrees(safeEulerAngles(getWorldOrientation()));
    eulerAngles.x = bodyPitch;
    setWorldOrientation(glm::quat(glm::radians(eulerAngles)));
}

float AvatarData::getBodyRoll() const {
    glm::vec3 eulerAngles = glm::degrees(safeEulerAngles(getWorldOrientation()));
    return eulerAngles.z;
}

void AvatarData::setBodyRoll(float bodyRoll) {
    glm::vec3 eulerAngles = glm::degrees(safeEulerAngles(getWorldOrientation()));
    eulerAngles.z = bodyRoll;
    setWorldOrientation(glm::quat(glm::radians(eulerAngles)));
}

void AvatarData::setPositionViaScript(const glm::vec3& position) {
    SpatiallyNestable::setWorldPosition(position);
}

void AvatarData::setOrientationViaScript(const glm::quat& orientation) {
    SpatiallyNestable::setWorldOrientation(orientation);
}

glm::quat AvatarData::getAbsoluteJointRotationInObjectFrame(int index) const {
    assert(false);
    return glm::quat();
}

glm::vec3 AvatarData::getAbsoluteJointTranslationInObjectFrame(int index) const {
    assert(false);
    return glm::vec3();
}

/**jsdoc
 * @typedef {object} AttachmentData
 * @property {string} modelUrl
 * @property {string} jointName
 * @property {Vec3} translation
 * @property {Vec3} rotation
 * @property {number} scale
 * @property {boolean} soft
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

QVariantList AvatarData::getAttachmentsVariant() const {
    QVariantList result;
    for (const auto& attachment : getAttachmentData()) {
        result.append(attachment.toVariant());
    }
    return result;
}

void AvatarData::setAttachmentsVariant(const QVariantList& variant) {
    QVector<AttachmentData> newAttachments;
    newAttachments.reserve(variant.size());
    for (const auto& attachmentVar : variant) {
        AttachmentData attachment;
        if (attachment.fromVariant(attachmentVar)) {
            newAttachments.append(attachment);
        }
    }
    setAttachmentData(newAttachments);
}

const int MAX_NUM_AVATAR_ENTITIES = 42;

void AvatarData::updateAvatarEntity(const QUuid& entityID, const QByteArray& entityData) {
    _avatarEntitiesLock.withWriteLock([&] {
        AvatarEntityMap::iterator itr = _avatarEntityData.find(entityID);
        if (itr == _avatarEntityData.end()) {
            if (_avatarEntityData.size() < MAX_NUM_AVATAR_ENTITIES) {
                _avatarEntityData.insert(entityID, entityData);
            }
        } else {
            itr.value() = entityData;
        }
    });

    _avatarEntityDataChanged = true;

    if (_clientTraitsHandler) {
        // we have a client traits handler, so we need to mark this instanced trait as changed
        // so that changes will be sent next frame
        _clientTraitsHandler->markInstancedTraitUpdated(AvatarTraits::AvatarEntity, entityID);
    }
}

void AvatarData::clearAvatarEntity(const QUuid& entityID, bool requiresRemovalFromTree) {

    bool removedEntity = false;

    _avatarEntitiesLock.withWriteLock([this, &removedEntity, &entityID] {
        removedEntity = _avatarEntityData.remove(entityID);
    });

    insertDetachedEntityID(entityID);

    if (removedEntity && _clientTraitsHandler) {
        // we have a client traits handler, so we need to mark this removed instance trait as deleted
        // so that changes are sent next frame
        _clientTraitsHandler->markInstancedTraitDeleted(AvatarTraits::AvatarEntity, entityID);
    }
}

AvatarEntityMap AvatarData::getAvatarEntityData() const {
    AvatarEntityMap result;
    _avatarEntitiesLock.withReadLock([&] {
        result = _avatarEntityData;
    });
    return result;
}

void AvatarData::insertDetachedEntityID(const QUuid entityID) {
    _avatarEntitiesLock.withWriteLock([&] {
        _avatarEntityDetached.insert(entityID);
    });

    _avatarEntityDataChanged = true;
}

void AvatarData::setAvatarEntityData(const AvatarEntityMap& avatarEntityData) {
    if (avatarEntityData.size() > MAX_NUM_AVATAR_ENTITIES) {
        // the data is suspect
        qCDebug(avatars) << "discard suspect AvatarEntityData with size =" << avatarEntityData.size();
        return;
    }
    _avatarEntitiesLock.withWriteLock([&] {
        if (_avatarEntityData != avatarEntityData) {
            // keep track of entities that were attached to this avatar but no longer are
            AvatarEntityIDs previousAvatarEntityIDs = QSet<QUuid>::fromList(_avatarEntityData.keys());

            _avatarEntityData = avatarEntityData;
            setAvatarEntityDataChanged(true);

            foreach (auto entityID, previousAvatarEntityIDs) {
                if (!_avatarEntityData.contains(entityID)) {
                    _avatarEntityDetached.insert(entityID);

                    if (_clientTraitsHandler) {
                        // we have a client traits handler, so we flag this removed entity as deleted
                        // so that changes are sent next frame
                        _clientTraitsHandler->markInstancedTraitDeleted(AvatarTraits::AvatarEntity, entityID);
                    }
                }
            }

            if (_clientTraitsHandler) {
                // if we have a client traits handler, flag any updated or created entities
                // so that we send changes for them next frame
                foreach (auto entityID, _avatarEntityData.keys()) {
                    _clientTraitsHandler->markInstancedTraitUpdated(AvatarTraits::AvatarEntity, entityID);
                }
            }
        }
    });
}

AvatarEntityIDs AvatarData::getAndClearRecentlyDetachedIDs() {
    AvatarEntityIDs result;
    _avatarEntitiesLock.withWriteLock([&] {
        result = _avatarEntityDetached;
        _avatarEntityDetached.clear();
    });
    return result;
}

// thread-safe
glm::mat4 AvatarData::getSensorToWorldMatrix() const {
    return _sensorToWorldMatrixCache.get();
}

// thread-safe
float AvatarData::getSensorToWorldScale() const {
    return extractUniformScale(_sensorToWorldMatrixCache.get());
}

// thread-safe
glm::mat4 AvatarData::getControllerLeftHandMatrix() const {
    return _controllerLeftHandMatrixCache.get();
}

// thread-safe
glm::mat4 AvatarData::getControllerRightHandMatrix() const {
    return _controllerRightHandMatrixCache.get();
}

QScriptValue RayToAvatarIntersectionResultToScriptValue(QScriptEngine* engine, const RayToAvatarIntersectionResult& value) {
    QScriptValue obj = engine->newObject();
    obj.setProperty("intersects", value.intersects);
    QScriptValue avatarIDValue = quuidToScriptValue(engine, value.avatarID);
    obj.setProperty("avatarID", avatarIDValue);
    obj.setProperty("distance", value.distance);
    obj.setProperty("face", boxFaceToString(value.face));

    QScriptValue intersection = vec3toScriptValue(engine, value.intersection);
    obj.setProperty("intersection", intersection);
    QScriptValue surfaceNormal = vec3toScriptValue(engine, value.surfaceNormal);
    obj.setProperty("surfaceNormal", surfaceNormal);
    obj.setProperty("extraInfo", engine->toScriptValue(value.extraInfo));
    return obj;
}

void RayToAvatarIntersectionResultFromScriptValue(const QScriptValue& object, RayToAvatarIntersectionResult& value) {
    value.intersects = object.property("intersects").toVariant().toBool();
    QScriptValue avatarIDValue = object.property("avatarID");
    quuidFromScriptValue(avatarIDValue, value.avatarID);
    value.distance = object.property("distance").toVariant().toFloat();
    value.face = boxFaceFromString(object.property("face").toVariant().toString());

    QScriptValue intersection = object.property("intersection");
    if (intersection.isValid()) {
        vec3FromScriptValue(intersection, value.intersection);
    }
    QScriptValue surfaceNormal = object.property("surfaceNormal");
    if (surfaceNormal.isValid()) {
        vec3FromScriptValue(surfaceNormal, value.surfaceNormal);
    }
    value.extraInfo = object.property("extraInfo").toVariant().toMap();
}

float AvatarData::_avatarSortCoefficientSize { 8.0f };
float AvatarData::_avatarSortCoefficientCenter { 4.0f };
float AvatarData::_avatarSortCoefficientAge { 1.0f };

QScriptValue AvatarEntityMapToScriptValue(QScriptEngine* engine, const AvatarEntityMap& value) {
    QScriptValue obj = engine->newObject();
    for (auto entityID : value.keys()) {
        QByteArray entityProperties = value.value(entityID);
        QJsonDocument jsonEntityProperties = QJsonDocument::fromBinaryData(entityProperties);
        if (!jsonEntityProperties.isObject()) {
            qCDebug(avatars) << "bad AvatarEntityData in AvatarEntityMap" << QString(entityProperties.toHex());
        }

        QVariant variantEntityProperties = jsonEntityProperties.toVariant();
        QVariantMap entityPropertiesMap = variantEntityProperties.toMap();
        QScriptValue scriptEntityProperties = variantMapToScriptValue(entityPropertiesMap, *engine);

        QString key = entityID.toString();
        obj.setProperty(key, scriptEntityProperties);
    }
    return obj;
}

void AvatarEntityMapFromScriptValue(const QScriptValue& object, AvatarEntityMap& value) {
    QScriptValueIterator itr(object);
    while (itr.hasNext()) {
        itr.next();
        QUuid EntityID = QUuid(itr.name());

        QScriptValue scriptEntityProperties = itr.value();
        QVariant variantEntityProperties = scriptEntityProperties.toVariant();
        QJsonDocument jsonEntityProperties = QJsonDocument::fromVariant(variantEntityProperties);
        QByteArray binaryEntityProperties = jsonEntityProperties.toBinaryData();

        value[EntityID] = binaryEntityProperties;
    }
}
