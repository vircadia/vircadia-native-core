//
//  ViewFrustum.cpp
//  hifi
//
//  Created by Brad Hefta-Gaub on 04/11/13.
//
//  Simple view frustum class.
//
//

#include "ViewFrustum.h"
#include "glmUtils.h"

ViewFrustum::ViewFrustum(glm::vec3 position, glm::vec3 direction) {
	this->calculateViewFrustum(position, direction);
}

/////////////////////////////////////////////////////////////////////////////////////
// ViewFrustum::calculateViewFrustum()
//
// Description: this will calculate the view frustum bounds for a given position
// 				and direction
void ViewFrustum::calculateViewFrustum(glm::vec3 position, glm::vec3 direction) {
	float nearDist = 0.1; 
	float farDist  = 1.0;
	
	glm::vec3 front = direction;
	glm::vec3 up    = glm::vec3(0,direction.y,0); // up is always this way
	glm::vec3 right = glm::vec3(direction.z,direction.y,direction.x); // up is
	
	float fovHalfAngle = 0.7854f; // 45 deg for half, so fov = 90 deg
	
	float screenWidth = 800;//::WIDTH; // These values come from reshape()
	float screenHeight = 600; //::HEIGHT;
	float ratio      = screenWidth/screenHeight;
	
	float nearHeight = 2 * tan(fovHalfAngle) * nearDist;
	float nearWidth  = nearHeight * ratio;
	float farHeight  = 2 * tan(fovHalfAngle) * farDist;
	float farWidth   = farHeight * ratio;
	
	this->_farCenter       = position+front*farDist;
	this->_farTopLeft      = this->_farCenter  + (up*farHeight*0.5)  - (right*farWidth*0.5); 
	this->_farTopRight     = this->_farCenter  + (up*farHeight*0.5)  + (right*farWidth*0.5); 
	this->_farBottomLeft   = this->_farCenter  - (up*farHeight*0.5)  - (right*farWidth*0.5); 
	this->_farBottomRight  = this->_farCenter  - (up*farHeight*0.5)  + (right*farWidth*0.5); 

	this->_nearCenter      = position+front*nearDist;
	this->_nearTopLeft     = this->_nearCenter + (up*nearHeight*0.5) - (right*nearWidth*0.5); 
	this->_nearTopRight    = this->_nearCenter + (up*nearHeight*0.5) + (right*nearWidth*0.5); 
	this->_nearBottomLeft  = this->_nearCenter - (up*nearHeight*0.5) - (right*nearWidth*0.5); 
	this->_nearBottomRight = this->_nearCenter - (up*nearHeight*0.5) + (right*nearWidth*0.5); 
}

void ViewFrustum::dump() {
    printf("farCenter.x=%f, farCenter.y=%f, farCenter.z=%f\n",this->_farCenter.x,this->_farCenter.y,this->_farCenter.z);
    printf("farTopLeft.x=%f, farTopLeft.y=%f, farTopLeft.z=%f\n",this->_farTopLeft.x,this->_farTopLeft.y,this->_farTopLeft.z);
    printf("farTopRight.x=%f, farTopRight.y=%f, farTopRight.z=%f\n",this->_farTopRight.x,this->_farTopRight.y,this->_farTopRight.z);
    printf("farBottomLeft.x=%f, farBottomLeft.y=%f, farBottomLeft.z=%f\n",this->_farBottomLeft.x,this->_farBottomLeft.y,this->_farBottomLeft.z);
    printf("farBottomRight.x=%f, farBottomRight.y=%f, farBottomRight.z=%f\n",this->_farBottomRight.x,this->_farBottomRight.y,this->_farBottomRight.z);

    printf("nearCenter.x=%f, nearCenter.y=%f, nearCenter.z=%f\n",this->_nearCenter.x,this->_nearCenter.y,this->_nearCenter.z);
    printf("nearTopLeft.x=%f, nearTopLeft.y=%f, nearTopLeft.z=%f\n",this->_nearTopLeft.x,this->_nearTopLeft.y,this->_nearTopLeft.z);
    printf("nearTopRight.x=%f, nearTopRight.y=%f, nearTopRight.z=%f\n",this->_nearTopRight.x,this->_nearTopRight.y,this->_nearTopRight.z);
    printf("nearBottomLeft.x=%f, nearBottomLeft.y=%f, nearBottomLeft.z=%f\n",this->_nearBottomLeft.x,this->_nearBottomLeft.y,this->_nearBottomLeft.z);
    printf("nearBottomRight.x=%f, nearBottomRight.y=%f, nearBottomRight.z=%f\n",this->_nearBottomRight.x,this->_nearBottomRight.y,this->_nearBottomRight.z);
}


