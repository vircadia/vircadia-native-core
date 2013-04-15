//
//  ViewFrustum.h
//  hifi
//
//  Created by Brad Hefta-Gaub on 04/11/13.
//
//  Simple view frustum class.
//
//

#ifndef __hifi__ViewFrustum__
#define __hifi__ViewFrustum__

#include <glm/glm.hpp>

class ViewFrustum {
private:
	glm::vec3 _position;
	glm::vec3 _direction;
	glm::vec3 _up;
	glm::vec3 _right;
	float _screenWidth;
	float _screenHeight;

	float _nearDist; 
	float _farDist;
	
	float _nearHeight;
	float _nearWidth;
	float _farHeight;
	float _farWidth;

	glm::vec3 _farCenter;
	glm::vec3 _farTopLeft;      
	glm::vec3 _farTopRight;     
	glm::vec3 _farBottomLeft;   
	glm::vec3 _farBottomRight;  

	glm::vec3 _nearCenter; 
	glm::vec3 _nearTopLeft;     
	glm::vec3 _nearTopRight;    
	glm::vec3 _nearBottomLeft;  
	glm::vec3 _nearBottomRight; 
public:
	const glm::vec3& getFarCenter()      const { return _farCenter; };
	const glm::vec3& getFarTopLeft()     const { return _farTopLeft; };  
	const glm::vec3& getFarTopRight()    const { return _farTopRight; };
	const glm::vec3& getFarBottomLeft()  const { return _farBottomLeft; };
	const glm::vec3& getFarBottomRight() const { return _farBottomRight; };

	const glm::vec3& getNearCenter()      const { return _nearCenter; };
	const glm::vec3& getNearTopLeft()     const { return _nearTopLeft; };  
	const glm::vec3& getNearTopRight()    const { return _nearTopRight; };
	const glm::vec3& getNearBottomLeft()  const { return _nearBottomLeft; };
	const glm::vec3& getNearBottomRight() const { return _nearBottomRight; };

	void calculateViewFrustum(glm::vec3 position, glm::vec3 direction, 
		glm::vec3 up, glm::vec3 right, float screenWidth, float screenHeight);

	ViewFrustum(glm::vec3 position, glm::vec3 direction, 
		glm::vec3 up, glm::vec3 right, float screenWidth, float screenHeight);
	
	void dump();
	
	static float fovAngleAdust;
};


#endif /* defined(__hifi__ViewFrustum__) */
