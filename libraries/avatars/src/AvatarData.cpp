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

#include <cstdio>
#include <cstring>
#include <stdint.h>

#include <QtCore/QDataStream>
#include <QtCore/QThread>
#include <QtCore/QUuid>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

#include <NetworkAccessManager.h>
#include <NodeList.h>
#include <PacketHeaders.h>
#include <GLMHelpers.h>
#include <StreamUtils.h>
#include <UUID.h>
#include <VoxelConstants.h>

#include "AvatarData.h"

quint64 DEFAULT_FILTERED_LOG_EXPIRY = 2 * USECS_PER_SECOND;

using namespace std;

AvatarData::AvatarData() :
    _sessionUUID(),
    _handPosition(0,0,0),
    _referential(NULL),
    _bodyYaw(-90.f),
    _bodyPitch(0.0f),
    _bodyRoll(0.0f),
    _targetScale(1.0f),
    _handState(0),
    _keyState(NO_KEY_DOWN),
    _isChatCirclingEnabled(false),
    _forceFaceshiftConnected(false),
    _hasNewJointRotations(true),
    _headData(NULL),
    _handData(NULL),
    _faceModelURL("http://invalid.com"),
    _displayNameBoundingRect(), 
    _displayNameTargetAlpha(0.0f), 
    _displayNameAlpha(0.0f),
    _billboard(),
    _errorLogExpiry(0),
    _owningAvatarMixer(),
    _lastUpdateTimer()
{
    
}

AvatarData::~AvatarData() {
    delete _headData;
    delete _handData;
    delete _referential;
}

const glm::vec3& AvatarData::getPosition() {
    if (_referential) {
        _referential->update();
    }
    return _position;
}

void AvatarData::setPosition(const glm::vec3 position, bool overideReferential) {
    if (!_referential || overideReferential) {
        _position = position;
    }
}

glm::quat AvatarData::getOrientation() const {
    if (_referential) {
        _referential->update();
    }
    
    return glm::quat(glm::radians(glm::vec3(_bodyPitch, _bodyYaw, _bodyRoll)));
}

void AvatarData::setOrientation(const glm::quat& orientation, bool overideReferential) {
    if (!_referential || overideReferential) {
        glm::vec3 eulerAngles = glm::degrees(safeEulerAngles(orientation));
        _bodyPitch = eulerAngles.x;
        _bodyYaw = eulerAngles.y;
        _bodyRoll = eulerAngles.z;
    }
}

float AvatarData::getTargetScale() const {
    if (_referential) {
        _referential->update();
    }
    
    return _targetScale;
}

void AvatarData::setTargetScale(float targetScale, bool overideReferential) {
    if (!_referential || overideReferential) {
        _targetScale = targetScale;
    }
}

void AvatarData::setClampedTargetScale(float targetScale, bool overideReferential) {
    
    targetScale =  glm::clamp(targetScale, MIN_AVATAR_SCALE, MAX_AVATAR_SCALE);
    
    setTargetScale(targetScale, overideReferential);
    qDebug() << "Changed scale to " << _targetScale;
}

glm::vec3 AvatarData::getHandPosition() const {
    return getOrientation() * _handPosition + _position;
}

void AvatarData::setHandPosition(const glm::vec3& handPosition) {
    // store relative to position/orientation
    _handPosition = glm::inverse(getOrientation()) * (handPosition - _position);
}

