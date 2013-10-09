//
//  Field.cpp
//  interface
//
//  Created by Philip Rosedale on 8/23/12.
//  Copyright (c) 2012 High Fidelity, Inc. All rights reserved.
//

#include "Field.h"

//  A vector-valued field over an array of elements arranged as a 3D lattice 

int Field::value(float *value, float *pos)
// sets the vector value (3 floats) to field value at location pos in space.
// returns zero if the location is outside world bounds
{
    int index = (int)(pos[0] / _worldSize * 10.0) +
    (int)(pos[1] / _worldSize * 10.0) * 10 + 
    (int)(pos[2] / _worldSize * 10.0) * 100;
    
    if ((index >= 0) && (index < FIELD_ELEMENTS))
    {
        value[0] = _field[index].val.x;
        value[1] = _field[index].val.y;
        value[2] = _field[index].val.z;
        return 1;
    }
    else {
        return 0;
    }
}

Field::Field(float worldSize)
//  Initializes the field to some random values
{
    _worldSize = worldSize;
    float fx, fy, fz;
    for (int i = 0; i < FIELD_ELEMENTS; i++)
    {
        const float FIELD_INITIAL_MAG = 0.0f;
        _field[i].val = randVector() * FIELD_INITIAL_MAG * _worldSize;
        _field[i].scalar = 0; 
        //  Record center point for this field cell
        fx = static_cast<float>(i % 10);
        fy = static_cast<float>(i % 100 / 10);
        fz = static_cast<float>(i / 100);
        _field[i].center.x = (fx + 0.5f);
        _field[i].center.y = (fy + 0.5f);
        _field[i].center.z = (fz + 0.5f);
        _field[i].center *= _worldSize / 10.f;
        
    }
}

void Field::add(float* add, float *pos)
//  At location loc, add vector add to the field values 
{
    int index = (int)(pos[0] / _worldSize * 10.0) + 
    (int)(pos[1] / _worldSize * 10.0)*10 +
    (int)(pos[2] / _worldSize * 10.0)*100;
    
    if ((index >= 0) && (index < FIELD_ELEMENTS))
    {
        _field[index].val.x += add[0];
        _field[index].val.y += add[1];
        _field[index].val.z += add[2];
    }
}

void Field::interact(float deltaTime, glm::vec3* pos, glm::vec3* vel, glm::vec3* color, float coupling) {
    
    int index = (int)(pos->x/ _worldSize * 10.0) +
    (int)(pos->y/_worldSize*10.0) * 10 +
    (int)(pos->z/_worldSize*10.0) * 100;
    if ((index >= 0) && (index < FIELD_ELEMENTS)) {
        //  
        //  Vector Coupling with particle velocity
        //
        *vel += _field[index].val * deltaTime;            //  Particle influenced by field
        
        glm::vec3 temp = *vel * deltaTime;               //  Field influenced by particle
        temp *= coupling;
        _field[index].val += temp;
    }
}

void Field::simulate(float deltaTime) {
    glm::vec3 neighbors, add, diff;

    for (int i = 0; i < FIELD_ELEMENTS; i++)
    {
        const float CONSTANT_DAMPING = 0.5;
        const float CONSTANT_SCALAR_DAMPING = 2.5;
        _field[i].val *= (1.f - CONSTANT_DAMPING * deltaTime);
        _field[i].scalar *= (1.f - CONSTANT_SCALAR_DAMPING * deltaTime);
    }
}

void Field::render()
//  Render the field lines
{
    int i;
    float scale_view = 0.05f * _worldSize;
    
    glDisable(GL_LIGHTING);
    glBegin(GL_LINES);
    for (i = 0; i < FIELD_ELEMENTS; i++)
    {        
        glColor3f(0, 1, 0);
        glVertex3fv(&_field[i].center.x);
        glVertex3f(_field[i].center.x + _field[i].val.x * scale_view,
                   _field[i].center.y + _field[i].val.y * scale_view,
                   _field[i].center.z + _field[i].val.z * scale_view);
    }
    glEnd();
    
    glColor3f(0, 1, 0);
    glPointSize(4.0);
    glEnable(GL_POINT_SMOOTH);
    glBegin(GL_POINTS);
    for (i = 0; i < FIELD_ELEMENTS; i++)
    {
        glVertex3fv(&_field[i].center.x);
    }
    glEnd();
}



