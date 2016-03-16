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

#include <QVariantGLM.h>
#include <Transform.h>
#include <NetworkAccessManager.h>
#include <NodeList.h>
#include <udt/PacketHeaders.h>
#include <GLMHelpers.h>
#include <StreamUtils.h>
#include <UUID.h>
#include <shared/JSONHelpers.h>

#include "AvatarLogging.h"

quint64 DEFAULT_FILTERED_LOG_EXPIRY = 2 * USECS_PER_SECOND;

using namespace std;

const glm::vec3 DEFAULT_LOCAL_AABOX_CORNER(-0.5f);
const glm::vec3 DEFAULT_LOCAL_AABOX_SCALE(1.0f);

const QString AvatarData::FRAME_NAME = "com.highfidelity.recording.AvatarData";
static std::once_flag frameTypeRegistration;

AvatarData::AvatarData() :
    SpatiallyNestable(NestableType::Avatar, QUuid()),
    _handPosition(0.0f),
    _targetScale(1.0f),
    _handState(0),
    _keyState(NO_KEY_DOWN),
    _forceFaceTrackerConnected(false),
    _hasNewJointRotations(true),
    _hasNewJointTranslations(true),
    _headData(NULL),
    _faceModelURL("http://invalid.com"),
    _displayNameTargetAlpha(1.0f),
    _displayNameAlpha(1.0f),
    _billboard(),
    _errorLogExpiry(0),
    _owningAvatarMixer(),
    _targetVelocity(0.0f),
    _localAABox(DEFAULT_LOCAL_AABOX_CORNER, DEFAULT_LOCAL_AABOX_SCALE)
{
    setBodyPitch(0.0f);
    setBodyYaw(-90.0f);
    setBodyRoll(0.0f);
}

AvatarData::~AvatarData() {
    delete _headData;
}

// We cannot have a file-level variable (const or otherwise) in the header if it uses PathUtils, because that references Application, which will not yet initialized.
// Thus we have a static class getter, referencing a static class var.
QUrl AvatarData::_defaultFullAvatarModelUrl = {}; // In C++, if this initialization were in the header, every file would have it's own copy, even for class vars.
const QUrl& AvatarData::defaultFullAvatarModelUrl() {
    if (_defaultFullAvatarModelUrl.isEmpty()) {
        _defaultFullAvatarModelUrl = QUrl::fromLocalFile(PathUtils::resourcesPath() + "meshes/defaultAvatar_full.fst");
    }
    return _defaultFullAvatarModelUrl;
}

// There are a number of possible strategies for this set of tools through endRender, below.
void AvatarData::nextAttitude(glm::vec3 position, glm::quat orientation) {
    avatarLock.lock();
    bool success;
    Transform trans = getTransform(success);
    if (!success) {
        qDebug() << "Warning -- AvatarData::nextAttitude failed";
        return;
    }
    trans.setTranslation(position);
    trans.setRotation(orientation);
    SpatiallyNestable::setTransform(trans, success);
    if (!success) {
        qDebug() << "Warning -- AvatarData::nextAttitude failed";
    }
    avatarLock.unlock();
    updateAttitude();
}
void AvatarData::startCapture() {
    avatarLock.lock();
}
void AvatarData::endCapture() {
    avatarLock.unlock();
}
void AvatarData::startUpdate() {
    avatarLock.lock();
}
void AvatarData::endUpdate() {
    avatarLock.unlock();
}
void AvatarData::startRenderRun() {
    // I'd like to get rid of this and just (un)lock at (end-)startRender.
    // But somehow that causes judder in rotations.
    avatarLock.lock();
}
void AvatarData::endRenderRun() {
    avatarLock.unlock();
}
void AvatarData::startRender() {
    updateAttitude();
}
void AvatarData::endRender() {
    updateAttitude();
}

float AvatarData::getTargetScale() const {
    return _targetScale;
}

void AvatarData::setTargetScale(float targetScale) {
    _targetScale = glm::clamp(targetScale, MIN_AVATAR_SCALE, MAX_AVATAR_SCALE);
}

void AvatarData::setTargetScaleVerbose(float targetScale) {
    setTargetScale(targetScale);
    qCDebug(avatars) << "Changed scale to " << _targetScale;
}

glm::vec3 AvatarData::getHandPosition() const {
    return getOrientation() * _handPosition + getPosition();
}

void AvatarData::setHandPosition(const glm::vec3& handPosition) {
    // store relative to position/orientation
    _handPosition = glm::inverse(getOrientation()) * (handPosition - getPosition());
}

