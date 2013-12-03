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

AvatarData::AvatarData(Node* owningNode) :
    NodeData(owningNode),
    _uuid(),
    _handPosition(0,0,0),
    _bodyYaw(-90.0),
    _bodyPitch(0.0),
    _bodyRoll(0.0),
    _newScale(1.0f),
    _leaderUUID(),
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

int AvatarData::getBroadcastData(unsigned char* destinationBuffer) {
    unsigned char* bufferStart = destinationBuffer;
    
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
    
    // UUID
    QByteArray uuidByteArray = _uuid.toRfc4122();
    memcpy(destinationBuffer, uuidByteArray.constData(), uuidByteArray.size());
    destinationBuffer += uuidByteArray.size();
    
    // Body world position
    memcpy(destinationBuffer, &_position, sizeof(float) * 3);
    destinationBuffer += sizeof(float) * 3;
    
    // Body rotation (NOTE: This needs to become a quaternion to save two bytes)
    destinationBuffer += packFloatAngleToTwoByte(destinationBuffer, _bodyYaw);
    destinationBuffer += packFloatAngleToTwoByte(destinationBuffer, _bodyPitch);
    destinationBuffer += packFloatAngleToTwoByte(destinationBuffer, _bodyRoll);

    // Body scale
    destinationBuffer += packFloatRatioToTwoByte(destinationBuffer, _newScale);
    
    // Follow mode info
    memcpy(destinationBuffer, _leaderUUID.toRfc4122().constData(), NUM_BYTES_RFC4122_UUID);
    destinationBuffer += NUM_BYTES_RFC4122_UUID;

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
    
    // skeleton joints
    *destinationBuffer++ = (unsigned char)_joints.size();
    for (vector<JointData>::iterator it = _joints.begin(); it != _joints.end(); it++) {
        *destinationBuffer++ = (unsigned char)it->jointID;
        destinationBuffer += packOrientationQuatToBytes(destinationBuffer, it->rotation);
    }

    return destinationBuffer - bufferStart;
}

