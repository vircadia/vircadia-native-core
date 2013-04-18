//-----------------------------------------------------------
//
// Created by Jeffrey Ventrella  
// Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//-----------------------------------------------------------

#include "Orientation.h"
#include <SharedUtil.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
//#include "Util.h"


// XXXBHG - this test has not yet been reworked to match the correct vector orientation 
// of the coordinate system, so don't  use it for now.
static bool testingForNormalizationAndOrthogonality = false;

Orientation::Orientation() {
    setToIdentity();
}

void Orientation::setToIdentity() {
    _yaw    = 0.0;
    _pitch  = 0.0;
    _roll   = 0.0;
	right	= glm::vec3( -1.0f,  0.0f,  0.0f );
	up		= glm::vec3(  0.0f,  1.0f,  0.0f );
	front	= glm::vec3(  0.0f,  0.0f,  1.0f );
}

void Orientation::set( Orientation o ) { 
	right	= o.right;
	up		= o.up;
	front	= o.front;	
}

void Orientation::update() {

    float pitchRads  = _pitch * PI_OVER_180;
    float yawRads    = _yaw * PI_OVER_180;
    float rollRads   = _roll * PI_OVER_180;

    glm::quat q(glm::vec3(pitchRads, -(yawRads), rollRads));

    //  Next, create a rotation matrix from that quaternion
    glm::mat4 rotation;
    rotation = glm::mat4_cast(q);
    
    //  Transform the original vectors by the rotation matrix to get the new vectors
    glm::vec4 qup(0,1,0,0);
    glm::vec4 qright(-1,0,0,0);
    glm::vec4 qfront(0,0,1,0);
    glm::vec4 upNew    = qup*rotation;
    glm::vec4 rightNew = qright*rotation;
    glm::vec4 frontNew = qfront*rotation;

    //  Copy the answers to output vectors
    up.x = upNew.x;
    up.y = upNew.y;
    up.z = upNew.z;

    right.x = rightNew.x;
    right.y = rightNew.y;
    right.z = rightNew.z;
    
    front.x = frontNew.x;
    front.y = frontNew.y;
    front.z = frontNew.z;
    
    if ( testingForNormalizationAndOrthogonality ) { testForOrthogonalAndNormalizedVectors( EPSILON ); }
}

void Orientation::yaw(float angle) {
    // remember the value for any future changes to other angles
    _yaw = angle;
    update();
}

void Orientation::pitch( float angle ) {
    // remember the value for any future changes to other angles
    _pitch = angle;
    update();
}


void Orientation::roll( float angle ) {
    _roll = angle;
    update();
}


void Orientation::setRightUpFront( const glm::vec3 &r, const glm::vec3 &u, const glm::vec3 &f ) {
    right   = r;
    up      = u;
    front   = f;
}

void Orientation::testForOrthogonalAndNormalizedVectors( float epsilon ) {

    // XXXBHG - this test has not yet been reworked to match the correct vector orientation
    // of the coordinate system
    // bail for now, assume all is good
    return;

    // make sure vectors are normalized (or close enough to length 1.0)
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


    // make sure vectors are orthogonal (or close enough)
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


