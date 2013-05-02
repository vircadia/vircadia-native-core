//
//  AvatarTouch.cpp
//  interface
//
//  Created by Jeffrey Ventrella
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
#include <iostream>
#include <glm/glm.hpp>
#include <SharedUtil.h>
#include "AvatarTouch.h"
#include "InterfaceConfig.h"

const float THREAD_RADIUS = 0.007;

AvatarTouch::AvatarTouch() {

    _myHandPosition   = glm::vec3( 0.0f, 0.0f, 0.0f );
    _yourHandPosition = glm::vec3( 0.0f, 0.0f, 0.0f );
    _myHandState      = 0;
    _yourHandState    = 0;
    
    for (int p=0; p<NUM_POINTS; p++) {
        _point[p] = glm::vec3( 0.0, 0.0, 0.0 );
    }
}

void AvatarTouch::setMyHandPosition( glm::vec3 position ) {
    _myHandPosition = position;
}

void AvatarTouch::setYourHandPosition( glm::vec3 position ) {
    _yourHandPosition = position;
}

void AvatarTouch::setMyHandState( int state ) {
    _myHandState = state;
}

void AvatarTouch::setYourHandState( int state ) {
    _yourHandState = state;
}

void AvatarTouch::render() {

    glm::vec3 v1( _myHandPosition );
    glm::vec3 v2( _yourHandPosition );
    
    // if my hand is grasping, show it...
	if ( _myHandState == 1 ) {
		glPushMatrix();
        glTranslatef(_myHandPosition.x, _myHandPosition.y, _myHandPosition.z);
        glColor4f( 1.0, 1.0, 0.8, 0.3 ); glutSolidSphere( 0.020f, 10.0f, 10.0f );
        glColor4f( 1.0, 1.0, 0.4, 0.2 ); glutSolidSphere( 0.025f, 10.0f, 10.0f );
        glColor4f( 1.0, 1.0, 0.2, 0.1 ); glutSolidSphere( 0.030f, 10.0f, 10.0f );
		glPopMatrix();
	}

    // if your hand is grasping, show it...
	if ( _yourHandState == 1 ) {
		glPushMatrix();
        glTranslatef(_yourHandPosition.x, _yourHandPosition.y, _yourHandPosition.z);
        glColor4f( 1.0, 1.0, 0.8, 0.3 ); glutSolidSphere( 0.020f, 10.0f, 10.0f );
        glColor4f( 1.0, 1.0, 0.4, 0.2 ); glutSolidSphere( 0.025f, 10.0f, 10.0f );
        glColor4f( 1.0, 1.0, 0.2, 0.1 ); glutSolidSphere( 0.030f, 10.0f, 10.0f );
		glPopMatrix();
	}
    
    glLineWidth( 2.0 );
    glColor4f( 0.7f, 0.4f, 0.1f, 0.3 );
    glBegin( GL_LINE_STRIP );
    glVertex3f( v1.x, v1.y, v1.z );
    glVertex3f( v2.x, v2.y, v2.z );
    glEnd();

    glColor4f( 1.0f, 1.0f, 0.0f, 0.8 );

    for (int p=0; p<NUM_POINTS; p++) {
        glBegin(GL_POINTS);
        glVertex3f(_point[p].x, _point[p].y, _point[p].z);
        glEnd();
    }    
}

void AvatarTouch::simulate (float deltaTime) {

    glm::vec3 v = _yourHandPosition - _myHandPosition;
    for (int p=0; p<NUM_POINTS; p++) {
        _point[p] = _myHandPosition + v * ( (float)p / (float)NUM_POINTS );
        _point[p].x += randFloatInRange( -THREAD_RADIUS, THREAD_RADIUS );
        _point[p].y += randFloatInRange( -THREAD_RADIUS, THREAD_RADIUS );
        _point[p].z += randFloatInRange( -THREAD_RADIUS, THREAD_RADIUS );
    }
 }
