//
//  HeadData.h
//  hifi
//
//  Created by Stephen Birarda on 5/20/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __hifi__HeadData__
#define __hifi__HeadData__

#include <iostream>

#include <glm/glm.hpp>

class HeadData {
public:
    HeadData();
    
    float getYaw() const { return _yaw; }
    void setYaw(float yaw);
    
    float getPitch() const { return _pitch; }
    void setPitch(float pitch);
    
    float getRoll() const { return _roll; }
    void setRoll(float roll);
    
    void addYaw(float yaw);
    void addPitch(float pitch);
    void addRoll(float roll);
    
    const glm::vec3& getLookAtPosition() const { return _lookAtPosition; }
    void setLookAtPosition(const glm::vec3& lookAtPosition) { _lookAtPosition = lookAtPosition; }
    
    friend class AvatarData;
protected:
    float _yaw;
    float _pitch;
    float _roll;
    glm::vec3 _lookAtPosition;
};

#endif /* defined(__hifi__HeadData__) */
