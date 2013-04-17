//-----------------------------------------------------------
//
// Created by Jeffrey Ventrella  
// Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//-----------------------------------------------------------

#include "Orientation.h"
#include <SharedUtil.h>


static bool testingForNormalizationAndOrthogonality = true;

Orientation::Orientation() {
	right	= glm::vec3(  1.0,  0.0,  0.0 );
	up		= glm::vec3(  0.0,  1.0,  0.0 );
	front	= glm::vec3(  0.0,  0.0,  1.0 );	
}


void Orientation::setToIdentity() {
	right	= glm::vec3(  1.0,  0.0,  0.0 );
	up		= glm::vec3(  0.0,  1.0,  0.0 );
	front	= glm::vec3(  0.0,  0.0,  1.0 );	
}


void Orientation::set( Orientation o ) { 
	right	= o.right;
	up		= o.up;
	front	= o.front;	
}


void Orientation::yaw( float angle ) {
	float r = angle * PI_OVER_180;
	float s = sin(r);
	float c = cos(r);
	
	glm::vec3 cosineFront	= front * c;
	glm::vec3 cosineRight	= right * c;
	glm::vec3 sineFront		= front * s;
	glm::vec3 sineRight		= right * s;
	
	front	= cosineFront + sineRight;
	right	= cosineRight - sineFront;	
    
    if ( testingForNormalizationAndOrthogonality ) { testForOrthogonalAndNormalizedVectors( EPSILON ); }
}


void Orientation::pitch( float angle ) {
	float r = angle * PI_OVER_180;
	float s = sin(r);
	float c = cos(r);
	
	glm::vec3 cosineUp		= up	* c;
	glm::vec3 cosineFront	= front	* c;
	glm::vec3 sineUp		= up	* s;
	glm::vec3 sineFront		= front * s;
	
	up		= cosineUp		+ sineFront;
	front	= cosineFront	- sineUp;

    if ( testingForNormalizationAndOrthogonality ) { testForOrthogonalAndNormalizedVectors( EPSILON ); }
}


void Orientation::roll( float angle ) {
	float r = angle * PI_OVER_180;
	float s = sin(r);
	float c = cos(r);
	
	glm::vec3 cosineUp		= up	* c;
	glm::vec3 cosineRight	= right	* c;
	glm::vec3 sineUp		= up	* s;
	glm::vec3 sineRight		= right	* s;
	
	up		= cosineUp		+ sineRight;
	right	= cosineRight	- sineUp;	

    if ( testingForNormalizationAndOrthogonality ) { testForOrthogonalAndNormalizedVectors( EPSILON ); }
}


void Orientation::setRightUpFront( const glm::vec3 &r, const glm::vec3 &u, const glm::vec3 &f ) {
	right	= r;
	up		= u;
	front	= f;
}



//----------------------------------------------------------------------
void Orientation::testForOrthogonalAndNormalizedVectors( float epsilon ) {

    //------------------------------------------------------------------
    // make sure vectors are normalized (or close enough to length 1.0)
    //------------------------------------------------------------------
	float rightLength	= glm::length( right );
	float upLength		= glm::length( up    );
	float frontLength	= glm::length( front );
	     
	if (( rightLength > 1.0f + epsilon )
	||  ( rightLength < 1.0f - epsilon )) { 
        printf( "Error in Orientation class: right direction length is %f \n", rightLength ); 
    }
	assert ( rightLength > 1.0f - epsilon );
	assert ( rightLength < 1.0f + epsilon );


	if (( upLength > 1.0f + epsilon )
	||  ( upLength < 1.0f - epsilon )) { 
        printf( "Error in Orientation class: up direction length is %f \n", upLength ); 
    }
	assert ( upLength > 1.0f - epsilon );
	assert ( upLength < 1.0f + epsilon );


	if (( frontLength > 1.0f + epsilon )
	||  ( frontLength < 1.0f - epsilon )) { 
        printf( "Error in Orientation class: front direction length is %f \n", frontLength ); 
    }
	assert ( frontLength > 1.0f - epsilon );
	assert ( frontLength < 1.0f + epsilon );



    //----------------------------------------------------------------
    // make sure vectors are orthoginal (or close enough)
    //----------------------------------------------------------------
    glm::vec3 rightCross    = glm::cross( up, front );
    glm::vec3 upCross       = glm::cross( front, right );
    glm::vec3 frontCross    = glm::cross( right, up );
    
    float rightDiff = glm::length( rightCross - right );
    float upDiff    = glm::length( upCross - up );
    float frontDiff = glm::length( frontCross - front );


    if ( rightDiff > epsilon ) { 
        printf( "Error in Orientation class: right direction not orthogonal to up and/or front. " ); 
        printf( "The tested cross of up and front is off by %f \n", rightDiff ); 
    }
	assert ( rightDiff < epsilon );
    
    
    if ( upDiff > epsilon ) { 
        printf( "Error in Orientation class: up direction not orthogonal to front and/or right. " ); 
        printf( "The tested cross of front and right is off by %f \n", upDiff ); 
    }
	assert ( upDiff < epsilon );


    if ( frontDiff > epsilon ) { 
        printf( "Error in Orientation class: front direction not orthogonal to right and/or up. " ); 
        printf( "The tested cross of right and up is off by %f \n", frontDiff ); 
    }
	assert ( frontDiff < epsilon );
}


