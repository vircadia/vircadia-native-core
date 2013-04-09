//-----------------------------------------------------------
//
// Created by Jeffrey Ventrella and added as a utility 
// class for High Fidelity Code base, April 2013
//
//-----------------------------------------------------------

#include "Vector3D.h"
#include <cmath> // "Math.h"

//---------------------------------------
Vector3D::Vector3D()
{
	x = 0.0;
	y = 0.0;
	z = 0.0;	
}

//---------------------------------------
Vector3D::Vector3D( double x_, double y_, double z_ )
{
	x = x_;
	y = y_;
	z = z_;	
}

//---------------------------------------
Vector3D::Vector3D( const Vector3D & v )
{
	x = v.x;
	y = v.y;
	z = v.z;
}

//-----------------------------------------------------
void Vector3D::setXYZ( double x_, double y_, double z_ )
{
	x = x_;
	y = y_;
	z = z_;	
}


//---------------------
void Vector3D::clear()
{
	x = 0.0;
	y = 0.0;
	z = 0.0;	
}


//-----------------------------------------------------
void Vector3D::addXYZ( double x_, double y_, double z_ )
{
	x += x_;
	y += y_;
	z += z_;	
}

//---------------------------------------
void Vector3D::set( const Vector3D &v )
{
	x = v.x;
	y = v.y;
	z = v.z;	
}

//-------------------------------------
void Vector3D::add( const Vector3D &v )
{
	x += v.x;
	y += v.y;
	z += v.z;	
}


//--------------------------------------------
void Vector3D::subtract ( const Vector3D &v )
{
	x -= v.x;
	y -= v.y;
	z -= v.z;	
}

//-----------------------------------------------------
void Vector3D::addScaled( const Vector3D &v, double s )
{
	x += v.x * s;
	y += v.y * s;
	z += v.z * s;	
}

//-----------------------------------------------------
void Vector3D::subtractScaled( const Vector3D &v, double s )
{
	x -= v.x * s;
	y -= v.y * s;
	z -= v.z * s;	
}
	   

//-------------------------
void Vector3D::normalize()
{
	double d = sqrt( x * x + y * y + z * z );
	
	if ( d > 0.0 )
	{
		x /= d;
		y /= d;
		z /= d;
	}	
}


//--------------------------------------------
void Vector3D::setX ( double x_ ) { x = x_; }
void Vector3D::setY ( double y_ ) { y = y_; }
void Vector3D::setZ ( double z_ ) { z = z_; }

void Vector3D::addX ( double x_ ) { x += x_; }
void Vector3D::addY ( double y_ ) { y += y_; }
void Vector3D::addZ ( double z_ ) { z += z_; }

double Vector3D::getX () { return x; }
double Vector3D::getY () { return y; }
double Vector3D::getZ () { return z; }

void Vector3D::scaleX ( double s ) { x *= s; }
void Vector3D::scaleY ( double s ) { y *= s; }
void Vector3D::scaleZ ( double s ) { z *= s; }


//-----------------------------------------------------
void Vector3D::setToScaled( const Vector3D &v, double s )
{
	Vector3D c;
	
	x = v.x * s;
	y = v.y * s;
	z = v.z * s;	
}

//--------------------------------------------------------------------
void Vector3D::setToAverage( const Vector3D &v1, const Vector3D &v2 )
{
	x = v1.x + ( v2.x - v1.x ) * 0.5;
	y = v1.y + ( v2.y - v1.y ) * 0.5;
	z = v1.z + ( v2.z - v1.z ) * 0.5;	
}


//-----------------------------------------------------
void Vector3D::setToDifference( const Vector3D &v1, const Vector3D &v2 )
{
	x = v1.x - v2.x;
	y = v1.y - v2.y;
	z = v1.z - v2.z;	
}

//-----------------------------------------------------
void Vector3D::scale( double s )
{
	x *= s;
	y *= s;
	z *= s;	
}

//-----------------------------------------------------
double Vector3D::getMagnitude()
{	
	return sqrt( x * x + y * y + z * z );	
}


//-----------------------------------------------------
double Vector3D::getMagnitudeSquared()
{	
	return x * x + y * y + z * z ;	
}

//-----------------------------------------------------
double Vector3D::getDistanceTo( const Vector3D &v )
{
	double xx = v.x - x;
	double yy = v.y - y;
	double zz = v.z - z;
	
	return sqrt( xx * xx + yy * yy + zz * zz );
}


//-----------------------------------------------------
double Vector3D::getDistanceSquaredTo( const Vector3D &v )
{
	double xx = v.x - x;
	double yy = v.y - y;
	double zz = v.z - z;
	
	return xx * xx + yy * yy + zz * zz;
}


//-------------------------------------------------------------------
double Vector3D::getDistance( const Vector3D &v1, const Vector3D &v2 )
{	
	double xx = v2.x - v1.x;
	double yy = v2.y - v1.y;
	double zz = v2.z - v1.z;
	
	return sqrt( xx * xx + yy * yy + zz * zz );	
}


//---------------------------------------------------------------------------
double Vector3D::getDistanceSquared( const Vector3D &v1, const Vector3D &v2 )
{	
	double xx = v2.x - v1.x;
	double yy = v2.y - v1.y;
	double zz = v2.z - v1.z;
	
	return xx * xx + yy * yy + zz * zz;	
}


//-----------------------------------------------------
double Vector3D::dotWith( const Vector3D &v )
{	
	return 
	x * v.x +
	y * v.y +
	z * v.z;	
}



//-----------------------------------------------------------------
void Vector3D::setToCross( const Vector3D &v1, const Vector3D &v2 )
{	
	x = v1.z * v2.y - v1.y * v2.z;
	y = v1.x * v2.z - v1.z * v2.x;
	z = v1.y * v2.x - v1.x * v2.y;	
}


//---------------------------------------------------------------
void Vector3D::setToSum( const Vector3D &v1, const Vector3D &v2 )
{	
	x = v1.x + v2.x;
	y = v1.y + v2.y;
	z = v1.z + v2.z;	
}


//-----------------------------------------------------
void Vector3D::halve()
{
	x *= 0.5;
	y *= 0.5;
	z *= 0.5;
}





