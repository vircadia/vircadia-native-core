//---------------------------------------------------------------------
//
// Created by Jeffrey Ventrella for High Fidelity.
// Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//---------------------------------------------------------------------

#include "Camera.h"
#include "Util.h"

//------------------------
Camera::Camera()
{
	_mode			= CAMERA_MODE_THIRD_PERSON;
	_fieldOfView    = 60.0;     // default
	_nearClip       = 0.1;      // default
	_farClip        = 50.0;    // default
	_yaw            = 0.0;
	_pitch			= 0.0;
	_roll			= 0.0;
	_up				= 0.0;
	_distance		= 0.0;
	_targetPosition	= glm::vec3( 0.0, 0.0, 0.0 );
	_position		= glm::vec3( 0.0, 0.0, 0.0 );
	_orientation.setToIdentity();
}



//------------------------
void Camera::update()
{
	float radian = ( _yaw / 180.0 ) * PIE;

	//these need to be checked to make sure they correspond to the cordinate system.
	float x = _distance * -sin( radian );
	float z = _distance *  cos( radian );
	float y = _up;
	
	_position = _targetPosition + glm::vec3( x, y, z );
	
	//------------------------------------------------------------------------
	//geterate the ortho-normals for the orientation based on the Euler angles
	//------------------------------------------------------------------------
	_orientation.setToIdentity();
	_orientation.yaw    ( _yaw	);
	_orientation.pitch	( _pitch	);
	_orientation.roll	( _roll	);
}

