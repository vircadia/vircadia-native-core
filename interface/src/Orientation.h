//-----------------------------------------------------------
//
// Created by Jeffrey Ventrella and added as a utility 
// class for High Fidelity Code base, April 2013
//
//-----------------------------------------------------------

#ifndef __interface__orientation__
#define __interface__orientation__

#include "Math.h"
#include "Vector3D.h"

enum Axis
{
	ORIENTATION_RIGHT_AXIS,
	ORIENTATION_UP_AXIS,
	ORIENTATION_FRONT_AXIS
};

class Orientation
{
private:
	
	Vector3D right;
	Vector3D up;
	Vector3D front;
	
	void verifyValidOrientation();
	
public:
	Orientation();
	
	void yaw	( double );
	void pitch	( double );
	void roll	( double );

	void set( Orientation );
	void setToIdentity();

	void forceFrontInDirection( const Vector3D &, const Vector3D &, double );
	void forceAxisInDirection( int, const Vector3D &, double );

	Vector3D getRight();
	Vector3D getUp();
	Vector3D getFront();
	
	void setRightUpFront( const Vector3D &, const Vector3D &, const Vector3D & );
};


#endif
