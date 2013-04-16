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

class AvatarData : public AgentData {
public:
    AvatarData();
    ~AvatarData();
    
    AvatarData* clone() const;
    
    glm::vec3 getBodyPosition();
    void setBodyPosition(glm::vec3 bodyPosition);
    
    int getBroadcastData(char* destinationBuffer);
    void parseData(void *sourceBuffer, int numBytes);
    
    float getBodyYaw();
    void  setBodyYaw(float bodyYaw);
    
    float getBodyPitch();
    void  setBodyPitch(float bodyPitch);
    
    float getBodyRoll();
    void  setBodyRoll(float bodyRoll);
    
protected:
    glm::vec3 _bodyPosition;
    
    float _bodyYaw;
    float _bodyPitch;
    float _bodyRoll;
};

#endif /* defined(__hifi__AvatarData__) */
