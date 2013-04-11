//-----------------------------------------------------------
//
// Created by Jeffrey Ventrella and added as a utility 
// class for High Fidelity Code base, April 2013
//
//-----------------------------------------------------------

#ifndef __interface__orientation__
#define __interface__orientation__

#include <cmath> // with this work? "Math.h"
#include <glm/glm.hpp>

enum Axis
{
	ORIENTATION_RIGHT_AXIS,
	ORIENTATION_UP_AXIS,
	ORIENTATION_FRONT_AXIS
};

class Orientation
{
private:
	
	glm::vec3 right;
	glm::vec3 up;
	glm::vec3 front;
	
	//void verifyValidOrientation();
	
public:
	Orientation();
	
	void yaw	( float );
	void pitch	( float );
	void roll	( float );

	void set( Orientation );
	void setToIdentity();

	glm::vec3 getRight()	{ return right;	}
	glm::vec3 getUp()		{ return up;	}
	glm::vec3 getFront()	{ return front;	}
	
	void setRightUpFront( const glm::vec3 &, const glm::vec3 &, const glm::vec3 & );
};


#endif
