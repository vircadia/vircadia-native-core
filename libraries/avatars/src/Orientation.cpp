//-----------------------------------------------------------
//
// Created by Jeffrey Ventrella  
// Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//-----------------------------------------------------------

#include "Orientation.h"
#include "SharedUtil.h"

static const bool USING_QUATERNIONS = true;

Orientation::Orientation() {
    setToIdentity();
}

void Orientation::setToIdentity() {

    quat  = glm::quat();
    right = glm::vec3(IDENTITY_RIGHT);
	up	  = glm::vec3(IDENTITY_UP   );
	front = glm::vec3(IDENTITY_FRONT);
}

void Orientation::setToPitchYawRoll(float pitch_change, float yaw_change, float roll_change) {

    setToIdentity();
    pitch(pitch_change);
    yaw  (yaw_change);
    roll (roll_change);
}


void Orientation::set(Orientation o) { 

    quat  = o.quat;
	right = o.right;
	up    = o.up;
	front = o.front;	
}

void Orientation::yaw(float angle) {

    float radian = angle * PI_OVER_180;

    if (USING_QUATERNIONS) {
        rotateAndGenerateDirections(glm::quat(glm::vec3(0.0f, -radian, 0.0f)));
    } else {    
        float s = sin(radian);
        float c = cos(radian);
            
        glm::vec3 cosineFront = front * c;
        glm::vec3 cosineRight = right * c;
        glm::vec3 sineFront   = front * s;
        glm::vec3 sineRight   = right * s;
        
        front = cosineFront - sineRight;
        right = cosineRight + sineFront;	
    }
}

void Orientation::pitch(float angle) {

    float radian = angle * PI_OVER_180;

    if (USING_QUATERNIONS) {
        rotateAndGenerateDirections(glm::quat(glm::vec3(radian, 0.0f, 0.0f)));
    } else {    
        float s = sin(radian);
        float c = cos(radian);
        
        glm::vec3 cosineUp    = up    * c;
        glm::vec3 cosineFront = front * c;
        glm::vec3 sineUp      = up    * s;
        glm::vec3 sineFront   = front * s;
        
        up    = cosineUp    - sineFront;
        front = cosineFront + sineUp;
    }
}

void Orientation::roll(float angle) {

    float radian = angle * PI_OVER_180;

    if (USING_QUATERNIONS) {
        rotateAndGenerateDirections(glm::quat(glm::vec3(0.0f, 0.0f, radian)));
    } else {    
        float s = sin(radian);
        float c = cos(radian);
            
        glm::vec3 cosineUp    = up    * c;
        glm::vec3 cosineRight = right * c;
        glm::vec3 sineUp      = up    * s;
        glm::vec3 sineRight   = right * s;
        
        up    = cosineUp    - sineRight;
        right = cosineRight + sineUp;	
    }
}

void Orientation::rotate(float pitch_change, float yaw_change, float roll_change) {
    pitch(pitch_change);
    yaw  (yaw_change);
    roll (roll_change);
}

void Orientation::rotate(glm::vec3 eulerAngles) {

//this needs to be optimized!
    pitch(eulerAngles.x);
    yaw  (eulerAngles.y);
    roll (eulerAngles.z);
}

void Orientation::rotate( glm::quat rotation ) {
    rotateAndGenerateDirections(rotation);
}


void Orientation::rotateAndGenerateDirections(glm::quat rotation) {

    quat = quat * rotation;
    
    glm::mat4 rotationMatrix = glm::mat4_cast(quat);
    
    right = glm::vec3(glm::vec4(IDENTITY_RIGHT, 0.0f) * rotationMatrix);
    up    = glm::vec3(glm::vec4(IDENTITY_UP,    0.0f) * rotationMatrix);
    front = glm::vec3(glm::vec4(IDENTITY_FRONT, 0.0f) * rotationMatrix);
}