QByteArray AvatarData::toByteArray(bool cullSmallChanges, bool sendAll) {
    // TODO: DRY this up to a shared method
    // that can pack any type given the number of bytes
    // and return the number of bytes to push the pointer

    // lazily allocate memory for HeadData in case we're not an Avatar instance
    if (!_headData) {
        _headData = new HeadData(this);
    }
    if (_forceFaceTrackerConnected) {
        _headData->_isFaceTrackerConnected = true;
    }

    QByteArray avatarDataByteArray(udt::MAX_PACKET_SIZE, 0);

    unsigned char* destinationBuffer = reinterpret_cast<unsigned char*>(avatarDataByteArray.data());
    unsigned char* startPosition = destinationBuffer;

    const glm::vec3& position = getLocalPosition();
    memcpy(destinationBuffer, &position, sizeof(position));
    destinationBuffer += sizeof(position);

    memcpy(destinationBuffer, &_globalPosition, sizeof(_globalPosition));
    destinationBuffer += sizeof(_globalPosition);

    // Body rotation
    glm::vec3 bodyEulerAngles = glm::degrees(safeEulerAngles(getLocalOrientation()));
    destinationBuffer += packFloatAngleToTwoByte(destinationBuffer, bodyEulerAngles.y);
    destinationBuffer += packFloatAngleToTwoByte(destinationBuffer, bodyEulerAngles.x);
    destinationBuffer += packFloatAngleToTwoByte(destinationBuffer, bodyEulerAngles.z);

    // Body scale
    destinationBuffer += packFloatRatioToTwoByte(destinationBuffer, _targetScale);

    // Lookat Position
    memcpy(destinationBuffer, &_headData->_lookAtPosition, sizeof(_headData->_lookAtPosition));
    destinationBuffer += sizeof(_headData->_lookAtPosition);

    // Instantaneous audio loudness (used to drive facial animation)
    memcpy(destinationBuffer, &_headData->_audioLoudness, sizeof(float));
    destinationBuffer += sizeof(float);

    // bitMask of less than byte wide items
    unsigned char bitItems = 0;

    // key state
    setSemiNibbleAt(bitItems,KEY_STATE_START_BIT,_keyState);
    // hand state
    bool isFingerPointing = _handState & IS_FINGER_POINTING_FLAG;
    setSemiNibbleAt(bitItems, HAND_STATE_START_BIT, _handState & ~IS_FINGER_POINTING_FLAG);
    if (isFingerPointing) {
        setAtBit(bitItems, HAND_STATE_FINGER_POINTING_BIT);
    }
    // faceshift state
    if (_headData->_isFaceTrackerConnected) {
        setAtBit(bitItems, IS_FACESHIFT_CONNECTED);
    }
    // eye tracker state
    if (_headData->_isEyeTrackerConnected) {
        setAtBit(bitItems, IS_EYE_TRACKER_CONNECTED);
    }
    // referential state
    QUuid parentID = getParentID();
    if (!parentID.isNull()) {
        setAtBit(bitItems, HAS_REFERENTIAL);
    }
    *destinationBuffer++ = bitItems;

    if (!parentID.isNull()) {
        QByteArray referentialAsBytes = parentID.toRfc4122();
        memcpy(destinationBuffer, referentialAsBytes.data(), referentialAsBytes.size());
        destinationBuffer += referentialAsBytes.size();
        memcpy(destinationBuffer, &_parentJointIndex, sizeof(_parentJointIndex));
        destinationBuffer += sizeof(_parentJointIndex);
    }

    // If it is connected, pack up the data
    if (_headData->_isFaceTrackerConnected) {
        memcpy(destinationBuffer, &_headData->_leftEyeBlink, sizeof(float));
        destinationBuffer += sizeof(float);

        memcpy(destinationBuffer, &_headData->_rightEyeBlink, sizeof(float));
        destinationBuffer += sizeof(float);

        memcpy(destinationBuffer, &_headData->_averageLoudness, sizeof(float));
        destinationBuffer += sizeof(float);

        memcpy(destinationBuffer, &_headData->_browAudioLift, sizeof(float));
        destinationBuffer += sizeof(float);

        *destinationBuffer++ = _headData->_blendshapeCoefficients.size();
        memcpy(destinationBuffer, _headData->_blendshapeCoefficients.data(),
            _headData->_blendshapeCoefficients.size() * sizeof(float));
        destinationBuffer += _headData->_blendshapeCoefficients.size() * sizeof(float);
    }

    // pupil dilation
    destinationBuffer += packFloatToByte(destinationBuffer, _headData->_pupilDilation, 1.0f);

    // joint rotation data
    *destinationBuffer++ = _jointData.size();
    unsigned char* validityPosition = destinationBuffer;
    unsigned char validity = 0;
    int validityBit = 0;

    #ifdef WANT_DEBUG
    int rotationSentCount = 0;
    unsigned char* beforeRotations = destinationBuffer;
    #endif

    _lastSentJointData.resize(_jointData.size());

    for (int i=0; i < _jointData.size(); i++) {
        const JointData& data = _jointData.at(i);
        if (sendAll || _lastSentJointData[i].rotation != data.rotation) {
            if (sendAll ||
                !cullSmallChanges ||
                fabsf(glm::dot(data.rotation, _lastSentJointData[i].rotation)) <= AVATAR_MIN_ROTATION_DOT) {
                if (data.rotationSet) {
                    validity |= (1 << validityBit);
                    #ifdef WANT_DEBUG
                    rotationSentCount++;
                    #endif
                }
            }
        }
        if (++validityBit == BITS_IN_BYTE) {
            *destinationBuffer++ = validity;
            validityBit = validity = 0;
        }
    }
    if (validityBit != 0) {
        *destinationBuffer++ = validity;
    }

    validityBit = 0;
    validity = *validityPosition++;
    for (int i = 0; i < _jointData.size(); i ++) {
        const JointData& data = _jointData[ i ];
        if (validity & (1 << validityBit)) {
            destinationBuffer += packOrientationQuatToBytes(destinationBuffer, data.rotation);
        }
        if (++validityBit == BITS_IN_BYTE) {
            validityBit = 0;
            validity = *validityPosition++;
        }
    }


    // joint translation data
    validityPosition = destinationBuffer;
    validity = 0;
    validityBit = 0;

    #ifdef WANT_DEBUG
    int translationSentCount = 0;
    unsigned char* beforeTranslations = destinationBuffer;
    #endif

    float maxTranslationDimension = 0.0;
    for (int i=0; i < _jointData.size(); i++) {
        const JointData& data = _jointData.at(i);
        if (sendAll || _lastSentJointData[i].translation != data.translation) {
            if (sendAll ||
                !cullSmallChanges ||
                glm::distance(data.translation, _lastSentJointData[i].translation) > AVATAR_MIN_TRANSLATION) {
                if (data.translationSet) {
                    validity |= (1 << validityBit);
                    #ifdef WANT_DEBUG
                    translationSentCount++;
                    #endif
                    maxTranslationDimension = glm::max(fabsf(data.translation.x), maxTranslationDimension);
                    maxTranslationDimension = glm::max(fabsf(data.translation.y), maxTranslationDimension);
                    maxTranslationDimension = glm::max(fabsf(data.translation.z), maxTranslationDimension);
                }
            }
        }
        if (++validityBit == BITS_IN_BYTE) {
            *destinationBuffer++ = validity;
            validityBit = validity = 0;
        }
    }


    if (validityBit != 0) {
        *destinationBuffer++ = validity;
    }

    // TODO -- automatically pick translationCompressionRadix
    int translationCompressionRadix = 12;

    *destinationBuffer++ = translationCompressionRadix;

    validityBit = 0;
    validity = *validityPosition++;
    for (int i = 0; i < _jointData.size(); i ++) {
        const JointData& data = _jointData[ i ];
        if (validity & (1 << validityBit)) {
            destinationBuffer +=
                packFloatVec3ToSignedTwoByteFixed(destinationBuffer, data.translation, translationCompressionRadix);
        }
        if (++validityBit == BITS_IN_BYTE) {
            validityBit = 0;
            validity = *validityPosition++;
        }
    }

    #ifdef WANT_DEBUG
    if (sendAll) {
        qDebug() << "AvatarData::toByteArray" << cullSmallChanges << sendAll
                 << "rotations:" << rotationSentCount << "translations:" << translationSentCount
                 << "largest:" << maxTranslationDimension
                 << "radix:" << translationCompressionRadix
                 << "size:"
                 << (beforeRotations - startPosition) << "+"
                 << (beforeTranslations - beforeRotations) << "+"
                 << (destinationBuffer - beforeTranslations) << "="
                 << (destinationBuffer - startPosition);
    }
    #endif

    return avatarDataByteArray.left(destinationBuffer - startPosition);
}

