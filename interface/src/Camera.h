//-----------------------------------------------------------
//
// Created by Jeffrey Ventrella and added as a utility 
// class for High Fidelity Code base, April 2013
//
//-----------------------------------------------------------

#ifndef __interface__camera__
#define __interface__camera__

#include "Vector3D.h"
#include "Orientation.h"
#include <glm/glm.hpp>

class Camera
{
public:
	Camera();
	
	void update();
	
	void setYaw				( double		y ) { yaw				= y; }
	void setPitch			( double		p ) { pitch				= p; }
	void setRoll			( double		r ) { roll				= r; }
	void setUp				( double		u ) { up				= u; }
	void setDistance		( double		d ) { distance			= d; }
	void setTargetPosition	( glm::dvec3	t ) { targetPosition	= t; };
	void setPosition		( glm::dvec3	p ) { position			= p; };
	void setOrientation		( Orientation	o ) { orientation.set(o); }

	double		getYaw			() { return yaw;			}
	double		getPitch		() { return pitch;			}
	double		getRoll			() { return roll;			}
	glm::dvec3	getPosition		() { return position;		}
	Orientation	getOrientation	() { return orientation;	}

private:

	glm::dvec3	position;
	glm::dvec3	targetPosition;
	double		yaw;
	double		pitch;
	double		roll;
	double		up;
	double		distance;
	Orientation	orientation;
};

#endif
