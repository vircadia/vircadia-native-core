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

const int BYTES_PER_AVATAR = 94;

class AvatarData : public AgentData {
public:
    AvatarData();
    ~AvatarData();
    
    AvatarData* clone() const;
    
    glm::vec3 getBodyPosition();
    void setBodyPosition(glm::vec3 bodyPosition);
    void setHandPosition(glm::vec3 handPosition);
    
    int getBroadcastData(unsigned char* destinationBuffer);
    void parseData(unsigned char* sourceBuffer, int numBytes);
    
    float getBodyYaw();
    void  setBodyYaw(float bodyYaw);
    
    float getBodyPitch();
    void  setBodyPitch(float bodyPitch);
    
    float getBodyRoll();
    void  setBodyRoll(float bodyRoll);

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
    glm::vec3 _bodyPosition;
    glm::vec3 _handPosition;
    
    float _bodyYaw;
    float _bodyPitch;
    float _bodyRoll;

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