void AvatarData::doneEncoding(bool cullSmallChanges) {
    // The server has finished sending this version of the joint-data to other nodes.  Update _lastSentJointData.
    _lastSentJointData.resize(_jointData.size());
    for (int i = 0; i < _jointData.size(); i ++) {
        const JointData& data = _jointData[ i ];
        if (_lastSentJointData[i].rotation != data.rotation) {
            if (!cullSmallChanges ||
                fabsf(glm::dot(data.rotation, _lastSentJointData[i].rotation)) <= AVATAR_MIN_ROTATION_DOT) {
                if (data.rotationSet) {
                    _lastSentJointData[i].rotation = data.rotation;
                }
            }
        }
        if (_lastSentJointData[i].translation != data.translation) {
            if (!cullSmallChanges ||
                glm::distance(data.translation, _lastSentJointData[i].translation) > AVATAR_MIN_TRANSLATION) {
                if (data.translationSet) {
                    _lastSentJointData[i].translation = data.translation;
                }
            }
        }
    }
}

bool AvatarData::shouldLogError(const quint64& now) {
    if (now > _errorLogExpiry) {
        _errorLogExpiry = now + DEFAULT_FILTERED_LOG_EXPIRY;
        return true;
    }
    return false;
}

// read data in packet starting at byte offset and return number of bytes parsed
int AvatarData::parseDataFromBuffer(const QByteArray& buffer) {

    // lazily allocate memory for HeadData in case we're not an Avatar instance
    if (!_headData) {
        _headData = new HeadData(this);
    }

    const unsigned char* startPosition = reinterpret_cast<const unsigned char*>(buffer.data());
    const unsigned char* sourceBuffer = startPosition;
    quint64 now = usecTimestampNow();

    // The absolute minimum size of the update data is as follows:
    // 36 bytes of "plain old data" {
    //     position      = 12 bytes
    //     bodyYaw       =  2 (compressed float)
    //     bodyPitch     =  2 (compressed float)
    //     bodyRoll      =  2 (compressed float)
    //     targetScale   =  2 (compressed float)
    //     lookAt        = 12
    //     audioLoudness =  4
    // }
    // + 1 byte for varying data
    // + 1 byte for pupilSize
    // + 1 byte for numJoints (0)
    // = 39 bytes
    int minPossibleSize = 39;

    int maxAvailableSize = buffer.size();
    if (minPossibleSize > maxAvailableSize) {
        if (shouldLogError(now)) {
            qCDebug(avatars) << "Malformed AvatarData packet at the start; "
                << " displayName = '" << _displayName << "'"
                << " minPossibleSize = " << minPossibleSize
                << " maxAvailableSize = " << maxAvailableSize;
        }
        // this packet is malformed so we report all bytes as consumed
        return maxAvailableSize;
    }

    { // Body world position, rotation, and scale
        // position
        glm::vec3 position;
        memcpy(&position, sourceBuffer, sizeof(position));
        sourceBuffer += sizeof(position);

        memcpy(&_globalPosition, sourceBuffer, sizeof(_globalPosition));
        sourceBuffer += sizeof(_globalPosition);

        if (glm::isnan(position.x) || glm::isnan(position.y) || glm::isnan(position.z)) {
            if (shouldLogError(now)) {
                qCDebug(avatars) << "Discard nan AvatarData::position; displayName = '" << _displayName << "'";
            }
            return maxAvailableSize;
        }
        setLocalPosition(position);

        // rotation (NOTE: This needs to become a quaternion to save two bytes)
        float yaw, pitch, roll;
        sourceBuffer += unpackFloatAngleFromTwoByte((uint16_t*) sourceBuffer, &yaw);
        sourceBuffer += unpackFloatAngleFromTwoByte((uint16_t*) sourceBuffer, &pitch);
        sourceBuffer += unpackFloatAngleFromTwoByte((uint16_t*) sourceBuffer, &roll);
        if (glm::isnan(yaw) || glm::isnan(pitch) || glm::isnan(roll)) {
            if (shouldLogError(now)) {
                qCDebug(avatars) << "Discard nan AvatarData::yaw,pitch,roll; displayName = '" << _displayName << "'";
            }
            return maxAvailableSize;
        }

        // TODO is this safe? will the floats not exactly match?
        // Andrew says:
        // Yes, there is a possibility that the transmitted will not quite match the extracted despite being originally
        // extracted from the exact same quaternion. I followed the code through and it appears the risk is that the
        // avatar's SkeletonModel might fall into the CPU expensive part of Model::updateClusterMatrices() when otherwise it
        // would not have required it.  However, we know we can update many simultaneously animating avatars, and most
        // avatars will be moving constantly anyway, so I don't think we need to worry.
        glm::quat currentOrientation = getLocalOrientation();
        glm::vec3 newEulerAngles(pitch, yaw, roll);
        glm::quat newOrientation = glm::quat(glm::radians(newEulerAngles));
        if (currentOrientation != newOrientation) {
            _hasNewJointRotations = true;
            setLocalOrientation(newOrientation);
        }

        // scale
        float scale;
        sourceBuffer += unpackFloatRatioFromTwoByte(sourceBuffer, scale);
        if (glm::isnan(scale)) {
            if (shouldLogError(now)) {
                qCDebug(avatars) << "Discard nan AvatarData::scale; displayName = '" << _displayName << "'";
            }
            return maxAvailableSize;
        }
        _targetScale = std::max(MIN_AVATAR_SCALE, std::min(MAX_AVATAR_SCALE, scale));
    } // 20 bytes

    { // Lookat Position
        glm::vec3 lookAt;
        memcpy(&lookAt, sourceBuffer, sizeof(lookAt));
        sourceBuffer += sizeof(lookAt);
        if (glm::isnan(lookAt.x) || glm::isnan(lookAt.y) || glm::isnan(lookAt.z)) {
            if (shouldLogError(now)) {
                qCDebug(avatars) << "Discard nan AvatarData::lookAt; displayName = '" << _displayName << "'";
            }
            return maxAvailableSize;
        }
        _headData->_lookAtPosition = lookAt;
    } // 12 bytes

    { // AudioLoudness
        // Instantaneous audio loudness (used to drive facial animation)
        float audioLoudness;
        memcpy(&audioLoudness, sourceBuffer, sizeof(float));
        sourceBuffer += sizeof(float);
        if (glm::isnan(audioLoudness)) {
            if (shouldLogError(now)) {
                qCDebug(avatars) << "Discard nan AvatarData::audioLoudness; displayName = '" << _displayName << "'";
            }
            return maxAvailableSize;
         }
        _headData->_audioLoudness = audioLoudness;
    } // 4 bytes

    { // bitFlags and face data
        unsigned char bitItems = *sourceBuffer++;

        // key state, stored as a semi-nibble in the bitItems
        _keyState = (KeyState)getSemiNibbleAt(bitItems,KEY_STATE_START_BIT);

        // hand state, stored as a semi-nibble plus a bit in the bitItems
        // we store the hand state as well as other items in a shared bitset. The hand state is an octal, but is split
        // into two sections to maintain backward compatibility. The bits are ordered as such (0-7 left to right).
        //     +---+-----+-----+--+
        //     |x,x|H0,H1|x,x,x|H2|
        //     +---+-----+-----+--+
        // Hand state - H0,H1,H2 is found in the 3rd, 4th, and 8th bits
        _handState = getSemiNibbleAt(bitItems, HAND_STATE_START_BIT)
            + (oneAtBit(bitItems, HAND_STATE_FINGER_POINTING_BIT) ? IS_FINGER_POINTING_FLAG : 0);

        _headData->_isFaceTrackerConnected = oneAtBit(bitItems, IS_FACESHIFT_CONNECTED);
        _headData->_isEyeTrackerConnected = oneAtBit(bitItems, IS_EYE_TRACKER_CONNECTED);
        bool hasReferential = oneAtBit(bitItems, HAS_REFERENTIAL);

        if (hasReferential) {
            const int sizeOfPackedUuid = 16;
            QByteArray referentialAsBytes((const char*)sourceBuffer, sizeOfPackedUuid);
            _parentID = QUuid::fromRfc4122(referentialAsBytes);
            sourceBuffer += sizeOfPackedUuid;
            memcpy(&_parentJointIndex, sourceBuffer, sizeof(_parentJointIndex));
            sourceBuffer += sizeof(_parentJointIndex);
        } else {
            _parentID = QUuid();
        }

        if (_headData->_isFaceTrackerConnected) {
            float leftEyeBlink, rightEyeBlink, averageLoudness, browAudioLift;
            minPossibleSize += sizeof(leftEyeBlink) + sizeof(rightEyeBlink) + sizeof(averageLoudness) + sizeof(browAudioLift);
            minPossibleSize++; // one byte for blendDataSize
            if (minPossibleSize > maxAvailableSize) {
                if (shouldLogError(now)) {
                    qCDebug(avatars) << "Malformed AvatarData packet after BitItems;"
                        << " displayName = '" << _displayName << "'"
                        << " minPossibleSize = " << minPossibleSize
                        << " maxAvailableSize = " << maxAvailableSize;
                }
                return maxAvailableSize;
            }
            // unpack face data
            memcpy(&leftEyeBlink, sourceBuffer, sizeof(float));
            sourceBuffer += sizeof(float);

            memcpy(&rightEyeBlink, sourceBuffer, sizeof(float));
            sourceBuffer += sizeof(float);

            memcpy(&averageLoudness, sourceBuffer, sizeof(float));
            sourceBuffer += sizeof(float);

            memcpy(&browAudioLift, sourceBuffer, sizeof(float));
            sourceBuffer += sizeof(float);

            if (glm::isnan(leftEyeBlink) || glm::isnan(rightEyeBlink)
                    || glm::isnan(averageLoudness) || glm::isnan(browAudioLift)) {
                if (shouldLogError(now)) {
                    qCDebug(avatars) << "Discard nan AvatarData::faceData; displayName = '" << _displayName << "'";
                }
                return maxAvailableSize;
            }
            _headData->_leftEyeBlink = leftEyeBlink;
            _headData->_rightEyeBlink = rightEyeBlink;
            _headData->_averageLoudness = averageLoudness;
            _headData->_browAudioLift = browAudioLift;

            int numCoefficients = (int)(*sourceBuffer++);
            int blendDataSize = numCoefficients * sizeof(float);
            minPossibleSize += blendDataSize;
            if (minPossibleSize > maxAvailableSize) {
                if (shouldLogError(now)) {
                    qCDebug(avatars) << "Malformed AvatarData packet after Blendshapes;"
                        << " displayName = '" << _displayName << "'"
                        << " minPossibleSize = " << minPossibleSize
                        << " maxAvailableSize = " << maxAvailableSize;
                }
                return maxAvailableSize;
            }

            _headData->_blendshapeCoefficients.resize(numCoefficients);
            memcpy(_headData->_blendshapeCoefficients.data(), sourceBuffer, blendDataSize);
            sourceBuffer += numCoefficients * sizeof(float);

            //bitItemsDataSize = 4 * sizeof(float) + 1 + blendDataSize;
        }
    } // 1 + bitItemsDataSize bytes

    { // pupil dilation
        sourceBuffer += unpackFloatFromByte(sourceBuffer, _headData->_pupilDilation, 1.0f);
    } // 1 byte

    // joint rotations
    int numJoints = *sourceBuffer++;
    int bytesOfValidity = (int)ceil((float)numJoints / (float)BITS_IN_BYTE);
    minPossibleSize += bytesOfValidity;
    if (minPossibleSize > maxAvailableSize) {
        if (shouldLogError(now)) {
            qCDebug(avatars) << "Malformed AvatarData packet after JointValidityBits;"
                << " displayName = '" << _displayName << "'"
                << " minPossibleSize = " << minPossibleSize
                << " maxAvailableSize = " << maxAvailableSize;
        }
        return maxAvailableSize;
    }
    int numValidJointRotations = 0;
    _jointData.resize(numJoints);

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
    } // 1 + bytesOfValidity bytes

    // each joint rotation component is stored in two bytes (sizeof(uint16_t))
    int COMPONENTS_PER_QUATERNION = 4;
    minPossibleSize += numValidJointRotations * COMPONENTS_PER_QUATERNION * sizeof(uint16_t);
    if (minPossibleSize > maxAvailableSize) {
        if (shouldLogError(now)) {
            qCDebug(avatars) << "Malformed AvatarData packet after JointData rotation validity;"
                << " displayName = '" << _displayName << "'"
                << " minPossibleSize = " << minPossibleSize
                << " maxAvailableSize = " << maxAvailableSize;
        }
        return maxAvailableSize;
    }

    { // joint data
        for (int i = 0; i < numJoints; i++) {
            JointData& data = _jointData[i];
            if (validRotations[i]) {
                _hasNewJointRotations = true;
                data.rotationSet = true;
                sourceBuffer += unpackOrientationQuatFromBytes(sourceBuffer, data.rotation);
            }
        }
    } // numJoints * 8 bytes

    // joint translations
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

    // each joint translation component is stored in 6 bytes.  1 byte for translationCompressionRadix
    minPossibleSize += numValidJointTranslations * 6 + 1;
    if (minPossibleSize > maxAvailableSize) {
        if (shouldLogError(now)) {
            qCDebug(avatars) << "Malformed AvatarData packet after JointData translation validity;"
                << " displayName = '" << _displayName << "'"
                << " minPossibleSize = " << minPossibleSize
                << " maxAvailableSize = " << maxAvailableSize;
        }
        return maxAvailableSize;
    }

    int translationCompressionRadix = *sourceBuffer++;

    { // joint data
        for (int i = 0; i < numJoints; i++) {
            JointData& data = _jointData[i];
            if (validTranslations[i]) {
                sourceBuffer +=
                    unpackFloatVec3FromSignedTwoByteFixed(sourceBuffer, data.translation, translationCompressionRadix);
                _hasNewJointTranslations = true;
                data.translationSet = true;
            }
        }
    } // numJoints * 12 bytes

    #ifdef WANT_DEBUG
    if (numValidJointRotations > 15) {
        qDebug() << "RECEIVING -- rotations:" << numValidJointRotations
                 << "translations:" << numValidJointTranslations
                 << "radix:" << translationCompressionRadix
                 << "size:" << (int)(sourceBuffer - startPosition);
    }
    #endif

    int numBytesRead = sourceBuffer - startPosition;
    _averageBytesReceived.updateAverage(numBytesRead);
    return numBytesRead;
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
    _jointData = data;
}

