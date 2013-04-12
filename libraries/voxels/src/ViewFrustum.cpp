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

ViewFrustum::ViewFrustum(glm::vec3 position, glm::vec3 direction, glm::vec3 up, glm::vec3 right, float screenWidth, float screenHeight) {
	this->calculateViewFrustum(position, direction, up, right, screenWidth, screenHeight);
}

/////////////////////////////////////////////////////////////////////////////////////
// ViewFrustum::calculateViewFrustum()
//
// Description: this will calculate the view frustum bounds for a given position
// 				and direction
//
// Notes on how/why this works: 
//     http://www.lighthouse3d.com/tutorials/view-frustum-culling/view-frustums-shape/
//
void ViewFrustum::calculateViewFrustum(glm::vec3 position, glm::vec3 direction, glm::vec3 up, glm::vec3 right, float screenWidth, float screenHeight) {
	
	// Save the values we were passed...
	this->_position=position;
	this->_direction=direction;
	this->_up=up;
	this->_right=right;
	this->_screenWidth=screenWidth;
	this->_screenHeight=screenHeight;
	
	glm::vec3 front    = direction;
	float fovHalfAngle = 0.7854f*1.5; // 45 deg for half, so fov = 90 deg
	float ratio        = screenWidth/screenHeight;
	
	this->_nearDist = 0.1;
	this->_farDist  = 10.0;
	
	this->_nearHeight = 2 * tan(fovHalfAngle) * this->_nearDist;
	this->_nearWidth  = this->_nearHeight * ratio;
	this->_farHeight  = 2 * tan(fovHalfAngle) * this->_farDist;
	this->_farWidth   = this->_farHeight * ratio;

	this->_farCenter       = this->_position+front*this->_farDist;
	this->_farTopLeft      = this->_farCenter  + (this->_up*this->_farHeight*0.5)  - (this->_right*this->_farWidth*0.5); 
	this->_farTopRight     = this->_farCenter  + (this->_up*this->_farHeight*0.5)  + (this->_right*this->_farWidth*0.5); 
	this->_farBottomLeft   = this->_farCenter  - (this->_up*this->_farHeight*0.5)  - (this->_right*this->_farWidth*0.5); 
	this->_farBottomRight  = this->_farCenter  - (this->_up*this->_farHeight*0.5)  + (this->_right*this->_farWidth*0.5); 

	this->_nearCenter      = this->_position+front*this->_nearDist;
	this->_nearTopLeft     = this->_nearCenter + (this->_up*this->_nearHeight*0.5) - (this->_right*this->_nearWidth*0.5); 
	this->_nearTopRight    = this->_nearCenter + (this->_up*this->_nearHeight*0.5) + (this->_right*this->_nearWidth*0.5); 
	this->_nearBottomLeft  = this->_nearCenter - (this->_up*this->_nearHeight*0.5) - (this->_right*this->_nearWidth*0.5); 
	this->_nearBottomRight = this->_nearCenter - (this->_up*this->_nearHeight*0.5) + (this->_right*this->_nearWidth*0.5); 
}

void ViewFrustum::dump() {

    printf("position.x=%f, position.y=%f, position.z=%f\n",this->_position.x,this->_position.y,this->_position.z);
    printf("direction.x=%f, direction.y=%f, direction.z=%f\n",this->_direction.x,this->_direction.y,this->_direction.z);
    printf("up.x=%f, up.y=%f, up.z=%f\n",this->_up.x,this->_up.y,this->_up.z);
    printf("right.x=%f, right.y=%f, right.z=%f\n",this->_right.x,this->_right.y,this->_right.z);

    printf("farDist=%f\n",this->_farDist);
    printf("farHeight=%f\n",this->_farHeight);
    printf("farWidth=%f\n",this->_farWidth);

    printf("nearDist=%f\n",this->_nearDist);
    printf("nearHeight=%f\n",this->_nearHeight);
    printf("nearWidth=%f\n",this->_nearWidth);

    printf("farCenter.x=%f,      farCenter.y=%f,      farCenter.z=%f\n",this->_farCenter.x,this->_farCenter.y,this->_farCenter.z);
    printf("farTopLeft.x=%f,     farTopLeft.y=%f,     farTopLeft.z=%f\n",this->_farTopLeft.x,this->_farTopLeft.y,this->_farTopLeft.z);
    printf("farTopRight.x=%f,    farTopRight.y=%f,    farTopRight.z=%f\n",this->_farTopRight.x,this->_farTopRight.y,this->_farTopRight.z);
    printf("farBottomLeft.x=%f,  farBottomLeft.y=%f,  farBottomLeft.z=%f\n",this->_farBottomLeft.x,this->_farBottomLeft.y,this->_farBottomLeft.z);
    printf("farBottomRight.x=%f, farBottomRight.y=%f, farBottomRight.z=%f\n",this->_farBottomRight.x,this->_farBottomRight.y,this->_farBottomRight.z);

    printf("nearCenter.x=%f,      nearCenter.y=%f,      nearCenter.z=%f\n",this->_nearCenter.x,this->_nearCenter.y,this->_nearCenter.z);
    printf("nearTopLeft.x=%f,     nearTopLeft.y=%f,     nearTopLeft.z=%f\n",this->_nearTopLeft.x,this->_nearTopLeft.y,this->_nearTopLeft.z);
    printf("nearTopRight.x=%f,    nearTopRight.y=%f,    nearTopRight.z=%f\n",this->_nearTopRight.x,this->_nearTopRight.y,this->_nearTopRight.z);
    printf("nearBottomLeft.x=%f,  nearBottomLeft.y=%f,  nearBottomLeft.z=%f\n",this->_nearBottomLeft.x,this->_nearBottomLeft.y,this->_nearBottomLeft.z);
    printf("nearBottomRight.x=%f, nearBottomRight.y=%f, nearBottomRight.z=%f\n",this->_nearBottomRight.x,this->_nearBottomRight.y,this->_nearBottomRight.z);
}


