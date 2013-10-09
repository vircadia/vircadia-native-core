//
//  Cloud.cpp
//  interface
//
//  Created by Philip Rosedale on 11/17/12.
//  Copyright (c) 2012 High Fidelity, Inc. All rights reserved.
//

#include <iostream>
#include <InterfaceConfig.h>
#include "Cloud.h"
#include "Util.h"
#include "Field.h"

const int NUM_PARTICLES = 100000;

Cloud::Cloud() {
    //  Create and initialize particles 
    glm::vec3 box = glm::vec3(PARTICLE_WORLD_SIZE);
    _bounds = box;
    _count = NUM_PARTICLES;
    _particles = new Particle[_count];
    _field = new Field(PARTICLE_WORLD_SIZE);
    
    for (int i = 0; i < _count; i++) {
        _particles[i].position = randVector() * box;
        const float INIT_VEL_SCALE = 0.03f;
        _particles[i].velocity = randVector() * ((float)PARTICLE_WORLD_SIZE * INIT_VEL_SCALE);
        _particles[i].color = randVector();
     }
}

void Cloud::render() {
    //field->render();
    
    glPointSize(3.0f);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_POINT_SMOOTH);
    glBegin(GL_POINTS);
        for (unsigned int i = 0; i < _count; i++)
        {
            glColor3f(_particles[i].color.x,
                      _particles[i].color.y,
                      _particles[i].color.z);
            glVertex3f(_particles[i].position.x,
                       _particles[i].position.y,
                       _particles[i].position.z);
        }
    glEnd();
}

void Cloud::simulate (float deltaTime) {
    unsigned int i;
    _field->simulate(deltaTime);
    for (i = 0; i < _count; ++i) {
        
        // Update position 
        _particles[i].position += _particles[i].velocity * deltaTime;

        // Decay Velocity (Drag)
        const float CONSTANT_DAMPING = 0.15f;
        _particles[i].velocity *= (1.f - CONSTANT_DAMPING * deltaTime);
                
        // Interact with Field
        const float FIELD_COUPLE = 0.005f; 
        _field->interact(deltaTime,
                        &_particles[i].position,
                        &_particles[i].velocity,
                        &_particles[i].color,
                        FIELD_COUPLE);
        
        //  Update color to velocity 
        _particles[i].color = (glm::normalize(_particles[i].velocity) * 0.5f) + 0.5f;
        
        // Bounce at bounds
        if ((_particles[i].position.x > _bounds.x) || (_particles[i].position.x < 0.f)) {
            _particles[i].position.x = glm::clamp(_particles[i].position.x, 0.f, _bounds.x);
            _particles[i].velocity.x *= -1.f;
        }
        if ((_particles[i].position.y > _bounds.y) || (_particles[i].position.y < 0.f)) {
            _particles[i].position.y = glm::clamp(_particles[i].position.y, 0.f, _bounds.y);
            _particles[i].velocity.y *= -1.f;
        }
        if ((_particles[i].position.z > _bounds.z) || (_particles[i].position.z < 0.f)) {
            _particles[i].position.z = glm::clamp(_particles[i].position.z, 0.f, _bounds.z);
            _particles[i].velocity.z *= -1.f;
        }
    }
 }
