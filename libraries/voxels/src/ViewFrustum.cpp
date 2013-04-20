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
#include "voxels_Log.h"

using voxels_lib::printLog;

ViewFrustum::ViewFrustum() :
    _position(glm::vec3(0,0,0)),
    _direction(glm::vec3(0,0,0)),
    _up(glm::vec3(0,0,0)),
    _right(glm::vec3(0,0,0)),
    _fieldOfView(0.0),
    _aspectRatio(1.0),
    _nearClip(0.1),
    _farClip(500.0),
    _nearHeight(0.0),
    _nearWidth(0.0),
    _farHeight(0.0),
    _farWidth(0.0),
    _farCenter(glm::vec3(0,0,0)),
    _farTopLeft(glm::vec3(0,0,0)),
    _farTopRight(glm::vec3(0,0,0)),
    _farBottomLeft(glm::vec3(0,0,0)),
    _farBottomRight(glm::vec3(0,0,0)),
    _nearCenter(glm::vec3(0,0,0)),
    _nearTopLeft(glm::vec3(0,0,0)),
    _nearTopRight(glm::vec3(0,0,0)),
    _nearBottomLeft(glm::vec3(0,0,0)),
    _nearBottomRight(glm::vec3(0,0,0)) { }

/////////////////////////////////////////////////////////////////////////////////////
// ViewFrustum::calculateViewFrustum()
//
// Description: this will calculate the view frustum bounds for a given position
// 				and direction
//
// Notes on how/why this works: 
//     http://www.lighthouse3d.com/tutorials/view-frustum-culling/view-frustums-shape/
//
void ViewFrustum::calculate() {

    static const double PI_OVER_180			= 3.14159265359 / 180.0; // would be better if this was in a shared location
	
	glm::vec3 front    = _direction;
	
	// Calculating field of view.
	float fovInRadians = _fieldOfView * PI_OVER_180;
	
	float twoTimesTanHalfFOV = 2.0f * tan(fovInRadians/2.0f);

    // Do we need this?
	//tang = (float)tan(ANG2RAD * angle * 0.5) ;

    float nearClip = _nearClip;
    float farClip  = _farClip;
	
	_nearHeight = (twoTimesTanHalfFOV * nearClip);
	_nearWidth  = _nearHeight * _aspectRatio;
	_farHeight  = (twoTimesTanHalfFOV * farClip);
	_farWidth   = _farHeight * _aspectRatio;

	float farHalfHeight    = (_farHeight * 0.5f);
	float farHalfWidth     = (_farWidth  * 0.5f);
	_farCenter       = _position+front * farClip;
	_farTopLeft      = _farCenter  + (_up * farHalfHeight)  - (_right * farHalfWidth); 
	_farTopRight     = _farCenter  + (_up * farHalfHeight)  + (_right * farHalfWidth); 
	_farBottomLeft   = _farCenter  - (_up * farHalfHeight)  - (_right * farHalfWidth); 
	_farBottomRight  = _farCenter  - (_up * farHalfHeight)  + (_right * farHalfWidth); 

	float nearHalfHeight   = (_nearHeight * 0.5f);
	float nearHalfWidth    = (_nearWidth  * 0.5f);
	_nearCenter      = _position+front * nearClip;
	_nearTopLeft     = _nearCenter + (_up * nearHalfHeight) - (_right * nearHalfWidth); 
	_nearTopRight    = _nearCenter + (_up * nearHalfHeight) + (_right * nearHalfWidth); 
	_nearBottomLeft  = _nearCenter - (_up * nearHalfHeight) - (_right * nearHalfWidth); 
	_nearBottomRight = _nearCenter - (_up * nearHalfHeight) + (_right * nearHalfWidth); 

	// compute the six planes
	// the function set3Points assumes that the points
	// are given in counter clockwise order
	_planes[TOPP].set3Points(_nearTopRight,_nearTopLeft,_farTopLeft);
	_planes[BOTTOMP].set3Points(_nearBottomLeft,_nearBottomRight,_farBottomRight);
	_planes[LEFTP].set3Points(_nearTopLeft,_nearBottomLeft,_farBottomLeft);
	_planes[RIGHTP].set3Points(_nearBottomRight,_nearTopRight,_farBottomRight);
	_planes[NEARP].set3Points(_nearTopLeft,_nearTopRight,_nearBottomRight);
	_planes[FARP].set3Points(_farTopRight,_farTopLeft,_farBottomLeft);

}