QByteArray AvatarData::toByteArray() {
    // TODO: DRY this up to a shared method
    // that can pack any type given the number of bytes
    // and return the number of bytes to push the pointer
    
    // lazily allocate memory for HeadData in case we're not an Avatar instance
    if (!_headData) {
        _headData = new HeadData(this);
    }
    if (_forceFaceshiftConnected) {
        _headData->_isFaceshiftConnected = true;
    }
    
    QByteArray avatarDataByteArray;
    avatarDataByteArray.resize(MAX_PACKET_SIZE);
    
    unsigned char* destinationBuffer = reinterpret_cast<unsigned char*>(avatarDataByteArray.data());
    unsigned char* startPosition = destinationBuffer;
    
    memcpy(destinationBuffer, &_position, sizeof(_position));
    destinationBuffer += sizeof(_position);
    
    // Body rotation (NOTE: This needs to become a quaternion to save two bytes)
    destinationBuffer += packFloatAngleToTwoByte(destinationBuffer, _bodyYaw);
    destinationBuffer += packFloatAngleToTwoByte(destinationBuffer, _bodyPitch);
    destinationBuffer += packFloatAngleToTwoByte(destinationBuffer, _bodyRoll);
    
    // Body scale
    destinationBuffer += packFloatRatioToTwoByte(destinationBuffer, _targetScale);

    // Head rotation (NOTE: This needs to become a quaternion to save two bytes)
    destinationBuffer += packFloatAngleToTwoByte(destinationBuffer, _headData->getFinalYaw());
    destinationBuffer += packFloatAngleToTwoByte(destinationBuffer, _headData->getFinalPitch());
    destinationBuffer += packFloatAngleToTwoByte(destinationBuffer, _headData->getFinalRoll());
    
    // Head lean X,Z (head lateral and fwd/back motion relative to torso)
    memcpy(destinationBuffer, &_headData->_leanSideways, sizeof(_headData->_leanSideways));
    destinationBuffer += sizeof(_headData->_leanSideways);
    memcpy(destinationBuffer, &_headData->_leanForward, sizeof(_headData->_leanForward));
    destinationBuffer += sizeof(_headData->_leanForward);

    // Lookat Position
    memcpy(destinationBuffer, &_headData->_lookAtPosition, sizeof(_headData->_lookAtPosition));
    destinationBuffer += sizeof(_headData->_lookAtPosition);
     
    // Instantaneous audio loudness (used to drive facial animation)
    memcpy(destinationBuffer, &_headData->_audioLoudness, sizeof(float));
    destinationBuffer += sizeof(float);

    // chat message
    *destinationBuffer++ = _chatMessage.size();
    memcpy(destinationBuffer, _chatMessage.data(), _chatMessage.size() * sizeof(char));
    destinationBuffer += _chatMessage.size() * sizeof(char);
    
    // bitMask of less than byte wide items
    unsigned char bitItems = 0;

    // key state
    setSemiNibbleAt(bitItems,KEY_STATE_START_BIT,_keyState);
    // hand state
    setSemiNibbleAt(bitItems,HAND_STATE_START_BIT,_handState);
    // faceshift state
    if (_headData->_isFaceshiftConnected) {
        setAtBit(bitItems, IS_FACESHIFT_CONNECTED);
    }
    if (_isChatCirclingEnabled) {
        setAtBit(bitItems, IS_CHAT_CIRCLING_ENABLED);
    }
    if (_referential != NULL && _referential->isValid()) {
        setAtBit(bitItems, HAS_REFERENTIAL);
    }
    *destinationBuffer++ = bitItems;
    
    // Add referential
    if (_referential != NULL && _referential->isValid()) {
        destinationBuffer += _referential->packReferential(destinationBuffer);
    }

    // If it is connected, pack up the data
    if (_headData->_isFaceshiftConnected) {
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

    // joint data
    *destinationBuffer++ = _jointData.size();
    unsigned char validity = 0;
    int validityBit = 0;
    foreach (const JointData& data, _jointData) {
        if (data.valid) {
            validity |= (1 << validityBit);
        }
        if (++validityBit == BITS_IN_BYTE) {
            *destinationBuffer++ = validity;
            validityBit = validity = 0;
        }
    }
    if (validityBit != 0) {
        *destinationBuffer++ = validity;
    }
    foreach (const JointData& data, _jointData) {
        if (data.valid) {
            destinationBuffer += packOrientationQuatToBytes(destinationBuffer, data.rotation);
        }
    }
        
    return avatarDataByteArray.left(destinationBuffer - startPosition);
}

bool AvatarData::shouldLogError(const quint64& now) {
    if (now > _errorLogExpiry) {
        _errorLogExpiry = now + DEFAULT_FILTERED_LOG_EXPIRY;
        return true;
    }
    return false;
}

// read data in packet starting at byte offset and return number of bytes parsed
int AvatarData::parseDataAtOffset(const QByteArray& packet, int offset) {
    
    // reset the last heard timer since we have new data for this AvatarData
    _lastUpdateTimer.restart();
    
    // lazily allocate memory for HeadData in case we're not an Avatar instance
    if (!_headData) {
        _headData = new HeadData(this);
    }
    
    // lazily allocate memory for HandData in case we're not an Avatar instance
    if (!_handData) {
        _handData = new HandData(this);
    }
    
    const unsigned char* startPosition = reinterpret_cast<const unsigned char*>(packet.data()) + offset;
    const unsigned char* sourceBuffer = startPosition;
    quint64 now = usecTimestampNow();

    // The absolute minimum size of the update data is as follows:
    // 50 bytes of "plain old data" {
    //     position      = 12 bytes
    //     bodyYaw       =  2 (compressed float)
    //     bodyPitch     =  2 (compressed float)
    //     bodyRoll      =  2 (compressed float)
    //     targetScale   =  2 (compressed float)
    //     headYaw       =  2 (compressed float)
    //     headPitch     =  2 (compressed float)
    //     headRoll      =  2 (compressed float)
    //     leanSideways  =  4
    //     leanForward   =  4
    //     lookAt        = 12
    //     audioLoudness =  4
    // }
    // + 1 byte for messageSize (0)
    // + 1 byte for pupilSize
    // + 1 byte for numJoints (0)
    // = 53 bytes
    int minPossibleSize = 53; 
    
    int maxAvailableSize = packet.size() - offset;
    if (minPossibleSize > maxAvailableSize) {
        if (shouldLogError(now)) {
            qDebug() << "Malformed AvatarData packet at the start; "
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
        
        if (glm::isnan(position.x) || glm::isnan(position.y) || glm::isnan(position.z)) {
            if (shouldLogError(now)) {
                qDebug() << "Discard nan AvatarData::position; displayName = '" << _displayName << "'";
            }
            return maxAvailableSize;
        }
        _position = position;
        
        // rotation (NOTE: This needs to become a quaternion to save two bytes)
        float yaw, pitch, roll;
        sourceBuffer += unpackFloatAngleFromTwoByte((uint16_t*) sourceBuffer, &yaw);
        sourceBuffer += unpackFloatAngleFromTwoByte((uint16_t*) sourceBuffer, &pitch);
        sourceBuffer += unpackFloatAngleFromTwoByte((uint16_t*) sourceBuffer, &roll);
        if (glm::isnan(yaw) || glm::isnan(pitch) || glm::isnan(roll)) {
            if (shouldLogError(now)) {
                qDebug() << "Discard nan AvatarData::yaw,pitch,roll; displayName = '" << _displayName << "'";
            }
            return maxAvailableSize;
        }
        _bodyYaw = yaw;
        _bodyPitch = pitch;
        _bodyRoll = roll;
        
        // scale
        float scale;
        sourceBuffer += unpackFloatRatioFromTwoByte(sourceBuffer, scale);
        if (glm::isnan(scale)) {
            if (shouldLogError(now)) {
                qDebug() << "Discard nan AvatarData::scale; displayName = '" << _displayName << "'";
            }
            return maxAvailableSize;
        }
        _targetScale = scale;
    } // 20 bytes
    
    { // Head rotation 
        //(NOTE: This needs to become a quaternion to save two bytes)
        float headYaw, headPitch, headRoll;
        sourceBuffer += unpackFloatAngleFromTwoByte((uint16_t*) sourceBuffer, &headYaw);
        sourceBuffer += unpackFloatAngleFromTwoByte((uint16_t*) sourceBuffer, &headPitch);
        sourceBuffer += unpackFloatAngleFromTwoByte((uint16_t*) sourceBuffer, &headRoll);
        if (glm::isnan(headYaw) || glm::isnan(headPitch) || glm::isnan(headRoll)) {
            if (shouldLogError(now)) {
                qDebug() << "Discard nan AvatarData::headYaw,headPitch,headRoll; displayName = '" << _displayName << "'";
            }
            return maxAvailableSize;
        }
        _headData->setBaseYaw(headYaw);
        _headData->setBasePitch(headPitch);
        _headData->setBaseRoll(headRoll);
    } // 6 bytes
        
    // Head lean (relative to pelvis)
    {
        float leanSideways, leanForward;
        memcpy(&leanSideways, sourceBuffer, sizeof(float));
        sourceBuffer += sizeof(float);
        memcpy(&leanForward, sourceBuffer, sizeof(float));
        sourceBuffer += sizeof(float);
        if (glm::isnan(leanSideways) || glm::isnan(leanForward)) {
            if (shouldLogError(now)) {
                qDebug() << "Discard nan AvatarData::leanSideways,leanForward; displayName = '" << _displayName << "'";
            }
            return maxAvailableSize;
        }
        _headData->_leanSideways = leanSideways;
        _headData->_leanForward = leanForward;
    } // 8 bytes
    
    { // Lookat Position
        glm::vec3 lookAt;
        memcpy(&lookAt, sourceBuffer, sizeof(lookAt));
        sourceBuffer += sizeof(lookAt);
        if (glm::isnan(lookAt.x) || glm::isnan(lookAt.y) || glm::isnan(lookAt.z)) {
            if (shouldLogError(now)) {
                qDebug() << "Discard nan AvatarData::lookAt; displayName = '" << _displayName << "'";
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
                qDebug() << "Discard nan AvatarData::audioLoudness; displayName = '" << _displayName << "'";
            }
            return maxAvailableSize;
         }
        _headData->_audioLoudness = audioLoudness;
    } // 4 bytes
    
    // chat
    int chatMessageSize = *sourceBuffer++;
    minPossibleSize += chatMessageSize;
    if (minPossibleSize > maxAvailableSize) {
        if (shouldLogError(now)) {
            qDebug() << "Malformed AvatarData packet before ChatMessage;"
                << " displayName = '" << _displayName << "'"
                << " minPossibleSize = " << minPossibleSize 
                << " maxAvailableSize = " << maxAvailableSize;
        }
        return maxAvailableSize;
    }
    { // chat payload
        _chatMessage = string((char*)sourceBuffer, chatMessageSize);
        sourceBuffer += chatMessageSize * sizeof(char);
    } // 1 + chatMessageSize bytes
    
    { // bitFlags and face data
        unsigned char bitItems = *sourceBuffer++;
        
        // key state, stored as a semi-nibble in the bitItems
        _keyState = (KeyState)getSemiNibbleAt(bitItems,KEY_STATE_START_BIT);
        // hand state, stored as a semi-nibble in the bitItems
        _handState = getSemiNibbleAt(bitItems,HAND_STATE_START_BIT);
        
        _headData->_isFaceshiftConnected = oneAtBit(bitItems, IS_FACESHIFT_CONNECTED);
        _isChatCirclingEnabled = oneAtBit(bitItems, IS_CHAT_CIRCLING_ENABLED);
        bool hasReferential = oneAtBit(bitItems, HAS_REFERENTIAL);
        
        // Referential
        if (hasReferential) {
            Referential* ref = new Referential(sourceBuffer, this);
            if (_referential == NULL ||
                ref->version() != _referential->version()) {
                changeReferential(ref);
            } else {
                delete ref;
            }
            _referential->update();
        } else if (_referential != NULL) {
            changeReferential(NULL);
        }
    
    
        if (_headData->_isFaceshiftConnected) {
            float leftEyeBlink, rightEyeBlink, averageLoudness, browAudioLift;
            minPossibleSize += sizeof(leftEyeBlink) + sizeof(rightEyeBlink) + sizeof(averageLoudness) + sizeof(browAudioLift);
            minPossibleSize++; // one byte for blendDataSize
            if (minPossibleSize > maxAvailableSize) {
                if (shouldLogError(now)) {
                    qDebug() << "Malformed AvatarData packet after BitItems;"
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
                    qDebug() << "Discard nan AvatarData::faceData; displayName = '" << _displayName << "'";
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
                    qDebug() << "Malformed AvatarData packet after Blendshapes;"
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
    
    // joint data
    int numJoints = *sourceBuffer++;
    int bytesOfValidity = (int)ceil((float)numJoints / (float)BITS_IN_BYTE);
    minPossibleSize += bytesOfValidity;
    if (minPossibleSize > maxAvailableSize) {
        if (shouldLogError(now)) {
            qDebug() << "Malformed AvatarData packet after JointValidityBits;"
                << " displayName = '" << _displayName << "'"
                << " minPossibleSize = " << minPossibleSize 
                << " maxAvailableSize = " << maxAvailableSize;
        }
        return maxAvailableSize;
    }
    int numValidJoints = 0;
    _jointData.resize(numJoints);
    { // validity bits
        unsigned char validity = 0;
        int validityBit = 0;
        for (int i = 0; i < numJoints; i++) {
            if (validityBit == 0) {
                validity = *sourceBuffer++;
            }
            bool valid = (bool)(validity & (1 << validityBit));
            if (valid) {
                ++numValidJoints;
            }
            _jointData[i].valid = valid;
            validityBit = (validityBit + 1) % BITS_IN_BYTE; 
        }
    }
    // 1 + bytesOfValidity bytes

    // each joint rotation component is stored in two bytes (sizeof(uint16_t))
    int COMPONENTS_PER_QUATERNION = 4;
    minPossibleSize += numValidJoints * COMPONENTS_PER_QUATERNION * sizeof(uint16_t);
    if (minPossibleSize > maxAvailableSize) {
        if (shouldLogError(now)) {
            qDebug() << "Malformed AvatarData packet after JointData;"
                << " displayName = '" << _displayName << "'"
                << " minPossibleSize = " << minPossibleSize 
                << " maxAvailableSize = " << maxAvailableSize;
        }
        return maxAvailableSize;
    }

    { // joint data
        for (int i = 0; i < numJoints; i++) {
            JointData& data = _jointData[i];
            if (data.valid) {
                _hasNewJointRotations = true;
                sourceBuffer += unpackOrientationQuatFromBytes(sourceBuffer, data.rotation);
            }
        }
    } // numJoints * 8 bytes
    
    return sourceBuffer - startPosition;
}

bool AvatarData::hasReferential() {
    return _referential != NULL;
}

bool AvatarData::isPlaying() {
    if (!_player) {
        return false;
    }
    if (QThread::currentThread() != thread()) {
        bool result;
        QMetaObject::invokeMethod(this, "isPlaying", Qt::BlockingQueuedConnection,
                                  Q_RETURN_ARG(bool, result));
        return result;
    }
    return _player && _player->isPlaying();
}

qint64 AvatarData::playerElapsed() {
    if (!_player) {
        return 0;
    }
    if (QThread::currentThread() != thread()) {
        qint64 result;
        QMetaObject::invokeMethod(this, "playerElapsed", Qt::BlockingQueuedConnection,
                                  Q_RETURN_ARG(qint64, result));
        return result;
    }
    return _player->elapsed();
}

qint64 AvatarData::playerLength() {
    if (!_player) {
        return 0;
    }
    if (QThread::currentThread() != thread()) {
        qint64 result;
        QMetaObject::invokeMethod(this, "playerLength", Qt::BlockingQueuedConnection,
                                  Q_RETURN_ARG(qint64, result));
        return result;
    }
    return _player->getRecording()->getLength();
}

void AvatarData::loadRecording(QString filename) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "loadRecording", Qt::BlockingQueuedConnection,
                                  Q_ARG(QString, filename));
        return;
    }
    if (!_player) {
        _player = PlayerPointer(new Player(this));
    }
    
    _player->loadFromFile(filename);
}

void AvatarData::startPlaying(bool fromCurrentPosition) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "startPlaying", Qt::BlockingQueuedConnection,
                                  Q_ARG(bool, fromCurrentPosition));
        return;
    }
    if (!_player) {
        _player = PlayerPointer(new Player(this));
    }
    qDebug() << "AvatarData::startPlaying():" << fromCurrentPosition;
    _player->startPlaying(fromCurrentPosition);
}

