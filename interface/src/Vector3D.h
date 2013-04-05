//-----------------------------------------------------------
//
// Created by Jeffrey Ventrella and added as a utility 
// class for High Fidelity Code base, April 2013
//
//-----------------------------------------------------------

#ifndef __interface__vector3D__
#define __interface__vector3D__

class Vector3D
{
public:

	//------------------
	// members
	//------------------
	double x;
	double y;
	double z;

	//------------------
	// methods
	//------------------
	Vector3D();
	Vector3D( double, double, double );
	Vector3D( const Vector3D & );
	
	void	clear();
	void	set				( const Vector3D & );
	void	setToScaled		( const Vector3D &, double );
	void	add				( const Vector3D & );
	void	subtract		( const Vector3D & );
	void	addScaled		( const Vector3D &, double );
	void	subtractScaled	( const Vector3D &, double );
	void	normalize		();
	void	setToCross		( const Vector3D &, const Vector3D & );
	void	setToAverage	( const Vector3D &, const Vector3D & );
	void	setToSum		( const Vector3D &, const Vector3D & );
	void	setXYZ			( double, double, double );
	void	addXYZ			( double, double, double );
	void	setX			( double );
	void	setY			( double );
	void	setZ			( double );
	void	addX			( double );
	void	addY			( double );
	void	addZ			( double );
	void	scaleX			( double );
	void	scaleY			( double );
	void	scaleZ			( double );
	void	halve			();
	double	getX			();
	double	getY			();
	double	getZ			();
	double	getMagnitude	();
	double	getMagnitudeSquared	();
	double	getDistance			( const Vector3D &, const Vector3D & );
	double	getDistanceSquared	( const Vector3D &, const Vector3D & );
	double	getDistanceTo		( const Vector3D & );
	double	getDistanceSquaredTo( const Vector3D & );
	double	dotWith				( const Vector3D & );
	void	scale				( double );
	void	setToDifference		( const Vector3D &, const Vector3D & );
};

#endif 
