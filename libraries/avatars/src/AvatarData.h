//
//  AvatarData.h
//  hifi
//
//  Created by Stephen Birarda on 4/9/13.
//
//

#ifndef __hifi__AvatarData__
#define __hifi__AvatarData__

#include <string>

#include <glm/glm.hpp>

#include <AgentData.h>

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
    AvatarData() :
    _handPosition(0,0,0),
    _bodyYaw(-90.0),
    _bodyPitch(0.0),
    _bodyRoll(0.0),
    _headYaw(0),
    _headPitch(0),
    _headRoll(0),
    _headLeanSideways(0),
    _headLeanForward(0),
    _audioLoudness(0),
    _handState(0),
    _cameraPosition(0,0,0),
    _cameraDirection(0,0,0),
    _cameraUp(0,0,0),
    _cameraRight(0,0,0),
    _cameraFov(0.0f),
    _cameraAspectRatio(0.0f),
    _cameraNearClip(0.0f),
    _cameraFarClip(0.0f),
    _keyState(NO_KEY_DOWN),
    _wantResIn(false),
    _wantColor(true) { };
    
    
    ~AvatarData();
    
    AvatarData* clone() const;
    
    const glm::vec3& getPosition() const;
    void setPosition(glm::vec3 position);
    void setHandPosition(glm::vec3 handPosition);
    
    int getBroadcastData(unsigned char* destinationBuffer);
    int parseData(unsigned char* sourceBuffer, int numBytes);
    
    //  Body Rotation
    float getBodyYaw();
    float getBodyPitch();
    float getBodyRoll();
    void  setBodyYaw(float bodyYaw);
    void  setBodyPitch(float bodyPitch);
    void  setBodyRoll(float bodyRoll);

    // Head Rotation
    void setHeadPitch(float p) {_headPitch = p; }
    void setHeadYaw(float y) {_headYaw = y; }
    void setHeadRoll(float r) {_headRoll = r; };
    float getHeadPitch() const { return _headPitch; };
    float getHeadYaw() const { return _headYaw; };
    float getHeadRoll() const { return _headRoll; };
    void  addHeadPitch(float p) {_headPitch -= p; }
    void  addHeadYaw(float y){_headYaw -= y; }
    void  addHeadRoll(float r){_headRoll += r; }

    //  Head vector deflection from pelvix in X,Z
    void setHeadLeanSideways(float s) {_headLeanSideways = s; };
    float getHeadLeanSideways() const { return _headLeanSideways; };
    void setHeadLeanForward(float f) {_headLeanForward = f; };
    float getHeadLeanForward() const { return _headLeanForward; };
    
    //  Hand State
    void setHandState(char s) { _handState = s; };
    char getHandState() const {return _handState; };

    //  Instantaneous audio loudness to drive mouth/facial animation
    void setLoudness(float l) { _audioLoudness = l; };
    float getLoudness() const {return _audioLoudness; };

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
    
protected:
    glm::vec3 _position;
    glm::vec3 _handPosition;
    
    //  Body rotation
    float _bodyYaw;
    float _bodyPitch;
    float _bodyRoll;
    
    //  Head rotation (relative to body) 
    float _headYaw;
    float _headPitch;
    float _headRoll;
    
    float _headLeanSideways;
    float _headLeanForward;

    //  Audio loudness (used to drive facial animation)
    float _audioLoudness;
    
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
};

#endif /* defined(__hifi__AvatarData__) */