void AvatarData::setJointData(int index, const glm::quat& rotation, const glm::vec3& translation) {
    if (index == -1) {
        return;
    }
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "setJointData", Q_ARG(int, index), Q_ARG(const glm::quat&, rotation));
        return;
    }
    if (_jointData.size() <= index) {
        _jointData.resize(index + 1);
    }
    JointData& data = _jointData[index];
    data.rotation = rotation;
    data.translation = translation;
}

void AvatarData::clearJointData(int index) {
    if (index == -1) {
        return;
    }
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "clearJointData", Q_ARG(int, index));
        return;
    }
    if (_jointData.size() <= index) {
        _jointData.resize(index + 1);
    }
}

bool AvatarData::isJointDataValid(int index) const {
    if (index == -1) {
        return false;
    }
    if (QThread::currentThread() != thread()) {
        bool result;
        QMetaObject::invokeMethod(const_cast<AvatarData*>(this), "isJointDataValid", Qt::BlockingQueuedConnection,
            Q_RETURN_ARG(bool, result), Q_ARG(int, index));
        return result;
    }
    return index < _jointData.size();
}

glm::quat AvatarData::getJointRotation(int index) const {
    if (index == -1) {
        return glm::quat();
    }
    if (QThread::currentThread() != thread()) {
        glm::quat result;
        QMetaObject::invokeMethod(const_cast<AvatarData*>(this), "getJointRotation", Qt::BlockingQueuedConnection,
            Q_RETURN_ARG(glm::quat, result), Q_ARG(int, index));
        return result;
    }
    return index < _jointData.size() ? _jointData.at(index).rotation : glm::quat();
}