void AvatarData::play() {
    if (isPlaying()) {
        if (QThread::currentThread() != thread()) {
            QMetaObject::invokeMethod(this, "play", Qt::BlockingQueuedConnection);
            return;
        }
        
        _player->play();
    }
}

void AvatarData::stopPlaying() {
    if (!_player) {
        return;
    }
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "stopPlaying", Qt::BlockingQueuedConnection);
        return;
    }
    if (_player) {
        _player->stopPlaying();
    }
}

void AvatarData::changeReferential(Referential *ref) {
    delete _referential;
    _referential = ref;
}

void AvatarData::setJointData(int index, const glm::quat& rotation) {
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
    data.valid = true;
    data.rotation = rotation;
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
    _jointData[index].valid = false;
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
    return index < _jointData.size() && _jointData.at(index).valid;
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

void AvatarData::setJointData(const QString& name, const glm::quat& rotation) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "setJointData", Q_ARG(const QString&, name),
            Q_ARG(const glm::quat&, rotation));
        return;
    }
    setJointData(getJointIndex(name), rotation);
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
            setJointData(i, jointRotations[i]);
        }
    }
}

void AvatarData::clearJointsData() {
    for (int i = 0; i < _jointData.size(); ++i) {
        clearJointData(i);
    }
}

bool AvatarData::hasIdentityChangedAfterParsing(const QByteArray &packet) {
    QDataStream packetStream(packet);
    packetStream.skipRawData(numBytesForPacketHeader(packet));
    
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

    identityStream << QUuid() << _faceModelURL << _skeletonModelURL << _attachmentData << _displayName;
    
    return identityData;
}