void ViewFrustum::dump() {

    printLog("position.x=%f, position.y=%f, position.z=%f\n", _position.x, _position.y, _position.z);
    printLog("direction.x=%f, direction.y=%f, direction.z=%f\n", _direction.x, _direction.y, _direction.z);
    printLog("up.x=%f, up.y=%f, up.z=%f\n", _up.x, _up.y, _up.z);
    printLog("right.x=%f, right.y=%f, right.z=%f\n", _right.x, _right.y, _right.z);

    printLog("farDist=%f\n", _farClip);
    printLog("farHeight=%f\n", _farHeight);
    printLog("farWidth=%f\n", _farWidth);

    printLog("nearDist=%f\n", _nearClip);
    printLog("nearHeight=%f\n", _nearHeight);
    printLog("nearWidth=%f\n", _nearWidth);

    printLog("farCenter.x=%f,      farCenter.y=%f,      farCenter.z=%f\n",
    	_farCenter.x, _farCenter.y, _farCenter.z);
    printLog("farTopLeft.x=%f,     farTopLeft.y=%f,     farTopLeft.z=%f\n",
    	_farTopLeft.x, _farTopLeft.y, _farTopLeft.z);
    printLog("farTopRight.x=%f,    farTopRight.y=%f,    farTopRight.z=%f\n",
    	_farTopRight.x, _farTopRight.y, _farTopRight.z);
    printLog("farBottomLeft.x=%f,  farBottomLeft.y=%f,  farBottomLeft.z=%f\n",
    	_farBottomLeft.x, _farBottomLeft.y, _farBottomLeft.z);
    printLog("farBottomRight.x=%f, farBottomRight.y=%f, farBottomRight.z=%f\n",
    	_farBottomRight.x, _farBottomRight.y, _farBottomRight.z);

    printLog("nearCenter.x=%f,      nearCenter.y=%f,      nearCenter.z=%f\n",
    	_nearCenter.x, _nearCenter.y, _nearCenter.z);
    printLog("nearTopLeft.x=%f,     nearTopLeft.y=%f,     nearTopLeft.z=%f\n",
    	_nearTopLeft.x, _nearTopLeft.y, _nearTopLeft.z);
    printLog("nearTopRight.x=%f,    nearTopRight.y=%f,    nearTopRight.z=%f\n",
    	_nearTopRight.x, _nearTopRight.y, _nearTopRight.z);
    printLog("nearBottomLeft.x=%f,  nearBottomLeft.y=%f,  nearBottomLeft.z=%f\n",
    	_nearBottomLeft.x, _nearBottomLeft.y, _nearBottomLeft.z);
    printLog("nearBottomRight.x=%f, nearBottomRight.y=%f, nearBottomRight.z=%f\n",
    	_nearBottomRight.x, _nearBottomRight.y, _nearBottomRight.z);
}


int ViewFrustum::pointInFrustum(glm::vec3 &p) {
	int result = INSIDE;
	for(int i=0; i < 6; i++) {
		if (_planes[i].distance(p) < 0)
			return OUTSIDE;
	}
	return(result);
}

int ViewFrustum::sphereInFrustum(glm::vec3 &center, float radius) {
	int result = INSIDE;
	float distance;
	for(int i=0; i < 6; i++) {
		distance = _planes[i].distance(center);
		if (distance < -radius)
			return OUTSIDE;
		else if (distance < radius)
			result =  INTERSECT;
	}
	return(result);
}


int ViewFrustum::boxInFrustum(AABox &b) {

printf("ViewFrustum::boxInFrustum() box.corner=%f,%f,%f x=%f\n",b.corner.x,b.corner.y,b.corner.z,b.x);
	int result = INSIDE;
	for(int i=0; i < 6; i++) {

        printf("plane[%d] -- point(%f,%f,%f) normal(%f,%f,%f) d=%f \n",i,
            _planes[i].point.x, _planes[i].point.y, _planes[i].point.z, 
            _planes[i].normal.x, _planes[i].normal.y, _planes[i].normal.z, 
            _planes[i].d
        );

	    glm::vec3 normal = _planes[i].normal;
	    glm::vec3 boxVertexP = b.getVertexP(normal);
	    float planeToBoxVertexPDistance = _planes[i].distance(boxVertexP);

	    glm::vec3 boxVertexN = b.getVertexN(normal);
	    float planeToBoxVertexNDistance = _planes[i].distance(boxVertexN);
	    
	    
	    
        printf("plane[%d] normal=(%f,%f,%f) bVertexP=(%f,%f,%f) planeToBoxVertexPDistance=%f  boxVertexN=(%f,%f,%f) planeToBoxVertexNDistance=%f\n",i, 
            normal.x,normal.y,normal.z,
            boxVertexP.x,boxVertexP.y,boxVertexP.z,planeToBoxVertexPDistance,
            boxVertexN.x,boxVertexN.y,boxVertexN.z,planeToBoxVertexNDistance
            );

		if (planeToBoxVertexPDistance < 0) {
			return OUTSIDE;
		} else if (planeToBoxVertexNDistance < 0) {
			result =  INTERSECT;
		}
	}
	return(result);
 }

