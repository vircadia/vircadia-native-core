//-----------------------------------------------------------
//
// Created by Jeffrey Ventrella and added as a utility 
// class for High Fidelity Code base, April 2013
//
//-----------------------------------------------------------

#include "Camera.h"
#include "Util.h"

//------------------------
Camera::Camera()
{
	yaw				= 0.0;
	pitch			= 0.0;
	roll			= 0.0;
	up				= 0.0;
	distance		= 0.0;
	targetPosition	= glm::dvec3( 0.0, 0.0, 0.0 );
	position		= glm::dvec3( 0.0, 0.0, 0.0 );
	orientation.setToIdentity();
}


//------------------------
void Camera::update()
{
	double radian = ( yaw / 180.0 ) * PIE;

	double x = distance *  sin( radian );
	double z = distance * -cos( radian );
	double y = -up;
	
	position = glm::dvec3( targetPosition );	
	position += glm::dvec3( x, y, z );
}

