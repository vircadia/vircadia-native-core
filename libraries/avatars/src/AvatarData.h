//
//  AvatarData.h
//  hifi
//
//  Created by Stephen Birarda on 4/9/13.
//
//

#ifndef __hifi__AvatarData__
#define __hifi__AvatarData__

#include <glm/glm.hpp>

#include <AgentData.h>

class AvatarData : public AgentData {
public:
    AvatarData();
    ~AvatarData();
    
    AvatarData* clone() const;
    
    glm::vec3 getPosition();
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
    const float getHeadPitch() const { return _headPitch; };
    const float getHeadYaw() const { return _headYaw; };
    const float getHeadRoll() const { return _headRoll; };
    void  addHeadPitch(float p) {_headPitch -= p; }
    void  addHeadYaw(float y){_headYaw -= y; }
    void  addHeadRoll(float r){_headRoll += r; }

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
};

#endif /* defined(__hifi__AvatarData__) */
