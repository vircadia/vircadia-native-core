//---------------------------------------------------------------------
//
// Created by Jeffrey Ventrella for High Fidelity.
// Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//---------------------------------------------------------------------

#include <SharedUtil.h>
#include <VoxelConstants.h>
// #include "Log.h"

#include "Camera.h"

Camera::Camera() {
    _frustumNeedsReshape  = false;
	_mode			= CAMERA_MODE_THIRD_PERSON;
	_tightness		= 10.0; // default
	_fieldOfView    = 60.0; // default
	_nearClip       = 0.08; // default
	_farClip        = 50.0 * TREE_SCALE; // default
    _modeShift      = 0.0;
	_yaw            = 0.0;
	_pitch			= 0.0;
	_roll			= 0.0;
	_upShift		= 0.0;
	_rightShift		= 0.0;
	_distance		= 0.0;
	_idealYaw		= 0.0;
	_targetPosition	= glm::vec3(0.0, 0.0, 0.0);
	_position		= glm::vec3(0.0, 0.0, 0.0);
	_idealPosition	= glm::vec3(0.0, 0.0, 0.0);
	_orientation.setToIdentity();
}


void Camera::update(float deltaTime)  {

    if (_mode == CAMERA_MODE_NULL) {
        _modeShift = 0.0;
    } else {
        // use iterative forces to push the camera towards the desired position and angle
        updateFollowMode(deltaTime);
        
        if (_modeShift < 1.0f) {
            _modeShift += MODE_SHIFT_RATE * deltaTime;
            if (_modeShift > 1.0f) {
                _modeShift = 1.0f;
            }
        }
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

    // update _yaw (before position!) 
    _yaw += (_idealYaw - _yaw) * t;
    _orientation.yaw(_yaw);
            
    float radian = (_yaw / 180.0) * PIE;

    // update _position
    double x = -_distance * sin(radian);
    double z = -_distance * cos(radian);
    double y = _upShift; 
        
    _idealPosition = _targetPosition + glm::vec3(x, y, z);
    
    // pull position towards ideal position
    _position += (_idealPosition - _position) * t; 
}

void Camera::setMode(CameraMode  m) { 
    _mode = m;
    _modeShift = 0.0f; 
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

// call to find out if the view frustum needs to be reshaped
bool Camera::getFrustumNeedsReshape() {
    return _frustumNeedsReshape;
}

// call this after reshaping the view frustum
void Camera::setFrustumWasReshaped() {
    _frustumNeedsReshape = false;
}