glm::vec3 AvatarData::getJointTranslation(int index) const {
    if (index == -1) {
        return glm::vec3();
    }
    if (QThread::currentThread() != thread()) {
        glm::vec3 result;
        QMetaObject::invokeMethod(const_cast<AvatarData*>(this), "getJointTranslation", Qt::BlockingQueuedConnection,
            Q_RETURN_ARG(glm::vec3, result), Q_ARG(int, index));
        return result;
    }
    return index < _jointData.size() ? _jointData.at(index).translation : glm::vec3();
}

glm::vec3 AvatarData::getJointTranslation(const QString& name) const {
    if (QThread::currentThread() != thread()) {
        glm::vec3 result;
        QMetaObject::invokeMethod(const_cast<AvatarData*>(this), "getJointTranslation", Qt::BlockingQueuedConnection,
            Q_RETURN_ARG(glm::vec3, result), Q_ARG(const QString&, name));
        return result;
    }
    return getJointTranslation(getJointIndex(name));
}

void AvatarData::setJointData(const QString& name, const glm::quat& rotation, const glm::vec3& translation) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "setJointData", Q_ARG(const QString&, name), Q_ARG(const glm::quat&, rotation),
            Q_ARG(const glm::vec3&, translation));
        return;
    }
    setJointData(getJointIndex(name), rotation, translation);
}

void AvatarData::setJointRotation(const QString& name, const glm::quat& rotation) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "setJointRotation", Q_ARG(const QString&, name), Q_ARG(const glm::quat&, rotation));
        return;
    }
    setJointRotation(getJointIndex(name), rotation);
}

void AvatarData::setJointTranslation(const QString& name, const glm::vec3& translation) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "setJointTranslation", Q_ARG(const QString&, name),
            Q_ARG(const glm::vec3&, translation));
        return;
    }
    setJointTranslation(getJointIndex(name), translation);
}

void AvatarData::setJointRotation(int index, const glm::quat& rotation) {
    if (index == -1) {
        return;
    }
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "setJointRotation", Q_ARG(int, index), Q_ARG(const glm::quat&, rotation));
        return;
    }
    if (_jointData.size() <= index) {
        _jointData.resize(index + 1);
    }
    JointData& data = _jointData[index];
    data.rotation = rotation;
}

void AvatarData::setJointTranslation(int index, const glm::vec3& translation) {
    if (index == -1) {
        return;
    }
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "setJointTranslation", Q_ARG(int, index), Q_ARG(const glm::vec3&, translation));
        return;
    }
    if (_jointData.size() <= index) {
        _jointData.resize(index + 1);
    }
    JointData& data = _jointData[index];
    data.translation = translation;
}

