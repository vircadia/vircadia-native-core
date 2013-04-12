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

#include "Vector3D.h"
#include "Orientation.h"

class ViewFrustum {
private:
	Vector3D _farCenter;
	Vector3D _farTopLeft;      
	Vector3D _farTopRight;     
	Vector3D _farBottomLeft;   
	Vector3D _farBottomRight;  

	Vector3D _nearCenter; 
	Vector3D _nearTopLeft;     
	Vector3D _nearTopRight;    
	Vector3D _nearBottomLeft;  
	Vector3D _nearBottomRight; 
public:
	Vector3D getFarCenter()      const { return _farCenter; };
	Vector3D getFarTopLeft()     const { return _farTopLeft; };  
	Vector3D getFarTopRight()    const { return _farTopRight; };
	Vector3D getFarBottomLeft()  const { return _farBottomLeft; };
	Vector3D getFarBottomRight() const { return _farBottomRight; };

	Vector3D getNearCenter()      const { return _nearCenter; };
	Vector3D getNearTopLeft()     const { return _nearTopLeft; };  
	Vector3D getNearTopRight()    const { return _nearTopRight; };
	Vector3D getNearBottomLeft()  const { return _nearBottomLeft; };
	Vector3D getNearBottomRight() const { return _nearBottomRight; };

	void calculateViewFrustum(Vector3D position, Orientation direction);

	ViewFrustum(Vector3D position, Orientation direction);
};


#endif /* defined(__hifi__ViewFrustum__) */
