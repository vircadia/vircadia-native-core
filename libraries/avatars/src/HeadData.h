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

const float MIN_HEAD_YAW = -85;
const float MAX_HEAD_YAW = 85;
const float MIN_HEAD_PITCH = -60;
const float MAX_HEAD_PITCH = 60;
const float MIN_HEAD_ROLL = -50;
const float MAX_HEAD_ROLL = 50;

class HeadData {
public:
    HeadData();
    
    float getLeanSideways() const { return _leanSideways; }
    void setLeanSideways(float leanSideways) { _leanSideways = leanSideways; }
    
    float getLeanForward() const { return _leanForward; }
    void setLeanForward(float leanForward) { _leanForward = leanForward; }
    
    float getYaw() const { return _yaw; }
    void setYaw(float yaw) { _yaw = glm::clamp(yaw, MIN_HEAD_YAW, MAX_HEAD_YAW); }
    
    float getPitch() const { return _pitch; }
    void setPitch(float pitch) { _pitch = glm::clamp(pitch, MIN_HEAD_PITCH, MAX_HEAD_PITCH); }
    
    float getRoll() const { return _roll; }
    void setRoll(float roll) { _roll = glm::clamp(roll, MIN_HEAD_ROLL, MAX_HEAD_ROLL); }
    
    void addYaw(float yaw);
    void addPitch(float pitch);
    void addRoll(float roll);
    void addLean(float sideways, float forwards);
    
    const glm::vec3& getLookAtPosition() const { return _lookAtPosition; }
    void setLookAtPosition(const glm::vec3& lookAtPosition) { _lookAtPosition = lookAtPosition; }
    
    friend class AvatarData;
protected:
    float _yaw;
    float _pitch;
    float _roll;
    glm::vec3 _lookAtPosition;
    float _leanSideways;
    float _leanForward;
};

#endif /* defined(__hifi__HeadData__) */
