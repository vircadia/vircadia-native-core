//
//  Field.cpp
//  interface
//
//  Created by Philip Rosedale on 8/23/12.
//  Copyright (c) 2012 High Fidelity, Inc. All rights reserved.
//
//  A vector-valued field over an array of elements arranged as a 3D lattice

#include "Field.h"

int Field::value(float *value, float *pos) {
    int index = (int)(pos[0] / _worldSize * 10.0) +
        (int)(pos[1] / _worldSize * 10.0) * 10 +
        (int)(pos[2] / _worldSize * 10.0) * 100;
    
    if ((index >= 0) && (index < FIELD_ELEMENTS)) {
        value[0] = _field[index].val.x;
        value[1] = _field[index].val.y;
        value[2] = _field[index].val.z;
        return 1;
    } else {
        return 0;
    }
}

Field::Field(float worldSize, float coupling) {
    _worldSize = worldSize;
    _coupling = coupling;
    //float fx, fy, fz;
    for (int i = 0; i < FIELD_ELEMENTS; i++) {
        const float FIELD_INITIAL_MAG = 0.0f;
        _field[i].val = randVector() * FIELD_INITIAL_MAG * _worldSize;
        _field[i].center.x = ((float)(i % 10) + 0.5f);
        _field[i].center.y = ((float)(i % 100 / 10) + 0.5f);
        _field[i].center.z = ((float)(i / 100) + 0.5f);
        _field[i].center *= _worldSize / 10.f;
        
    }
}

void Field::add(float* add, float *pos) {
    int index = (int)(pos[0] / _worldSize * 10.0) + 
        (int)(pos[1] / _worldSize * 10.0) * 10 +
        (int)(pos[2] / _worldSize * 10.0) * 100;
    
    if ((index >= 0) && (index < FIELD_ELEMENTS)) {
        _field[index].val.x += add[0];
        _field[index].val.y += add[1];
        _field[index].val.z += add[2];
    }
}

void Field::interact(float deltaTime, const glm::vec3& pos, glm::vec3& vel) {
    
    int index = (int)(pos.x / _worldSize * 10.0) +
        (int)(pos.y / _worldSize*10.0) * 10 +
        (int)(pos.z / _worldSize*10.0) * 100;
    if ((index >= 0) && (index < FIELD_ELEMENTS)) {
        vel += _field[index].val * deltaTime;               //  Particle influenced by field
        _field[index].val += vel * deltaTime * _coupling;   //  Field influenced by particle
    }
}

void Field::simulate(float deltaTime) {
    glm::vec3 neighbors, add, diff;

    for (int i = 0; i < FIELD_ELEMENTS; i++) {
        const float CONSTANT_DAMPING = 0.5f;
        _field[i].val *= (1.f - CONSTANT_DAMPING * deltaTime);
    }
}

void Field::render() {
    int i;
    float scale_view = 0.05f * _worldSize;
    
    glDisable(GL_LIGHTING);
    glBegin(GL_LINES);
    for (i = 0; i < FIELD_ELEMENTS; i++) {        
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
    for (i = 0; i < FIELD_ELEMENTS; i++) {
        glVertex3fv(&_field[i].center.x);
    }
    glEnd();
}



