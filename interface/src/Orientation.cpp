//-----------------------------------------------------------
//
// Created by Jeffrey Ventrella and added as a utility 
// class for High Fidelity Code base, April 2013
//
//-----------------------------------------------------------

#include "Orientation.h"
#include "Vector3D.h"
#include "Util.h"

//------------------------
Orientation::Orientation()
{
	right.setXYZ	(  1.0,  0.0,  0.0 );
	up.setXYZ		(  0.0,  1.0,  0.0 );
	front.setXYZ	(  0.0,  0.0,  1.0 );	
}


//--------------------------------
void Orientation::setToIdentity()
{
	right.setXYZ	(  1.0,  0.0,  0.0 );
	up.setXYZ		(  0.0,  1.0,  0.0 );
	front.setXYZ	(  0.0,  0.0,  1.0 );	
}


//------------------------------------
void Orientation::set( Orientation o )
{ 
	right.set	( o.getRight() );
	up.set		( o.getUp	() );
	front.set	( o.getFront() );	
}


//-----------------------------------------------------------------------------------------
void Orientation::forceAxisInDirection( int whichAxis, const Vector3D &direction, double forceAmount )
{
	Vector3D diff;
	
	if ( whichAxis == ORIENTATION_RIGHT_AXIS )
	{
		diff.setToDifference( direction, right );
		right.addScaled( diff, forceAmount );
		right.normalize();
		up.setToCross( front, right );
		up.normalize();
		front.setToCross( right, up );
	}
	else if ( whichAxis == ORIENTATION_UP_AXIS )
	{
		diff.setToDifference( direction, up );
		up.addScaled( diff, forceAmount );
		up.normalize();
		front.setToCross( right, up );
		front.normalize();
		right.setToCross( up, front );
	}
	else if ( whichAxis == ORIENTATION_FRONT_AXIS )
	{
		diff.setToDifference( direction, front );
		front.addScaled( diff, forceAmount );
		front.normalize();
		right.setToCross( up, front );
		right.normalize();
		up.setToCross( front, right );
	}	
}


//------------------------------------------------------------------------------------------------------
void Orientation::forceFrontInDirection( const Vector3D &direction, const Vector3D &upDirection, double forceAmount )
{
	Vector3D diff;
	diff.setToDifference( direction, front );
	front.addScaled( diff, forceAmount );
	front.normalize();	
	right.setToCross( upDirection, front );
	right.normalize();
	up.setToCross( front, right );	
}




//---------------------------------------
void Orientation::yaw( double angle )
{
	double r = angle * PI_OVER_180;
	double s = sin( r );
	double c = cos( r );
	
	Vector3D cosineFront;
	Vector3D cosineRight;
	Vector3D sineFront;
	Vector3D sineRight;
	
	cosineFront.setToScaled	( front, c );
	cosineRight.setToScaled	( right, c );
	sineFront.setToScaled	( front, s );
	sineRight.setToScaled	( right, s );
	
	front.set( cosineFront	);
	front.add( sineRight	);
	
	right.set( cosineRight		);
	right.subtract( sineFront	);	
}


//---------------------------------------
void Orientation::pitch( double angle )
{
	double r = angle * PI_OVER_180;
	double s = sin( r );
	double c = cos( r );

	Vector3D cosineUp;
	Vector3D cosineFront;
	Vector3D sineUp;
	Vector3D sineFront;
	
	cosineUp.setToScaled	( up,    c );
	cosineFront.setToScaled	( front, c );
	sineUp.setToScaled		( up,    s );
	sineFront.setToScaled	( front, s );
	
	up.set( cosineUp );
	up.add( sineFront );
	
	front.set( cosineFront );
	front.subtract( sineUp  );
}


//---------------------------------------
void Orientation::roll( double angle )
{
	double r = angle * PI_OVER_180;
	double s = sin( r );
	double c = cos( r );
	
	Vector3D cosineUp;
	Vector3D cosineRight;
	Vector3D sineUp;
	Vector3D sineRight;
	
	cosineUp.setToScaled	( up,    c );
	cosineRight.setToScaled	( right, c );
	sineUp.setToScaled		( up,    s );
	sineRight.setToScaled	( right, s );
	
	up.set( cosineUp );
	up.add( sineRight );
	
	right.set( cosineRight );
	right.subtract( sineUp	);	
}


Vector3D Orientation::getRight	() { return right;	}
Vector3D Orientation::getUp		() { return up;		}
Vector3D Orientation::getFront	() { return front;	}


//-----------------------------------------------------------------------------
void Orientation::setRightUpFront( const Vector3D &r, const Vector3D &u, const Vector3D &f )
{
	//verifyValidOrientation();

	right.set	(r);
	up.set		(u);
	front.set	(f);
}


//-----------------------------------------------------------------------------
void Orientation::verifyValidOrientation()
{
	assert( right.getMagnitude	() < 1.0 + BIG_EPSILON );
	assert( right.getMagnitude	() > 1.0 - BIG_EPSILON );
	assert( up.getMagnitude		() < 1.0 + BIG_EPSILON );
	assert( up.getMagnitude		() > 1.0 - BIG_EPSILON );
	assert( front.getMagnitude	() < 1.0 + BIG_EPSILON );
	assert( front.getMagnitude	() > 1.0 - BIG_EPSILON );
	
	if ( right.getMagnitude() > 1.0 + BIG_EPSILON ) 
	{ 
		printf( "oops: the magnitude of the 'right' part of the orientation is %f!\n", right.getMagnitude() ); 
	}
	else if ( right.getMagnitude() < 1.0 - BIG_EPSILON ) 
	{ 
		printf( "oops: the magnitude of the 'right' part of the orientation is %f!\n", right.getMagnitude() ); 
	}


	if ( up.getMagnitude() > 1.0 + BIG_EPSILON ) 
	{ 
		printf( "oops: the magnitude of the 'up' part of the orientation is %f!\n", up.getMagnitude() ); 
	}
	else if ( up.getMagnitude() < 1.0 - BIG_EPSILON ) 
	{ 
		printf( "oops: the magnitude of the 'up' part of the orientation is %f!\n", up.getMagnitude() ); 
	}


	if ( front.getMagnitude() > 1.0 + BIG_EPSILON ) 
	{ 
		printf( "oops: the magnitude of the 'front' part of the orientation is %f!\n", front.getMagnitude() ); 
	}
	else if ( front.getMagnitude() < 1.0 - BIG_EPSILON ) 
	{ 
		printf( "oops: the magnitude of the 'front' part of the orientation is %f!\n", front.getMagnitude() ); 
	}
	
	if (( right.dotWith	( up	) >  BIG_EPSILON )
	||  ( right.dotWith	( up	) < -BIG_EPSILON )) { printf( "oops: the 'right' and 'up' parts of the orientation are not perpendicular! The dot is: %f\n", right.dotWith	( up	)	); }
	
	if (( right.dotWith	( front	) >  BIG_EPSILON )
	||  ( right.dotWith	( front	) < -BIG_EPSILON )) { printf( "oops: the 'right' and 'front' parts of the orientation are not perpendicular! The dot is: %f\n",	right.dotWith	( front	) ); }
	
	if (( up.dotWith	( front	) >  BIG_EPSILON )
	||  ( up.dotWith	( front	) < -BIG_EPSILON )) { printf( "oops: the 'up' and 'front' parts of the orientation are not perpendicular! The dot is: %f\n", up.dotWith	( front	)	); }
	
}




