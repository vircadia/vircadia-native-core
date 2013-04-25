//---------------------------------------------------------------------
//
// Created by Jeffrey Ventrella for High Fidelity.
// Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//---------------------------------------------------------------------

#include <SharedUtil.h>
// #include "Log.h"

#include "Camera.h"

Camera::Camera() {
    _frustumNeedsReshape = false;
	_mode			     = CAMERA_MODE_THIRD_PERSON;
	_tightness		     = 10.0; // default
	_fieldOfView         = 60.0; // default
	_nearClip            = 0.08; // default
	_farClip             = 50.0; // default
	_yaw                 = 0.0;
	_pitch			     = 0.0;
	_roll			     = 0.0;
	_upShift		     = 0.0;
	_rightShift		     = 0.0;
	_distance		     = 0.0;
	_idealYaw		     = 0.0;
	_targetPosition	     = glm::vec3( 0.0, 0.0, 0.0 );
	_position		     = glm::vec3( 0.0, 0.0, 0.0 );
	_idealPosition	     = glm::vec3( 0.0, 0.0, 0.0 );
	_orientation.setToIdentity();
}

void Camera::update( float deltaTime )  {

	// derive t from tightness
	float t = _tightness * deltaTime;	
	if ( t > 1.0 ) {
		t = 1.0;
	}

	// update _yaw (before position!) 
	_yaw += ( _idealYaw - _yaw ) * t;
    
	// generate the ortho-normals for the orientation based on the Euler angles
	_orientation.setToIdentity();
	_orientation.yaw  ( _yaw   );
	_orientation.pitch( _pitch );
	_orientation.roll ( _roll  );
    
	float radian = ( _yaw / 180.0 ) * PIE;

	// update _position
	//these need to be checked to make sure they correspond to the correct coordinate system.
	double x = _distance * -sin( radian );
	double z = _distance *  cos( radian );
	double y = _upShift; 
		
	_idealPosition  = _targetPosition + glm::vec3( x, y, z );
    //_idealPosition += _orientation.getRight() * _rightShift;
    //_idealPosition += _orientation.getUp   () * _upShift;
	
    // pull position towards ideal position
	_position += ( _idealPosition - _position ) * t; 
}

