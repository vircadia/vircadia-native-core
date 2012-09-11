//
//  head.cpp
//  interface
//
//  Created by Philip Rosedale on 9/11/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include <iostream>
#include "head.h"

void Head::render()
{
    glDisable(GL_DEPTH_TEST);
    glPushMatrix();
    glTranslatef(0.f, -4.f, -15.f);
    glRotatef(yaw/2.0, 0, 1, 0);
    glRotatef(pitch/2.0, 1, 0, 0);
    glScalef(0.5, 1.0, 1.1);
    glColor3f(1.0, 0.84, 0.66);
    glutSolidSphere(1.f, 15, 15);           //  Head
    
    glTranslatef(1.f, 0.f, 0.f);
    glPushMatrix();
    glScalef(0.5, 0.75, 1.0);
    glutSolidSphere(0.5f, 15, 15);          //  Ears
    glPopMatrix();
    glTranslatef(-2.f, 0.f, 0.f);
    glPushMatrix();
    glScalef(0.5, 0.75, 1.0);
    glutSolidSphere(0.5f, 15, 15);
    glPopMatrix();
    glTranslatef(1.f, 1.f, 0.f);
    glScalef(2.5, 0.5, 2.2);
    glColor3f(0.5, 0.5, 0.5);
    glutSolidSphere(0.25f, 15, 15);         //  Beanie
    glPopMatrix();
    
}
