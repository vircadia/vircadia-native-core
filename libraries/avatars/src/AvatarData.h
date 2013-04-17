//
//  AvatarData.h
//  hifi
//
//  Created by Stephen Birarda on 4/9/13.
//
//

#ifndef __hifi__AvatarData__
#define __hifi__AvatarData__

#include <iostream>

#include <glm/glm.hpp>

#include <AgentData.h>

const int BYTES_PER_AVATAR = 30;

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
    
protected:
    glm::vec3 _bodyPosition;
    glm::vec3 _handPosition;
    
    float _bodyYaw;
    float _bodyPitch;
    float _bodyRoll;
};

#endif /* defined(__hifi__AvatarData__) */
