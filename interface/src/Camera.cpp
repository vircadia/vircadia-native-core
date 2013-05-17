//---------------------------------------------------------------------
//
// Created by Jeffrey Ventrella for High Fidelity.
// Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//---------------------------------------------------------------------

#include <SharedUtil.h>
#include <VoxelConstants.h>
#include <OculusManager.h>
// #include "Log.h"

#include "Camera.h"

const float MODE_SHIFT_RATE = 5.0f;

Camera::Camera() {

    _needsToInitialize   = true;
    _frustumNeedsReshape = true;
    
    _mode           = CAMERA_MODE_THIRD_PERSON;
    _tightness      = 10.0; // default
    _fieldOfView    = 60.0; // default
    _nearClip       = 0.08; // default
    _farClip        = 50.0 * TREE_SCALE; // default
    _yaw            = 0.0;
    _pitch          = 0.0;
    _roll           = 0.0;
    _upShift        = 0.0;
    _distance       = 0.0;
    _idealYaw       = 0.0;
    _idealPitch     = 0.0;
    _idealRoll      = 0.0;
    _targetPosition = glm::vec3(0.0, 0.0, 0.0);
    _position       = glm::vec3(0.0, 0.0, 0.0);
    _idealPosition  = glm::vec3(0.0, 0.0, 0.0);
    _orientation.setToIdentity();
    
    for (int m = 0; m < NUM_CAMERA_MODES; m ++) {
        _attributes[m].upShift   = 0.0f; 
        _attributes[m].distance  = 0.0f; 
        _attributes[m].tightness = 0.0f; 
    }
}


void Camera::update(float deltaTime)  {

    if (_mode != CAMERA_MODE_NULL) {
        // use iterative forces to push the camera towards the target position and angle
        updateFollowMode(deltaTime);
     }
    
    // do this AFTER making any changes to yaw pitch and roll....
    generateOrientation();    
}


// generate the ortho-normals for the orientation based on the three Euler angles
void Camera::generateOrientation() {
    _orientation.setToIdentity();
    _orientation.pitch(_pitch);
    _orientation.yaw  (_yaw  );
    _orientation.roll (_roll );    
}

// use iterative forces to keep the camera at the desired position and angle
void Camera::updateFollowMode(float deltaTime) {  

    // derive t from tightness
    float t = _tightness * deltaTime;	
    if (t > 1.0) {
        t = 1.0;
    }
    
    // update Euler angles (before position!)
    if (_needsToInitialize || OculusManager::isConnected()) {
        _yaw   = _idealYaw;
        _pitch = _idealPitch;
        _roll  = _idealRoll;
    } else {
        // pull Euler angles towards ideal Euler angles
        _yaw   += (_idealYaw   - _yaw  ) * t;
        _pitch += (_idealPitch - _pitch) * t;
        _roll  += (_idealRoll  - _roll ) * t;
    }
    
    _orientation.yaw  (_yaw  );
    _orientation.pitch(_pitch);
    _orientation.roll (_roll );
            
    float radian = (_yaw / 180.0) * PIE;

    // update _position
    double x = -_distance * sin(radian);
    double z = -_distance * cos(radian);
    double y = _upShift; 
        
    _idealPosition = _targetPosition + glm::vec3(x, y, z);
    
    if (_needsToInitialize) {
        _position = _idealPosition; 
        _needsToInitialize = false;
    } else {
        // force position towards ideal position
        _position += (_idealPosition - _position) * t; 
    }
    
    //transition to the attributes of the current mode
    _upShift   += (_attributes[_mode].upShift   - _upShift  ) * deltaTime * MODE_SHIFT_RATE;
    _distance  += (_attributes[_mode].distance  - _distance ) * deltaTime * MODE_SHIFT_RATE;
    _tightness += (_attributes[_mode].tightness - _tightness) * deltaTime * MODE_SHIFT_RATE;
}

void Camera::setMode(CameraMode  m, CameraFollowingAttributes a) { 
 
    _attributes[m].upShift   = a.upShift;
    _attributes[m].distance  = a.distance;
    _attributes[m].tightness = a.tightness;
    
    setMode(m);
}

void Camera::setMode(CameraMode  m) { 
    _mode = m;
    _needsToInitialize = true;
}

void Camera::setTargetRotation( float yaw, float pitch, float roll ) {
    _idealYaw   = yaw;
    _idealPitch = pitch;
    _idealRoll  = roll;
}

void Camera::setFieldOfView(float f) { 
    _fieldOfView = f; 
    _frustumNeedsReshape = true; 
}

void Camera::setAspectRatio(float a) { 
    _aspectRatio = a; 
    _frustumNeedsReshape = true; 
}

void Camera::setNearClip   (float n) { 
    _nearClip = n; 
    _frustumNeedsReshape = true; 
}

void Camera::setFarClip    (float f) { 
    _farClip = f; 
    _frustumNeedsReshape = true; 
}

void Camera::setEyeOffsetPosition  (const glm::vec3& p) {
    _eyeOffsetPosition = p;
    _frustumNeedsReshape = true;
}

void Camera::setEyeOffsetOrientation  (const glm::quat& o) {
    _eyeOffsetOrientation = o;
    _frustumNeedsReshape = true;
}

void Camera::initialize() {
    _needsToInitialize = true;
}

// call to find out if the view frustum needs to be reshaped
bool Camera::getFrustumNeedsReshape() {
    return _frustumNeedsReshape;
}

// call this after reshaping the view frustum
void Camera::setFrustumWasReshaped() {
    _frustumNeedsReshape = false;
}



