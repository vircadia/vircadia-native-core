//-----------------------------------------------------------
//
// Created by Jeffrey Ventrella and added as a utility 
// class for High Fidelity Code base, April 2013
//
//-----------------------------------------------------------

#include "Camera.h"

//------------------------
Camera::Camera()
{
	yaw			= 0.0;
	pitch		= 0.0;
	roll		= 0.0;
	position	= glm::dvec3( 0.0, 0.0, 0.0 );
	orientation.setToIdentity();
}

