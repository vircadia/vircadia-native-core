//
//  AvatarData.h
//  hifi
//
//  Created by Stephen Birarda on 4/9/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __hifi__AvatarData__
#define __hifi__AvatarData__

#include <string>
#include <inttypes.h>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <NodeData.h>
#include "HeadData.h"
#include "HandData.h"

const int WANT_RESIN_AT_BIT = 0;
const int WANT_COLOR_AT_BIT = 1;
const int WANT_DELTA_AT_BIT = 2;
const int KEY_STATE_START_BIT = 3;  // 4th and 5th bits
const int HAND_STATE_START_BIT = 5; // 6th and 7th bits
const int WANT_OCCLUSION_CULLING_BIT = 7; // 8th bit

const float MAX_AUDIO_LOUDNESS = 1000.0; // close enough for mouth animation


enum KeyState
{
    NO_KEY_DOWN = 0,
    INSERT_KEY_DOWN,
    DELETE_KEY_DOWN
};

class JointData;

class AvatarData : public NodeData {
public:
    AvatarData(Node* owningNode = NULL);
    ~AvatarData();
    
    const glm::vec3& getPosition() const { return _position; }
    
    void setPosition      (const glm::vec3 position      ) { _position       = position;       }
    void setHandPosition  (const glm::vec3 handPosition  ) { _handPosition   = handPosition;   }
    
    int getBroadcastData(unsigned char* destinationBuffer);
    int parseData(unsigned char* sourceBuffer, int numBytes);
    
    //  Body Rotation
    float getBodyYaw() const { return _bodyYaw; }
    void setBodyYaw(float bodyYaw) { _bodyYaw = bodyYaw; }
    float getBodyPitch() const { return _bodyPitch; }
    void setBodyPitch(float bodyPitch) { _bodyPitch = bodyPitch; }
    float getBodyRoll() const {return _bodyRoll; }
    void setBodyRoll(float bodyRoll) { _bodyRoll = bodyRoll; }
    
    //  Hand State
    void setHandState(char s) { _handState = s; };
    char getHandState() const {return _handState; };
    
    // getters for camera details
    const glm::vec3& getCameraPosition()    const { return _cameraPosition; };
    const glm::quat& getCameraOrientation() const { return _cameraOrientation; }
    float getCameraFov()                    const { return _cameraFov; }
    float getCameraAspectRatio()            const { return _cameraAspectRatio; }
    float getCameraNearClip()               const { return _cameraNearClip; }
    float getCameraFarClip()                const { return _cameraFarClip; }

    glm::vec3 calculateCameraDirection() const;

    // setters for camera details    
    void setCameraPosition(const glm::vec3& position)       { _cameraPosition    = position;    }
    void setCameraOrientation(const glm::quat& orientation) { _cameraOrientation = orientation; }
    void setCameraFov(float fov)                            { _cameraFov         = fov;         }
    void setCameraAspectRatio(float aspectRatio)            { _cameraAspectRatio = aspectRatio; }
    void setCameraNearClip(float nearClip)                  { _cameraNearClip    = nearClip;    }
    void setCameraFarClip(float farClip)                    { _cameraFarClip     = farClip;     }
    
    // key state
    void setKeyState(KeyState s) { _keyState = s; }
    KeyState keyState() const { return _keyState; }
    
    // chat message
    void setChatMessage(const std::string& msg) { _chatMessage = msg; }
    const std::string& chatMessage () const { return _chatMessage; }

    // related to Voxel Sending strategies
    bool getWantResIn() const { return _wantResIn; }
    bool getWantColor() const { return _wantColor; }
    bool getWantDelta() const { return _wantDelta; }
    bool getWantOcclusionCulling() const { return _wantOcclusionCulling; }
    void setWantResIn(bool wantResIn) { _wantResIn = wantResIn; }
    void setWantColor(bool wantColor) { _wantColor = wantColor; }
    void setWantDelta(bool wantDelta) { _wantDelta = wantDelta; }
    void setWantOcclusionCulling(bool wantOcclusionCulling) { _wantOcclusionCulling = wantOcclusionCulling; }
    
    void setHeadData(HeadData* headData) { _headData = headData; }
    void setHandData(HandData* handData) { _handData = handData; }
    
protected:
    glm::vec3 _position;
    glm::vec3 _handPosition;
    
    //  Body rotation
    float _bodyYaw;
    float _bodyPitch;
    float _bodyRoll;

    //  Hand state (are we grabbing something or not)
    char _handState;
    
    // camera details for the avatar
    glm::vec3 _cameraPosition;
    glm::quat _cameraOrientation;
    float _cameraFov;
    float _cameraAspectRatio;
    float _cameraNearClip;
    float _cameraFarClip;
    
    // key state
    KeyState _keyState;
    
    // chat message
    std::string _chatMessage;
    
    // voxel server sending items
    bool _wantResIn;
    bool _wantColor;
    bool _wantDelta;
    bool _wantOcclusionCulling;
    
    std::vector<JointData> _joints;
    
    HeadData* _headData;
    HandData* _handData;
    
private:
    // privatize the copy constructor and assignment operator so they cannot be called
    AvatarData(const AvatarData&);
    AvatarData& operator= (const AvatarData&);
};

class JointData {
public:
    
    int jointID;
    glm::quat rotation;
};

// These pack/unpack functions are designed to start specific known types in as efficient a manner
// as possible. Taking advantage of the known characteristics of the semantic types.

// Angles are known to be between 0 and 360deg, this allows us to encode in 16bits with great accuracy
int packFloatAngleToTwoByte(unsigned char* buffer, float angle);
int unpackFloatAngleFromTwoByte(uint16_t* byteAnglePointer, float* destinationPointer);

// Orientation Quats are known to have 4 normalized components be between -1.0 and 1.0 
// this allows us to encode each component in 16bits with great accuracy
int packOrientationQuatToBytes(unsigned char* buffer, const glm::quat& quatInput);
int unpackOrientationQuatFromBytes(unsigned char* buffer, glm::quat& quatOutput);

// Ratios need the be highly accurate when less than 10, but not very accurate above 10, and they
// are never greater than 1000 to 1, this allows us to encode each component in 16bits
int packFloatRatioToTwoByte(unsigned char* buffer, float ratio);
int unpackFloatRatioFromTwoByte(unsigned char* buffer, float& ratio);

// Near/Far Clip values need the be highly accurate when less than 10, but only integer accuracy above 10 and
// they are never greater than 16,000, this allows us to encode each component in 16bits
int packClipValueToTwoByte(unsigned char* buffer, float clipValue);
int unpackClipValueFromTwoByte(unsigned char* buffer, float& clipValue);

// Positive floats that don't need to be very precise
int packFloatToByte(unsigned char* buffer, float value, float scaleBy);
int unpackFloatFromByte(unsigned char* buffer, float& value, float scaleBy);

// Allows sending of fixed-point numbers: radix 1 makes 15.1 number, radix 8 makes 8.8 number, etc
int packFloatScalarToSignedTwoByteFixed(unsigned char* buffer, float scalar, int radix);
int unpackFloatScalarFromSignedTwoByteFixed(int16_t* byteFixedPointer, float* destinationPointer, int radix);

#endif /* defined(__hifi__AvatarData__) */
