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
	glm::vec3 getFarCenter()      const { return _farCenter; };
	glm::vec3 getFarTopLeft()     const { return _farTopLeft; };  
	glm::vec3 getFarTopRight()    const { return _farTopRight; };
	glm::vec3 getFarBottomLeft()  const { return _farBottomLeft; };
	glm::vec3 getFarBottomRight() const { return _farBottomRight; };

	glm::vec3 getNearCenter()      const { return _nearCenter; };
	glm::vec3 getNearTopLeft()     const { return _nearTopLeft; };  
	glm::vec3 getNearTopRight()    const { return _nearTopRight; };
	glm::vec3 getNearBottomLeft()  const { return _nearBottomLeft; };
	glm::vec3 getNearBottomRight() const { return _nearBottomRight; };

	void calculateViewFrustum(glm::vec3 position, glm::vec3 direction);

	ViewFrustum(glm::vec3 position, glm::vec3 direction);
	
	void dump();
};


#endif /* defined(__hifi__ViewFrustum__) */