bool AvatarData::hasBillboardChangedAfterParsing(const QByteArray& packet) {
    QByteArray newBillboard = packet.mid(numBytesForPacketHeader(packet));
    if (newBillboard == _billboard) {
        return false;
    }
    _billboard = newBillboard;
    return true;
}

void AvatarData::setFaceModelURL(const QUrl& faceModelURL) {
    _faceModelURL = faceModelURL;
    
    qDebug() << "Changing face model for avatar to" << _faceModelURL.toString();
}

void AvatarData::setSkeletonModelURL(const QUrl& skeletonModelURL) {
    _skeletonModelURL = skeletonModelURL.isEmpty() ? DEFAULT_BODY_MODEL_URL : skeletonModelURL;
    
    qDebug() << "Changing skeleton model for avatar to" << _skeletonModelURL.toString();
    
    updateJointMappings();
}

void AvatarData::setDisplayName(const QString& displayName) {
    _displayName = displayName;

    qDebug() << "Changing display name for avatar to" << displayName;
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

void AvatarData::attach(const QString& modelURL, const QString& jointName, const glm::vec3& translation,
        const glm::quat& rotation, float scale, bool allowDuplicates, bool useSaved) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "attach", Q_ARG(const QString&, modelURL), Q_ARG(const QString&, jointName),
            Q_ARG(const glm::vec3&, translation), Q_ARG(const glm::quat&, rotation),
            Q_ARG(float, scale), Q_ARG(bool, allowDuplicates), Q_ARG(bool, useSaved));
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
    attachmentData.append(data);
    setAttachmentData(attachmentData);
}

