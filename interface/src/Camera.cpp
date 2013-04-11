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
	mode			= CAMERA_MODE_THIRD_PERSON;
	fieldOfView		= 60.0; // default
	yaw				= 0.0;
	pitch			= 0.0;
	roll			= 0.0;
	up				= 0.0;
	distance		= 0.0;
	targetPosition	= glm::vec3( 0.0, 0.0, 0.0 );
	position		= glm::vec3( 0.0, 0.0, 0.0 );
	orientation.setToIdentity();
}



//------------------------
void Camera::update()
{
	double radian = ( yaw / 180.0 ) * PIE;

	double x = distance * -sin( radian );
	double z = distance * cos( radian );
	double y = up;
	
	position = targetPosition + glm::vec3( x, y, z );
	
	//------------------------------------------------------------------------
	//geterate the ortho-normals for the orientation based on the Euler angles
	//------------------------------------------------------------------------
	orientation.setToIdentity();
	orientation.yaw		( yaw	);
	orientation.pitch	( pitch	);
	orientation.roll	( roll	);
}

