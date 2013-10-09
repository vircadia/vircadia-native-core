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
    unsigned int i;
    glm::vec3 box = glm::vec3(PARTICLE_WORLD_SIZE);
    bounds = box;
    count = NUM_PARTICLES;
    particles = new Particle[count];
    field = new Field(PARTICLE_WORLD_SIZE);
    
    for (i = 0; i < count; i++) {
        particles[i].position = randVector() * box;
        const float INIT_VEL_SCALE = 0.03f;
        particles[i].velocity = randVector() * ((float)PARTICLE_WORLD_SIZE * INIT_VEL_SCALE);
        particles[i].color = randVector();
     }
}

void Cloud::render() {
    //field->render();
    
    glPointSize(3.0f);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_POINT_SMOOTH);
    glBegin(GL_POINTS);
        for (unsigned int i = 0; i < count; i++)
        {
            glColor3f(particles[i].color.x,
                      particles[i].color.y,
                      particles[i].color.z);
            glVertex3f(particles[i].position.x,
                       particles[i].position.y,
                       particles[i].position.z);
        }
    glEnd();
}

void Cloud::simulate (float deltaTime) {
    unsigned int i;
    field->simulate(deltaTime);
    for (i = 0; i < count; ++i) {
        
        // Update position 
        particles[i].position += particles[i].velocity * deltaTime;

        // Decay Velocity (Drag)
        const float CONSTANT_DAMPING = 0.15f;
        particles[i].velocity *= (1.f - CONSTANT_DAMPING * deltaTime);
                
        // Interact with Field
        const float FIELD_COUPLE = 0.005f; 
        field->interact(deltaTime,
                        &particles[i].position,
                        &particles[i].velocity,
                        &particles[i].color,
                        FIELD_COUPLE);
        
        //  Update color to velocity 
        particles[i].color = (glm::normalize(particles[i].velocity) * 0.5f) + 0.5f;
        
        // Bounce at bounds
        if ((particles[i].position.x > bounds.x) || (particles[i].position.x < 0.f)) {
            particles[i].position.x = glm::clamp(particles[i].position.x, 0.f, bounds.x);
            particles[i].velocity.x *= -1.f;
        }
        if ((particles[i].position.y > bounds.y) || (particles[i].position.y < 0.f)) {
            particles[i].position.y = glm::clamp(particles[i].position.y, 0.f, bounds.y);
            particles[i].velocity.y *= -1.f;
        }
        if ((particles[i].position.z > bounds.z) || (particles[i].position.z < 0.f)) {
            particles[i].position.z = glm::clamp(particles[i].position.z, 0.f, bounds.z);
            particles[i].velocity.z *= -1.f;
        }
    }
 }
