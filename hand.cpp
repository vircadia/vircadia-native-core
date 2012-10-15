//
//  hand.cpp
//  interface
//
//  Created by Philip Rosedale on 10/13/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "hand.h"

const float DEFAULT_X = 0.0; 
const float DEFAULT_Y = 0.0; 
const float DEFAULT_Z = -7.0; 

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
    position.x = DEFAULT_X;
    position.y = DEFAULT_Y;
    position.z = DEFAULT_Z;
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
    
}