void AvatarData::detachOne(const QString& modelURL, const QString& jointName) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "detachOne", Q_ARG(const QString&, modelURL), Q_ARG(const QString&, jointName));
        return;
    }
    QVector<AttachmentData> attachmentData = getAttachmentData();
    for (QVector<AttachmentData>::iterator it = attachmentData.begin(); it != attachmentData.end(); it++) {
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
            it++;
        }
    }
    setAttachmentData(attachmentData);
}

void AvatarData::setBillboard(const QByteArray& billboard) {
    _billboard = billboard;
    
    qDebug() << "Changing billboard for avatar.";
}

void AvatarData::setBillboardFromURL(const QString &billboardURL) {
    _billboardURL = billboardURL;
    
    
    qDebug() << "Changing billboard for avatar to PNG at" << qPrintable(billboardURL);
    
    QNetworkRequest billboardRequest;
    billboardRequest.setUrl(QUrl(billboardURL));
    
    NetworkAccessManager& networkAccessManager = NetworkAccessManager::getInstance();
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
        if (!(line = line.trimmed()).startsWith("jointIndex")) {
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

void AvatarData::sendIdentityPacket() {
    QByteArray identityPacket = byteArrayWithPopulatedHeader(PacketTypeAvatarIdentity);
    identityPacket.append(identityByteArray());
    
    NodeList::getInstance()->broadcastToNodes(identityPacket, NodeSet() << NodeType::AvatarMixer);
}

void AvatarData::sendBillboardPacket() {
    if (!_billboard.isEmpty()) {
        QByteArray billboardPacket = byteArrayWithPopulatedHeader(PacketTypeAvatarBillboard);
        billboardPacket.append(_billboard);
        
        NodeList::getInstance()->broadcastToNodes(billboardPacket, NodeSet() << NodeType::AvatarMixer);
    }
}

void AvatarData::updateJointMappings() {
    _jointIndices.clear();
    _jointNames.clear();
    
    if (_skeletonModelURL.fileName().toLower().endsWith(".fst")) {
        NetworkAccessManager& networkAccessManager = NetworkAccessManager::getInstance();
        QNetworkReply* networkReply = networkAccessManager.get(QNetworkRequest(_skeletonModelURL));
        connect(networkReply, SIGNAL(finished()), this, SLOT(setJointMappingsFromNetworkReply()));
    }
}

AttachmentData::AttachmentData() :
    scale(1.0f) {
}

bool AttachmentData::operator==(const AttachmentData& other) const {
    return modelURL == other.modelURL && jointName == other.jointName && translation == other.translation &&
        rotation == other.rotation && scale == other.scale;
}

QDataStream& operator<<(QDataStream& out, const AttachmentData& attachment) {
    return out << attachment.modelURL << attachment.jointName <<
        attachment.translation << attachment.rotation << attachment.scale;
}

QDataStream& operator>>(QDataStream& in, AttachmentData& attachment) {
    return in >> attachment.modelURL >> attachment.jointName >>
        attachment.translation >> attachment.rotation >> attachment.scale;
}

void AttachmentDataObject::setModelURL(const QString& modelURL) const {
    AttachmentData data = qscriptvalue_cast<AttachmentData>(thisObject());
    data.modelURL = modelURL;
    thisObject() = engine()->toScriptValue(data);
}

QString AttachmentDataObject::getModelURL() const {
    return qscriptvalue_cast<AttachmentData>(thisObject()).modelURL.toString();
}

void AttachmentDataObject::setJointName(const QString& jointName) const {
    AttachmentData data = qscriptvalue_cast<AttachmentData>(thisObject());
    data.jointName = jointName;
    thisObject() = engine()->toScriptValue(data);
}

QString AttachmentDataObject::getJointName() const {
    return qscriptvalue_cast<AttachmentData>(thisObject()).jointName;
}

void AttachmentDataObject::setTranslation(const glm::vec3& translation) const {
    AttachmentData data = qscriptvalue_cast<AttachmentData>(thisObject());
    data.translation = translation;
    thisObject() = engine()->toScriptValue(data);
}

glm::vec3 AttachmentDataObject::getTranslation() const {
    return qscriptvalue_cast<AttachmentData>(thisObject()).translation;
}

void AttachmentDataObject::setRotation(const glm::quat& rotation) const {
    AttachmentData data = qscriptvalue_cast<AttachmentData>(thisObject());
    data.rotation = rotation;
    thisObject() = engine()->toScriptValue(data);
}

glm::quat AttachmentDataObject::getRotation() const {
    return qscriptvalue_cast<AttachmentData>(thisObject()).rotation;
}

void AttachmentDataObject::setScale(float scale) const {
    AttachmentData data = qscriptvalue_cast<AttachmentData>(thisObject());
    data.scale = scale;
    thisObject() = engine()->toScriptValue(data);
}

float AttachmentDataObject::getScale() const {
    return qscriptvalue_cast<AttachmentData>(thisObject()).scale;
}

void registerAvatarTypes(QScriptEngine* engine) {
    qScriptRegisterSequenceMetaType<QVector<AttachmentData> >(engine);
    engine->setDefaultPrototype(qMetaTypeId<AttachmentData>(), engine->newQObject(
        new AttachmentDataObject(), QScriptEngine::ScriptOwnership));
}

