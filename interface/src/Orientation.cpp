#include "Orientation.h"
#include "Util.h"


Orientation::Orientation() {
	right	= glm::vec3(  1.0,  0.0,  0.0 );
	up		= glm::vec3(  0.0,  1.0,  0.0 );
	front	= glm::vec3(  0.0,  0.0,  1.0 );	
}


void Orientation::setToIdentity() {
	right	= glm::vec3(  1.0,  0.0,  0.0 );
	up		= glm::vec3(  0.0,  1.0,  0.0 );
	front	= glm::vec3(  0.0,  0.0,  1.0 );	
}


void Orientation::set( Orientation o ) { 
	right	= o.getRight();
	up		= o.getUp();
	front	= o.getFront();	
}


void Orientation::yaw( float angle ) {
	float r = angle * PI_OVER_180;
	float s = sin(r);
	float c = cos(r);
	
	glm::vec3 cosineFront	= front * c;
	glm::vec3 cosineRight	= right * c;
	glm::vec3 sineFront		= front * s;
	glm::vec3 sineRight		= right * s;
	
	front	= cosineFront + sineRight;
	right	= cosineRight - sineFront;	
}


void Orientation::pitch( float angle ) {
	float r = angle * PI_OVER_180;
	float s = sin(r);
	float c = cos(r);
	
	glm::vec3 cosineUp		= up	* c;
	glm::vec3 cosineFront	= front	* c;
	glm::vec3 sineUp		= up	* s;
	glm::vec3 sineFront		= front * s;
	
	up		= cosineUp		+ sineFront;
	front	= cosineFront	- sineUp;
}


void Orientation::roll( float angle ) {
	float r = angle * PI_OVER_180;
	float s = sin(r);
	float c = cos(r);
	
	glm::vec3 cosineUp		= up	* c;
	glm::vec3 cosineRight	= right	* c;
	glm::vec3 sineUp		= up	* s;
	glm::vec3 sineRight		= right	* s;
	
	up		= cosineUp		+ sineRight;
	right	= cosineRight	- sineUp;	
}


void Orientation::setRightUpFront( const glm::vec3 &r, const glm::vec3 &u, const glm::vec3 &f ) {
	right	= r;
	up		= u;
	front	= f;
}
