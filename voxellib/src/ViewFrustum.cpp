//
//  ViewFrustum.cpp
//  hifi
//
//  Created by Brad Hefta-Gaub on 04/11/13.
//
//  Simple view frustum class.
//
//

#if 0

#include "Vector3D.h"
#include "Orientation.h"
#include "ViewFrustum.h"

ViewFrustum::ViewFrustum(Vector3D position, Orientation direction) {
	this->calculateViewFrustum(position, direction);
}

/////////////////////////////////////////////////////////////////////////////////////
// render_view_frustum()
//
// Description: this will render the view frustum bounds for EITHER the head
// 				or the "myCamera". It appears as if the orientation that comes
//				from these two sources is in different coordinate spaces (namely)
// 				their axis orientations don't match.
void ViewFrustum::calculateViewFrustum(Vector3D position, Orientation direction) {
	float nearDist = 0.1; 
	float farDist  = 1.0;
	
	Vector3D front = direction.getFront();
	Vector3D up    = direction.getUp();
	Vector3D right = direction.getRight();
	
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

#endif


