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

#include <glm/glm.hpp>

#include <AgentData.h>
#include "HeadData.h"

const int WANT_RESIN_AT_BIT = 0;
const int WANT_COLOR_AT_BIT = 1;
const int WANT_DELTA_AT_BIT = 2;

enum KeyState
{
    NO_KEY_DOWN,
    INSERT_KEY_DOWN,
    DELETE_KEY_DOWN
};

class AvatarData : public AgentData {
public:
    AvatarData(Agent* owningAgent = NULL);
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
    const glm::vec3& getCameraDirection()   const { return _cameraDirection; }
    const glm::vec3& getCameraUp()          const { return _cameraUp; }
    const glm::vec3& getCameraRight()       const { return _cameraRight; }
    float getCameraFov()                    const { return _cameraFov; }
    float getCameraAspectRatio()            const { return _cameraAspectRatio; }
    float getCameraNearClip()               const { return _cameraNearClip; }
    float getCameraFarClip()                const { return _cameraFarClip; }

    // setters for camera details    
    void setCameraPosition(const glm::vec3& position)   { _cameraPosition    = position; };
    void setCameraDirection(const glm::vec3& direction) { _cameraDirection   = direction; }
    void setCameraUp(const glm::vec3& up)               { _cameraUp          = up; }
    void setCameraRight(const glm::vec3& right)         { _cameraRight       = right; }
    void setCameraFov(float fov)                        { _cameraFov         = fov; }
    void setCameraAspectRatio(float aspectRatio)        { _cameraAspectRatio = aspectRatio; }
    void setCameraNearClip(float nearClip)              { _cameraNearClip    = nearClip; }
    void setCameraFarClip(float farClip)                { _cameraFarClip     = farClip; }
    
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
    void setWantResIn(bool wantResIn) { _wantResIn = wantResIn; }
    void setWantColor(bool wantColor) { _wantColor = wantColor; }
    void setWantDelta(bool wantDelta) { _wantDelta = wantDelta; }
    
    void setHeadData(HeadData* headData) { _headData = headData; }
    
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

    // can we describe this in less space? For example, a Quaternion? or Euler angles?
    glm::vec3 _cameraDirection;
    glm::vec3 _cameraUp;
    glm::vec3 _cameraRight;
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
    
    HeadData* _headData;
private:
    // privatize the copy constructor and assignment operator so they cannot be called
    AvatarData(const AvatarData&);
    AvatarData& operator= (const AvatarData&);
};

#endif /* defined(__hifi__AvatarData__) */
