//
//  AvatarData.cpp
//  hifi
//
//  Created by Stephen Birarda on 4/9/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <cstdio>
#include <cstring>
#include <stdint.h>

#include <NodeList.h>
#include <PacketHeaders.h>
#include <SharedUtil.h>
#include <UUID.h>
#include <VoxelConstants.h>

#include "AvatarData.h"

using namespace std;

static const float fingerVectorRadix = 4; // bits of precision when converting from float<->fixed

AvatarData::AvatarData() :
    NodeData(),
    _handPosition(0,0,0),
    _bodyYaw(-90.0),
    _bodyPitch(0.0),
    _bodyRoll(0.0),
    _targetScale(1.0f),
    _handState(0),
    _keyState(NO_KEY_DOWN),
    _isChatCirclingEnabled(false),
    _headData(NULL),
    _handData(NULL)
{
    
}

AvatarData::~AvatarData() {
    delete _headData;
    delete _handData;
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
    // lazily allocate memory for HeadData in case we're not an Avatar instance
    if (!_handData) {
        _handData = new HandData(this);
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
    destinationBuffer += packFloatAngleToTwoByte(destinationBuffer, _headData->_yaw);
    destinationBuffer += packFloatAngleToTwoByte(destinationBuffer, _headData->_pitch);
    destinationBuffer += packFloatAngleToTwoByte(destinationBuffer, _headData->_roll);
    
    
    // Head lean X,Z (head lateral and fwd/back motion relative to torso)
    memcpy(destinationBuffer, &_headData->_leanSideways, sizeof(_headData->_leanSideways));
    destinationBuffer += sizeof(_headData->_leanSideways);
    memcpy(destinationBuffer, &_headData->_leanForward, sizeof(_headData->_leanForward));
    destinationBuffer += sizeof(_headData->_leanForward);

    // Hand Position - is relative to body position
    glm::vec3 handPositionRelative = _handPosition - _position;
    memcpy(destinationBuffer, &handPositionRelative, sizeof(float) * 3);
    destinationBuffer += sizeof(float) * 3;

    // Lookat Position
    memcpy(destinationBuffer, &_headData->_lookAtPosition, sizeof(_headData->_lookAtPosition));
    destinationBuffer += sizeof(_headData->_lookAtPosition);
     
    // Instantaneous audio loudness (used to drive facial animation)
    //destinationBuffer += packFloatToByte(destinationBuffer, std::min(MAX_AUDIO_LOUDNESS, _audioLoudness), MAX_AUDIO_LOUDNESS);
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
    if (_headData->_isFaceshiftConnected) { setAtBit(bitItems, IS_FACESHIFT_CONNECTED); }
    if (_isChatCirclingEnabled) {
        setAtBit(bitItems, IS_CHAT_CIRCLING_ENABLED);
    }
    *destinationBuffer++ = bitItems;

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
    
    // leap hand data
    destinationBuffer += _handData->encodeRemoteData(destinationBuffer);

    return avatarDataByteArray.left(destinationBuffer - startPosition);
}

// called on the other nodes - assigns it to my views of the others
int AvatarData::parseData(const QByteArray& packet) {

    // lazily allocate memory for HeadData in case we're not an Avatar instance
    if (!_headData) {
        _headData = new HeadData(this);
    }
    
    // lazily allocate memory for HandData in case we're not an Avatar instance
    if (!_handData) {
        _handData = new HandData(this);
    }
    
    QDataStream packetStream(packet);
    packetStream.skipRawData(numBytesForPacketHeader(packet));
    
    packetStream.readRawData(reinterpret_cast<char*>(&_position), sizeof(_position));
    
    // Body rotation (NOTE: This needs to become a quaternion to save two bytes)
    uint16_t twoByteHolder;
    packetStream >> twoByteHolder;
    unpackFloatAngleFromTwoByte(&twoByteHolder, &_bodyYaw);
    
    packetStream >> twoByteHolder;
    unpackFloatAngleFromTwoByte(&twoByteHolder, &_bodyPitch);
    
    packetStream >> twoByteHolder;
    unpackFloatAngleFromTwoByte(&twoByteHolder, &_bodyRoll);
    
    // body scale
    packetStream >> twoByteHolder;
    unpackFloatRatioFromTwoByte(reinterpret_cast<const unsigned char*>(&twoByteHolder), _targetScale);

    // Head rotation (NOTE: This needs to become a quaternion to save two bytes)
    float headYaw, headPitch, headRoll;
    
    packetStream >> twoByteHolder;
    unpackFloatAngleFromTwoByte(&twoByteHolder, &headYaw);
    
    packetStream >> twoByteHolder;
    unpackFloatAngleFromTwoByte(&twoByteHolder, &headPitch);
    
    packetStream >> twoByteHolder;
    unpackFloatAngleFromTwoByte(&twoByteHolder, &headRoll);
    
    _headData->setYaw(headYaw);
    _headData->setPitch(headPitch);
    _headData->setRoll(headRoll);

    //  Head position relative to pelvis
    packetStream >> _headData->_leanSideways;
    packetStream >> _headData->_leanForward;

    // Hand Position - is relative to body position
    glm::vec3 handPositionRelative;
    packetStream.readRawData(reinterpret_cast<char*>(&handPositionRelative), sizeof(handPositionRelative));
    _handPosition = _position + handPositionRelative;
    
    packetStream.readRawData(reinterpret_cast<char*>(&_headData->_lookAtPosition), sizeof(_headData->_lookAtPosition));

    // Instantaneous audio loudness (used to drive facial animation)
    //sourceBuffer += unpackFloatFromByte(sourceBuffer, _audioLoudness, MAX_AUDIO_LOUDNESS);
    packetStream >> _headData->_audioLoudness;

    // the rest is a chat message
    
    quint8 chatMessageSize;
    packetStream >> chatMessageSize;
    _chatMessage = string(packet.data() + packetStream.device()->pos(), chatMessageSize);
    packetStream.skipRawData(chatMessageSize);

    // voxel sending features...
    unsigned char bitItems = 0;
    packetStream >> bitItems;
    
    // key state, stored as a semi-nibble in the bitItems
    _keyState = (KeyState)getSemiNibbleAt(bitItems,KEY_STATE_START_BIT);

    // hand state, stored as a semi-nibble in the bitItems
    _handState = getSemiNibbleAt(bitItems,HAND_STATE_START_BIT);

    _headData->_isFaceshiftConnected = oneAtBit(bitItems, IS_FACESHIFT_CONNECTED);

    _isChatCirclingEnabled = oneAtBit(bitItems, IS_CHAT_CIRCLING_ENABLED);

    // If it is connected, pack up the data
    if (_headData->_isFaceshiftConnected) {
        packetStream >> _headData->_leftEyeBlink;
        packetStream >> _headData->_rightEyeBlink;
        packetStream >> _headData->_averageLoudness;
        packetStream >> _headData->_browAudioLift;
        
        quint8 numBlendshapeCoefficients;
        packetStream >> numBlendshapeCoefficients;
        
        _headData->_blendshapeCoefficients.resize(numBlendshapeCoefficients);
        packetStream.readRawData(reinterpret_cast<char*>(_headData->_blendshapeCoefficients.data()),
                                 numBlendshapeCoefficients * sizeof(float));
    }
    
    // pupil dilation
    quint8 pupilByte;
    packetStream >> pupilByte;
    unpackFloatFromByte(&pupilByte, _headData->_pupilDilation, 1.0f);
    
    // leap hand data
    if (packetStream.device()->pos() < packet.size()) {
        // check passed, bytes match
        packetStream.skipRawData(_handData->decodeRemoteData(packet.mid(packetStream.device()->pos())));
    }

    return packetStream.device()->pos();
}

void AvatarData::setClampedTargetScale(float targetScale) {
    
    targetScale =  glm::clamp(targetScale, MIN_AVATAR_SCALE, MAX_AVATAR_SCALE);
    
    _targetScale = targetScale;
    qDebug() << "Changed scale to " << _targetScale;
}

void AvatarData::setOrientation(const glm::quat& orientation) {
    glm::vec3 eulerAngles = safeEulerAngles(orientation);
    _bodyPitch = eulerAngles.x;
    _bodyYaw = eulerAngles.y;
    _bodyRoll = eulerAngles.z;
}
