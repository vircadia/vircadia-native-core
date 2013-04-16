//---------------------------------------------------------------------
//
// Created by Jeffrey Ventrella for High Fidelity.
// Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//---------------------------------------------------------------------

#include <SharedUtil.h>

#include "Camera.h"

Camera::Camera()
{
	_mode			= CAMERA_MODE_THIRD_PERSON;
	_tightness		= DEFAULT_CAMERA_TIGHTNESS;
	_fieldOfView	= 60.0; // default
	_yaw			= 0.0;
	_pitch			= 0.0;
	_roll			= 0.0;
	_up				= 0.0;
	_distance		= 0.0;
	_idealYaw		= 0.0;
	_targetPosition	= glm::vec3( 0.0, 0.0, 0.0 );
	_position		= glm::vec3( 0.0, 0.0, 0.0 );
	_idealPosition	= glm::vec3( 0.0, 0.0, 0.0 );
	_orientation.setToIdentity();
}

void Camera::update( float deltaTime )
{
	float radian = ( _yaw / 180.0 ) * PIE;

	//these need to be checked to make sure they correspond to the coordinate system.
	double x = _distance * -sin( radian );
	double z = _distance *  cos( radian );
	double y = _up;
		
	_idealPosition = _targetPosition + glm::vec3( x, y, z );
	float t = _tightness * deltaTime;
	
	if ( t > 1.0 ){
		t = 1.0;
	}
	
	_position += ( _idealPosition - _position ) * t; 
	_yaw      += ( _idealYaw      - _yaw      ) * t;
	
	//roll = 20.0;
	
	//-------------------------------------------------------------------------
	// generate the ortho-normals for the orientation based on the Euler angles
	//-------------------------------------------------------------------------
	_orientation.setToIdentity();
	_orientation.yaw	( _yaw	 );
	_orientation.pitch	( _pitch );
	_orientation.roll	( _roll	 );
}

