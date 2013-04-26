//
//  AvatarTouch.cpp
//  interface
//
//  Created by Jeffrey Ventrella
//  Copyright (c) 2012 High Fidelity, Inc. All rights reserved.
//

#include <iostream>
#include <glm/glm.hpp>
#include "AvatarTouch.h"
#include "InterfaceConfig.h"

AvatarTouch::AvatarTouch() {

    _myHandPosition   = glm::vec3( 0.0f, 0.0f, 0.0f );
    _yourHandPosition = glm::vec3( 0.0f, 0.0f, 0.0f );
}

void AvatarTouch::setMyHandPosition( glm::vec3 position ) {
    _myHandPosition = position;
}

void AvatarTouch::setYourPosition( glm::vec3 position ) {
    _yourHandPosition = position;
}


void AvatarTouch::render() {

    glm::vec3 v1( _myHandPosition );
    glm::vec3 v2( _yourHandPosition );
    
    glLineWidth( 8.0 );
    glColor4f( 0.7f, 0.4f, 0.1f, 0.6 );
    glBegin( GL_LINE_STRIP );
    glVertex3f( v1.x, v1.y, v1.z );
    glVertex3f( v2.x, v2.y, v2.z );
    glEnd();
}

void AvatarTouch::simulate (float deltaTime) {
 }