// called on the other nodes - assigns it to my views of the others
int AvatarData::parseData(unsigned char* sourceBuffer, int numBytes) {

    // lazily allocate memory for HeadData in case we're not an Avatar instance
    if (!_headData) {
        _headData = new HeadData(this);
    }
    
    // lazily allocate memory for HandData in case we're not an Avatar instance
    if (!_handData) {
        _handData = new HandData(this);
    }
    
    // increment to push past the packet header
    int numBytesPacketHeader = numBytesForPacketHeader(sourceBuffer);
    sourceBuffer += numBytesPacketHeader;
    
    unsigned char* startPosition = sourceBuffer;
    
    // push past the node session UUID
    sourceBuffer += NUM_BYTES_RFC4122_UUID;
    
    // user UUID
    _uuid = QUuid::fromRfc4122(QByteArray((char*) sourceBuffer, NUM_BYTES_RFC4122_UUID));
    sourceBuffer += NUM_BYTES_RFC4122_UUID;
    
    // Body world position
    memcpy(&_position, sourceBuffer, sizeof(float) * 3);
    sourceBuffer += sizeof(float) * 3;
    
    // Body rotation (NOTE: This needs to become a quaternion to save two bytes)
    sourceBuffer += unpackFloatAngleFromTwoByte((uint16_t*) sourceBuffer, &_bodyYaw);
    sourceBuffer += unpackFloatAngleFromTwoByte((uint16_t*) sourceBuffer, &_bodyPitch);
    sourceBuffer += unpackFloatAngleFromTwoByte((uint16_t*) sourceBuffer, &_bodyRoll);

    // Body scale
    sourceBuffer += unpackFloatRatioFromTwoByte(sourceBuffer, _newScale);

    // Follow mode info
    _leaderUUID = QUuid::fromRfc4122(QByteArray((char*) sourceBuffer, NUM_BYTES_RFC4122_UUID));
    sourceBuffer += NUM_BYTES_RFC4122_UUID;

    // Head rotation (NOTE: This needs to become a quaternion to save two bytes)
    float headYaw, headPitch, headRoll;
    sourceBuffer += unpackFloatAngleFromTwoByte((uint16_t*) sourceBuffer, &headYaw);
    sourceBuffer += unpackFloatAngleFromTwoByte((uint16_t*) sourceBuffer, &headPitch);
    sourceBuffer += unpackFloatAngleFromTwoByte((uint16_t*) sourceBuffer, &headRoll);
    
    _headData->setYaw(headYaw);
    _headData->setPitch(headPitch);
    _headData->setRoll(headRoll);

    //  Head position relative to pelvis
    memcpy(&_headData->_leanSideways, sourceBuffer, sizeof(_headData->_leanSideways));
    sourceBuffer += sizeof(float);
    memcpy(&_headData->_leanForward, sourceBuffer, sizeof(_headData->_leanForward));
    sourceBuffer += sizeof(_headData->_leanForward);

    // Hand Position - is relative to body position
    glm::vec3 handPositionRelative;
    memcpy(&handPositionRelative, sourceBuffer, sizeof(float) * 3);
    _handPosition = _position + handPositionRelative;
    sourceBuffer += sizeof(float) * 3;

    // Lookat Position
    memcpy(&_headData->_lookAtPosition, sourceBuffer, sizeof(_headData->_lookAtPosition));
    sourceBuffer += sizeof(_headData->_lookAtPosition);

    // Instantaneous audio loudness (used to drive facial animation)
    //sourceBuffer += unpackFloatFromByte(sourceBuffer, _audioLoudness, MAX_AUDIO_LOUDNESS);
    memcpy(&_headData->_audioLoudness, sourceBuffer, sizeof(float));
    sourceBuffer += sizeof(float);

    // the rest is a chat message
    int chatMessageSize = *sourceBuffer++;
    _chatMessage = string((char*)sourceBuffer, chatMessageSize);
    sourceBuffer += chatMessageSize * sizeof(char);

    // voxel sending features...
    unsigned char bitItems = 0;
    bitItems = (unsigned char)*sourceBuffer++;

    // key state, stored as a semi-nibble in the bitItems
    _keyState = (KeyState)getSemiNibbleAt(bitItems,KEY_STATE_START_BIT);

    // hand state, stored as a semi-nibble in the bitItems
    _handState = getSemiNibbleAt(bitItems,HAND_STATE_START_BIT);

    _headData->_isFaceshiftConnected = oneAtBit(bitItems, IS_FACESHIFT_CONNECTED);

    _isChatCirclingEnabled = oneAtBit(bitItems, IS_CHAT_CIRCLING_ENABLED);

    // If it is connected, pack up the data
    if (_headData->_isFaceshiftConnected) {
        memcpy(&_headData->_leftEyeBlink, sourceBuffer, sizeof(float));
        sourceBuffer += sizeof(float);

        memcpy(&_headData->_rightEyeBlink, sourceBuffer, sizeof(float));
        sourceBuffer += sizeof(float);

        memcpy(&_headData->_averageLoudness, sourceBuffer, sizeof(float));
        sourceBuffer += sizeof(float);

        memcpy(&_headData->_browAudioLift, sourceBuffer, sizeof(float));
        sourceBuffer += sizeof(float);
        
        _headData->_blendshapeCoefficients.resize(*sourceBuffer++);
        memcpy(_headData->_blendshapeCoefficients.data(), sourceBuffer,
            _headData->_blendshapeCoefficients.size() * sizeof(float));
        sourceBuffer += _headData->_blendshapeCoefficients.size() * sizeof(float);
    }
    
    // pupil dilation
    sourceBuffer += unpackFloatFromByte(sourceBuffer, _headData->_pupilDilation, 1.0f);
    
    // leap hand data
    if (sourceBuffer - startPosition < numBytes) {
        // check passed, bytes match
        sourceBuffer += _handData->decodeRemoteData(sourceBuffer);
    }
    
    // skeleton joints
    if (sourceBuffer - startPosition < numBytes) {
        // check passed, bytes match
        _joints.resize(*sourceBuffer++);
        for (vector<JointData>::iterator it = _joints.begin(); it != _joints.end(); it++) {
            it->jointID = *sourceBuffer++;
            sourceBuffer += unpackOrientationQuatFromBytes(sourceBuffer, it->rotation); 
        }
    }

    return sourceBuffer - startPosition;
}
