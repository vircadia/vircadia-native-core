//
//  hand.cpp
//  interface
//
//  Created by Philip Rosedale on 10/13/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "hand.h"

Hand::Hand()
{
    reset();
    noise = 0;
}

void Hand::render()
{
    glEnable(GL_DEPTH_TEST);
    glPushMatrix();
    glLoadIdentity();
    glColor3f(0.5, 0.5, 0.5);
    glBegin(GL_LINES);
        glVertex3f(-0.05, -0.5, 0.0);
        glVertex3f(position.x, position.y, position.z);
        glVertex3f(0.05, -0.5, 0.0);
        glVertex3f(position.x, position.y, position.z);
    glEnd();
    glTranslatef(position.x, position.y, position.z);
    glutSolidSphere(0.2, 15, 15);
    glPopMatrix();
}

void Hand::reset()
{
    position.x = 0.0;
    position.y = 0.0;
    position.z = -7.0;
    velocity.x = velocity.y = velocity.z = 0;
}

void Hand::simulate(float deltaTime)
{
    position += velocity*deltaTime;
    
    velocity *= (1.f - 4.0*deltaTime);
    
    if ((noise) && (randFloat() < 0.1))
    {
        velocity.x += (randFloat() - 0.5)*noise;
        velocity.y += (randFloat() - 0.5)*noise;
        velocity.z += (randFloat() - 0.5)*noise;

    }
    //position.x += (randFloat() - 0.5)/20.0;
    //position.y += (randFloat() - 0.5)/20.0;
    //position.z += (randFloat() - 0.5)/20.0;
    
}