//
//  util.cpp
//  interface
//
//  Created by Philip Rosedale on 8/24/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif
#include <iostream>
#include "world.h"

void render_world_box()
{
    //  Show edge of world 
    glColor3f(0.1, 0.1, 0.1);
    glBegin(GL_LINE_STRIP);
    glVertex3f(0,0,0);
    glVertex3f(WORLD_SIZE,0,0);
    glVertex3f(WORLD_SIZE,WORLD_SIZE,0);
    glVertex3f(0,WORLD_SIZE,0);
    glVertex3f(0,0,0);
    glVertex3f(0,0,WORLD_SIZE);
    glVertex3f(WORLD_SIZE,0,WORLD_SIZE);
    glVertex3f(WORLD_SIZE,WORLD_SIZE,WORLD_SIZE);
    glVertex3f(0,WORLD_SIZE,WORLD_SIZE);
    glVertex3f(0,0,WORLD_SIZE);
    glEnd();

    glBegin(GL_LINES);
    glVertex3f(0,WORLD_SIZE,0);
    glVertex3f(0,WORLD_SIZE,WORLD_SIZE);

    glVertex3f(WORLD_SIZE,WORLD_SIZE,0);
    glVertex3f(WORLD_SIZE,WORLD_SIZE,WORLD_SIZE);

    glVertex3f(WORLD_SIZE,0,0);
    glVertex3f(WORLD_SIZE,0,WORLD_SIZE);
    glEnd();
}
