//
//  field.cpp
//  interface
//
//  Created by Philip Rosedale on 8/23/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include <iostream>
#include "field.h"
#include "world.h"
#include <GLUT/glut.h>

//  A vector-valued field over an array of elements arranged as a 3D lattice 

struct {
    float x;
    float y;
    float z;
} field[FIELD_ELEMENTS];


int field_value(float *value, float *pos)
// sets the vector value (3 floats) to field value at location pos in space.
// returns zero if the location is outside world bounds
{
    int index = (int)(pos[0]/WORLD_SIZE*10.0) + 
    (int)(pos[1]/WORLD_SIZE*10.0)*10 + 
    (int)(pos[2]/WORLD_SIZE*10.0)*100;
    if ((index >= 0) && (index < FIELD_ELEMENTS))
    {
        value[0] = field[index].x;
        value[1] = field[index].y;
        value[2] = field[index].z;
        return 1;
    }
    else return 0;
}

void field_init()
//  Initializes the field to some random values
{
    int i;
    const float FIELD_SCALE = 0.00050;
    for (i = 0; i < FIELD_ELEMENTS; i++)
    {
        field[i].x = (randFloat() - 0.5)*FIELD_SCALE;
        field[i].y = (randFloat() - 0.5)*FIELD_SCALE;
        field[i].z = (randFloat() - 0.5)*FIELD_SCALE;
    }       
}

void field_add(float* add, float *pos)
//  At location loc, add vector add to the field values 
{
    int index = (int)(pos[0]/WORLD_SIZE*10.0) + 
    (int)(pos[1]/WORLD_SIZE*10.0)*10 + 
    (int)(pos[2]/WORLD_SIZE*10.0)*100;
    if ((index >= 0) && (index < FIELD_ELEMENTS))
    {
        field[index].x += add[0];
        field[index].y += add[0];
        field[index].z += add[0];
    }
}

void field_render()
//  Render the field lines
{
    int i;
    float fx, fy, fz;
    float scale_view = 1000.0;
    
    glColor3f(0, 1, 0);
    glBegin(GL_LINES);
    for (i = 0; i < FIELD_ELEMENTS; i++)
    {
        fx = (int)(i % 10);
        fy = (int)(i%100 / 10);
        fz = (int)(i / 100);
        
        glVertex3f(fx, fy, fz);
        glVertex3f(fx + field[i].x*scale_view, 
                   fy + field[i].y*scale_view, 
                   fz + field[i].z*scale_view);
        
    }
    glEnd();
    
    glColor3f(0, 1, 0);
    glPointSize(4.0);
    glEnable(GL_POINT_SMOOTH);
    glBegin(GL_POINTS);
    
    for (i = 0; i < FIELD_ELEMENTS; i++)
    {
        fx = (int)(i % 10);
        fy = (int)(i%100 / 10);
        fz = (int)(i / 100);
        
        glVertex3f(fx, fy, fz);
    }
    glEnd();
}