void AvatarData::clearJointData(const QString& name) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "clearJointData", Q_ARG(const QString&, name));
        return;
    }
    clearJointData(getJointIndex(name));
}

bool AvatarData::isJointDataValid(const QString& name) const {
    if (QThread::currentThread() != thread()) {
        bool result;
        QMetaObject::invokeMethod(const_cast<AvatarData*>(this), "isJointDataValid", Qt::BlockingQueuedConnection,
            Q_RETURN_ARG(bool, result), Q_ARG(const QString&, name));
        return result;
    }
    return isJointDataValid(getJointIndex(name));
}

glm::quat AvatarData::getJointRotation(const QString& name) const {
    if (QThread::currentThread() != thread()) {
        glm::quat result;
        QMetaObject::invokeMethod(const_cast<AvatarData*>(this), "getJointRotation", Qt::BlockingQueuedConnection,
            Q_RETURN_ARG(glm::quat, result), Q_ARG(const QString&, name));
        return result;
    }
    return getJointRotation(getJointIndex(name));
}

QVector<glm::quat> AvatarData::getJointRotations() const {
    if (QThread::currentThread() != thread()) {
        QVector<glm::quat> result;
        QMetaObject::invokeMethod(const_cast<AvatarData*>(this),
                                  "getJointRotations", Qt::BlockingQueuedConnection,
                                  Q_RETURN_ARG(QVector<glm::quat>, result));
        return result;
    }
    QVector<glm::quat> jointRotations(_jointData.size());
    for (int i = 0; i < _jointData.size(); ++i) {
        jointRotations[i] = _jointData[i].rotation;
    }
    return jointRotations;
}

void AvatarData::setJointRotations(QVector<glm::quat> jointRotations) {
    if (QThread::currentThread() != thread()) {
        QVector<glm::quat> result;
        QMetaObject::invokeMethod(const_cast<AvatarData*>(this),
                                  "setJointRotations", Qt::BlockingQueuedConnection,
                                  Q_ARG(QVector<glm::quat>, jointRotations));
    }
    if (_jointData.size() < jointRotations.size()) {
        _jointData.resize(jointRotations.size());
    }
    for (int i = 0; i < jointRotations.size(); ++i) {
        if (i < _jointData.size()) {
            setJointRotation(i, jointRotations[i]);
        }
    }
}

void AvatarData::setJointTranslations(QVector<glm::vec3> jointTranslations) {
    if (QThread::currentThread() != thread()) {
        QVector<glm::quat> result;
        QMetaObject::invokeMethod(const_cast<AvatarData*>(this),
                                  "setJointTranslations", Qt::BlockingQueuedConnection,
                                  Q_ARG(QVector<glm::vec3>, jointTranslations));
    }

    if (_jointData.size() < jointTranslations.size()) {
        _jointData.resize(jointTranslations.size());
    }
    for (int i = 0; i < jointTranslations.size(); ++i) {
        if (i < _jointData.size()) {
            setJointTranslation(i, jointTranslations[i]);
        }
    }
}

void AvatarData::clearJointsData() {
    for (int i = 0; i < _jointData.size(); ++i) {
        clearJointData(i);
    }
}

bool AvatarData::hasIdentityChangedAfterParsing(const QByteArray& data) {
    QDataStream packetStream(data);

    QUuid avatarUUID;
    QUrl faceModelURL, skeletonModelURL;
    QVector<AttachmentData> attachmentData;
    QString displayName;
    packetStream >> avatarUUID >> faceModelURL >> skeletonModelURL >> attachmentData >> displayName;

    bool hasIdentityChanged = false;

    if (faceModelURL != _faceModelURL) {
        setFaceModelURL(faceModelURL);
        hasIdentityChanged = true;
    }

    if (skeletonModelURL != _skeletonModelURL) {
        setSkeletonModelURL(skeletonModelURL);
        hasIdentityChanged = true;
    }

    if (displayName != _displayName) {
        setDisplayName(displayName);
        hasIdentityChanged = true;
    }

    if (attachmentData != _attachmentData) {
        setAttachmentData(attachmentData);
        hasIdentityChanged = true;
    }

    return hasIdentityChanged;
}

QByteArray AvatarData::identityByteArray() {
    QByteArray identityData;
    QDataStream identityStream(&identityData, QIODevice::Append);
    QUrl emptyURL("");
    const QUrl& urlToSend = (_skeletonModelURL == AvatarData::defaultFullAvatarModelUrl()) ? emptyURL : _skeletonModelURL;

    identityStream << QUuid() << _faceModelURL << urlToSend << _attachmentData << _displayName;

    return identityData;
}

bool AvatarData::hasBillboardChangedAfterParsing(const QByteArray& data) {
    if (data == _billboard) {
        return false;
    }
    _billboard = data;
    return true;
}

void AvatarData::setFaceModelURL(const QUrl& faceModelURL) {
    _faceModelURL = faceModelURL;

    qCDebug(avatars) << "Changing face model for avatar to" << _faceModelURL.toString();
}

void AvatarData::setSkeletonModelURL(const QUrl& skeletonModelURL) {
    const QUrl& expanded = skeletonModelURL.isEmpty() ? AvatarData::defaultFullAvatarModelUrl() : skeletonModelURL;
    if (expanded == _skeletonModelURL) {
        return;
    }
    _skeletonModelURL = expanded;
    qCDebug(avatars) << "Changing skeleton model for avatar to" << _skeletonModelURL.toString();

    updateJointMappings();
}

void AvatarData::setDisplayName(const QString& displayName) {
    _displayName = displayName;

    qCDebug(avatars) << "Changing display name for avatar to" << displayName;
}

