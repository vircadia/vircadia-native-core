//-----------------------------------------------------------
//
// Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//-----------------------------------------------------------

#ifndef __interface__orientation__
#define __interface__orientation__

#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

// this is where the coordinate system is represented
const glm::vec3 IDENTITY_RIGHT = glm::vec3(-1.0f, 0.0f, 0.0f);
const glm::vec3 IDENTITY_UP    = glm::vec3( 0.0f, 1.0f, 0.0f);
const glm::vec3 IDENTITY_FRONT = glm::vec3( 0.0f, 0.0f, 1.0f);

class Orientation
{
public:
	Orientation();
	
	void set(Orientation);
	void setToIdentity();

	void pitch(float p);
	void yaw  (float y);
	void roll (float r);
    
    void rotate(float pitch, float yaw, float roll);
    void rotate(glm::vec3 EulerAngles);
    void rotate(glm::quat quaternion);

	const glm::vec3 & getRight() const {return  right;}
	const glm::vec3 & getUp   () const {return  up;   }
	const glm::vec3 & getFront() const {return  front;}

	const glm::vec3 & getIdentityRight() const {return IDENTITY_RIGHT;}
	const glm::vec3 & getIdentityUp   () const {return IDENTITY_UP;}
	const glm::vec3 & getIdentityFront() const {return IDENTITY_FRONT;}

private:

    glm::quat quat;
    glm::vec3 right;
	glm::vec3 up;
	glm::vec3 front;
    
    void rotateAndGenerateDirections(glm::quat rotation);
};

#endif