QVector<AttachmentData> AvatarData::getAttachmentData() const {
    if (QThread::currentThread() != thread()) {
        QVector<AttachmentData> result;
        QMetaObject::invokeMethod(const_cast<AvatarData*>(this), "getAttachmentData", Qt::BlockingQueuedConnection,
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

void AvatarData::setBillboard(const QByteArray& billboard) {
    _billboard = billboard;

    qCDebug(avatars) << "Changing billboard for avatar.";
}

void AvatarData::setBillboardFromURL(const QString &billboardURL) {
    _billboardURL = billboardURL;


    qCDebug(avatars) << "Changing billboard for avatar to PNG at" << qPrintable(billboardURL);

    QNetworkRequest billboardRequest;
    billboardRequest.setHeader(QNetworkRequest::UserAgentHeader, HIGH_FIDELITY_USER_AGENT);
    billboardRequest.setUrl(QUrl(billboardURL));

    QNetworkAccessManager& networkAccessManager = NetworkAccessManager::getInstance();
    QNetworkReply* networkReply = networkAccessManager.get(billboardRequest);
    connect(networkReply, SIGNAL(finished()), this, SLOT(setBillboardFromNetworkReply()));
}

void AvatarData::setBillboardFromNetworkReply() {
    QNetworkReply* networkReply = reinterpret_cast<QNetworkReply*>(sender());
    setBillboard(networkReply->readAll());
    networkReply->deleteLater();
}

void AvatarData::setJointMappingsFromNetworkReply() {
    QNetworkReply* networkReply = static_cast<QNetworkReply*>(sender());

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
            while (_jointNames.size() < jointIndex + 1) {
                _jointNames.append(QString());
            }
            _jointNames[jointIndex] = jointName;
        }
    }
    for (int i = 0; i < _jointNames.size(); i++) {
        _jointIndices.insert(_jointNames.at(i), i + 1);
    }

    networkReply->deleteLater();
}

void AvatarData::sendAvatarDataPacket() {
    auto nodeList = DependencyManager::get<NodeList>();

    // about 2% of the time, we send a full update (meaning, we transmit all the joint data), even if nothing has changed.
    // this is to guard against a joint moving once, the packet getting lost, and the joint never moving again.
    bool sendFullUpdate = randFloat() < AVATAR_SEND_FULL_UPDATE_RATIO;
    QByteArray avatarByteArray = toByteArray(true, sendFullUpdate);
    doneEncoding(true);

    static AvatarDataSequenceNumber sequenceNumber = 0;

    auto avatarPacket = NLPacket::create(PacketType::AvatarData, avatarByteArray.size() + sizeof(sequenceNumber));
    avatarPacket->writePrimitive(sequenceNumber++);
    avatarPacket->write(avatarByteArray);

    nodeList->broadcastToNodes(std::move(avatarPacket), NodeSet() << NodeType::AvatarMixer);
}

void AvatarData::sendIdentityPacket() {
    auto nodeList = DependencyManager::get<NodeList>();

    QByteArray identityData = identityByteArray();

    auto identityPacket = NLPacket::create(PacketType::AvatarIdentity, identityData.size());
    identityPacket->write(identityData);

    nodeList->broadcastToNodes(std::move(identityPacket), NodeSet() << NodeType::AvatarMixer);
}

void AvatarData::sendBillboardPacket() {
    if (!_billboard.isEmpty()) {
        auto nodeList = DependencyManager::get<NodeList>();

        // This makes sure the billboard won't be too large to send.
        // Once more protocol changes are done and we can send blocks of data we can support sending > MTU sized billboards.
        if (_billboard.size() <= NLPacket::maxPayloadSize(PacketType::AvatarBillboard)) {
            auto billboardPacket = NLPacket::create(PacketType::AvatarBillboard, _billboard.size());
            billboardPacket->write(_billboard);

            nodeList->broadcastToNodes(std::move(billboardPacket), NodeSet() << NodeType::AvatarMixer);
        }
    }
}

void AvatarData::updateJointMappings() {
    _jointIndices.clear();
    _jointNames.clear();

    if (_skeletonModelURL.fileName().toLower().endsWith(".fst")) {
        QNetworkAccessManager& networkAccessManager = NetworkAccessManager::getInstance();
        QNetworkRequest networkRequest = QNetworkRequest(_skeletonModelURL);
        networkRequest.setHeader(QNetworkRequest::UserAgentHeader, HIGH_FIDELITY_USER_AGENT);
        QNetworkReply* networkReply = networkAccessManager.get(networkRequest);
        connect(networkReply, SIGNAL(finished()), this, SLOT(setJointMappingsFromNetworkReply()));
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
        recordingBasis->setRotation(getOrientation());
        recordingBasis->setTranslation(getPosition());
        // TODO: find a  different way to record/playback the Scale of the avatar
        //recordingBasis->setScale(getTargetScale());
    }
    _recordingBasis = recordingBasis;
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
static const QString JSON_AVATAR_ATTACHEMENTS = QStringLiteral("attachments");
static const QString JSON_AVATAR_SCALE = QStringLiteral("scale");

QJsonValue toJsonValue(const JointData& joint) {
    QJsonArray result;
    result.push_back(toJsonValue(joint.rotation));
    result.push_back(toJsonValue(joint.translation));
    return result;
}

JointData jointDataFromJsonValue(const QJsonValue& json) {
    JointData result;
    if (json.isArray()) {
        QJsonArray array = json.toArray();
        result.rotation = quatFromJsonValue(array[0]);
        result.rotationSet = true;
        result.translation = vec3FromJsonValue(array[1]);
        result.translationSet = false;
    }
    return result;
}

QJsonObject AvatarData::toJson() const {
    QJsonObject root;

    if (!getFaceModelURL().isEmpty()) {
        root[JSON_AVATAR_HEAD_MODEL] = getFaceModelURL().toString();
    }
    if (!getSkeletonModelURL().isEmpty()) {
        root[JSON_AVATAR_BODY_MODEL] = getSkeletonModelURL().toString();
    }
    if (!getDisplayName().isEmpty()) {
        root[JSON_AVATAR_DISPLAY_NAME] = getDisplayName();
    }
    if (!getAttachmentData().isEmpty()) {
        QJsonArray attachmentsJson;
        for (auto attachment : getAttachmentData()) {
            attachmentsJson.push_back(attachment.toJson());
        }
        root[JSON_AVATAR_ATTACHEMENTS] = attachmentsJson;
    }

    auto recordingBasis = getRecordingBasis();
    bool success;
    Transform avatarTransform = getTransform(success);
    if (!success) {
        qDebug() << "Warning -- AvatarData::toJson couldn't get avatar transform";
    }
    avatarTransform.setScale(getTargetScale());
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

    auto scale = getTargetScale();
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

void AvatarData::fromJson(const QJsonObject& json) {
    // The head setOrientation likes to overwrite the avatar orientation,
    // so lets do the head first
    // Most head data is relative to the avatar, and needs no basis correction,
    // but the lookat vector does need correction
    if (json.contains(JSON_AVATAR_HEAD)) {
        if (!_headData) {
            _headData = new HeadData(this);
        }
        _headData->fromJson(json[JSON_AVATAR_HEAD].toObject());
    }

    if (json.contains(JSON_AVATAR_HEAD_MODEL)) {
        auto faceModelURL = json[JSON_AVATAR_HEAD_MODEL].toString();
        if (faceModelURL != getFaceModelURL().toString()) {
            QUrl faceModel(faceModelURL);
            if (faceModel.isValid()) {
                setFaceModelURL(faceModel);
            }
        }
    }
    if (json.contains(JSON_AVATAR_BODY_MODEL)) {
        auto bodyModelURL = json[JSON_AVATAR_BODY_MODEL].toString();
        if (bodyModelURL != getSkeletonModelURL().toString()) {
            setSkeletonModelURL(bodyModelURL);
        }
    }
    if (json.contains(JSON_AVATAR_DISPLAY_NAME)) {
        auto newDisplayName = json[JSON_AVATAR_DISPLAY_NAME].toString();
        if (newDisplayName != getDisplayName()) {
            setDisplayName(newDisplayName);
        }
    }

    if (json.contains(JSON_AVATAR_RELATIVE)) {
        // During playback you can either have the recording basis set to the avatar current state
        // meaning that all playback is relative to this avatars starting position, or
        // the basis can be loaded from the recording, meaning the playback is relative to the
        // original avatar location
        // The first is more useful for playing back recordings on your own avatar, while
        // the latter is more useful for playing back other avatars within your scene.

        auto currentBasis = getRecordingBasis();
        if (!currentBasis) {
            currentBasis = std::make_shared<Transform>(Transform::fromJson(json[JSON_AVATAR_BASIS]));
        }

        auto relativeTransform = Transform::fromJson(json[JSON_AVATAR_RELATIVE]);
        auto worldTransform = currentBasis->worldTransform(relativeTransform);
        setPosition(worldTransform.getTranslation());
        setOrientation(worldTransform.getRotation());
    }

    if (json.contains(JSON_AVATAR_SCALE)) {
        setTargetScale((float)json[JSON_AVATAR_SCALE].toDouble());
    }

    if (json.contains(JSON_AVATAR_ATTACHEMENTS) && json[JSON_AVATAR_ATTACHEMENTS].isArray()) {
        QJsonArray attachmentsJson = json[JSON_AVATAR_ATTACHEMENTS].toArray();
        QVector<AttachmentData> attachments;
        for (auto attachmentJson : attachmentsJson) {
            AttachmentData attachment;
            attachment.fromJson(attachmentJson.toObject());
            attachments.push_back(attachment);
        }
        setAttachmentData(attachments);
    }

    // Joint rotations are relative to the avatar, so they require no basis correction
    if (json.contains(JSON_AVATAR_JOINT_ARRAY)) {
        QVector<JointData> jointArray;
        QJsonArray jointArrayJson = json[JSON_AVATAR_JOINT_ARRAY].toArray();
        jointArray.reserve(jointArrayJson.size());
        int i = 0;
        for (const auto& jointJson : jointArrayJson) {
            auto joint = jointDataFromJsonValue(jointJson);
            jointArray.push_back(joint);
            setJointData(i, joint.rotation, joint.translation);
            _jointData[i].rotationSet = true; // Have to do that to broadcast the avatar new pose
            i++;
        }
        setRawJointData(jointArray);
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
        qDebug().noquote() << QJsonDocument(obj).toJson(QJsonDocument::JsonFormat::Indented);
    }
#endif
    return QJsonDocument(root).toBinaryData();
}


void AvatarData::fromFrame(const QByteArray& frameData, AvatarData& result) {
    QJsonDocument doc = QJsonDocument::fromBinaryData(frameData);
#ifdef WANT_JSON_DEBUG
    {
        QJsonObject obj = doc.object();
        obj.remove(JSON_AVATAR_JOINT_ARRAY);
        qDebug().noquote() << QJsonDocument(obj).toJson(QJsonDocument::JsonFormat::Indented);
    }
#endif
    result.fromJson(doc.object());
}

float AvatarData::getBodyYaw() const {
    glm::vec3 eulerAngles = glm::degrees(safeEulerAngles(getOrientation()));
    return eulerAngles.y;
}

void AvatarData::setBodyYaw(float bodyYaw) {
    glm::vec3 eulerAngles = glm::degrees(safeEulerAngles(getOrientation()));
    eulerAngles.y = bodyYaw;
    setOrientation(glm::quat(glm::radians(eulerAngles)));
}

float AvatarData::getBodyPitch() const {
    glm::vec3 eulerAngles = glm::degrees(safeEulerAngles(getOrientation()));
    return eulerAngles.x;
}

void AvatarData::setBodyPitch(float bodyPitch) {
    glm::vec3 eulerAngles = glm::degrees(safeEulerAngles(getOrientation()));
    eulerAngles.x = bodyPitch;
    setOrientation(glm::quat(glm::radians(eulerAngles)));
}

float AvatarData::getBodyRoll() const {
    glm::vec3 eulerAngles = glm::degrees(safeEulerAngles(getOrientation()));
    return eulerAngles.z;
}

void AvatarData::setBodyRoll(float bodyRoll) {
    glm::vec3 eulerAngles = glm::degrees(safeEulerAngles(getOrientation()));
    eulerAngles.z = bodyRoll;
    setOrientation(glm::quat(glm::radians(eulerAngles)));
}

void AvatarData::setPosition(const glm::vec3& position) {
    SpatiallyNestable::setPosition(position);
}

void AvatarData::setOrientation(const glm::quat& orientation) {
    SpatiallyNestable::setOrientation(orientation);
}

glm::quat AvatarData::getAbsoluteJointRotationInObjectFrame(int index) const {
    assert(false);
    return glm::quat();
}

glm::vec3 AvatarData::getAbsoluteJointTranslationInObjectFrame(int index) const {
    assert(false);
    return glm::vec3();
}

QVariant AttachmentData::toVariant() const {
    QVariantMap result;
    result["modelUrl"] = modelURL;
    result["jointName"] = jointName;
    result["translation"] = glmToQMap(translation);
    result["rotation"] = glmToQMap(glm::degrees(safeEulerAngles(rotation)));
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

void AttachmentData::fromVariant(const QVariant& variant) {
    auto map = variant.toMap();
    if (map.contains("modelUrl")) {
        auto urlString = map["modelUrl"].toString();
        modelURL = urlString;
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
        attachment.fromVariant(attachmentVar);
        newAttachments.append(attachment);
    }
    setAttachmentData(newAttachments);
}
