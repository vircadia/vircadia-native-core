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

#define COLOR_MIN 0.2f // minimum R/G/B value at 0,0,0 - also needs setting in field.cpp

Cloud::Cloud(int num, 
             glm::vec3 box,
             int wrap) {
    //  Create and initialize particles 
    int i;
    bounds = box;
    count = num;
    wrapBounds = wrap;
    particles = new Particle[count];
    field = new Field();
    
    for (i = 0; i < count; i++) {
        float x = randFloat()*box.x;
        float y = randFloat()*box.y;
        float z = randFloat()*box.z;
        particles[i].position.x = x;
        particles[i].position.y = y;
        particles[i].position.z = z;
                
        particles[i].velocity.x = randFloat() - 0.5;
        particles[i].velocity.y = randFloat() - 0.5;
        particles[i].velocity.z = randFloat() - 0.5;
        
        float color_mult = 1 - COLOR_MIN;
        particles[i].color = glm::vec3(x*color_mult/WORLD_SIZE + COLOR_MIN,
                                       y*color_mult/WORLD_SIZE + COLOR_MIN,
                                       z*color_mult/WORLD_SIZE + COLOR_MIN);
    }
}


void Cloud::render() {
    
    float particle_attenuation_quadratic[] =  { 0.0f, 0.0f, 2.0f };
    
    glEnable( GL_TEXTURE_2D );
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    
    
    float maxSize = 0.0f;
    glGetFloatv( GL_POINT_SIZE_MAX_ARB, &maxSize );
    glPointSize( maxSize );
    
#ifdef __APPLE__
    glPointParameterfvARB( GL_POINT_DISTANCE_ATTENUATION_ARB, particle_attenuation_quadratic );
    glPointParameterfARB( GL_POINT_SIZE_MAX_ARB, maxSize );
    glPointParameterfARB( GL_POINT_SIZE_MIN_ARB, 0.001f );
#endif
    
    glTexEnvf( GL_POINT_SPRITE_ARB, GL_COORD_REPLACE_ARB, GL_TRUE );
    glEnable( GL_POINT_SPRITE_ARB );
    glBegin( GL_POINTS );
        for (int i = 0; i < count; i++)
        {
            glColor3f(particles[i].color.x,
                      particles[i].color.y,
                      particles[i].color.z);
            glVertex3f(particles[i].position.x,
                       particles[i].position.y,
                       particles[i].position.z);
        }
    glEnd();
    glDisable( GL_POINT_SPRITE_ARB );
    glDisable( GL_TEXTURE_2D );
}

void Cloud::simulate (float deltaTime) {
    int i;
    for (i = 0; i < count; ++i) {
        
        // Update position 
        particles[i].position += particles[i].velocity*deltaTime;
        //particles[i].position += particles[i].velocity;

        // Decay Velocity (Drag)
        const float CONSTANT_DAMPING = 0.5;
        particles[i].velocity *= (1.f - CONSTANT_DAMPING*deltaTime);
                
        // Interact with Field
        const float FIELD_COUPLE = 0.005;  //0.0000001;
        field->interact(deltaTime, &particles[i].position, &particles[i].velocity, &particles[i].color, FIELD_COUPLE);
        
        //  Update color to velocity 
        particles[i].color = (glm::normalize(particles[i].velocity)*0.5f);
        particles[i].color += 0.5f;
        
        
        //  Bounce or Wrap 
        if (wrapBounds) {
            // wrap around bounds
            if (particles[i].position.x > bounds.x)
                particles[i].position.x -= bounds.x;
            else if (particles[i].position.x < 0.0f)
                particles[i].position.x += bounds.x;
            
            if (particles[i].position.y > bounds.y)
                particles[i].position.y -= bounds.y;
            else if (particles[i].position.y < 0.0f)
                particles[i].position.y += bounds.y;
            
            if (particles[i].position.z > bounds.z)
                particles[i].position.z -= bounds.z;
            else if (particles[i].position.z < 0.0f)
                particles[i].position.z += bounds.z;
        } else {
            // Bounce at bounds
            if (particles[i].position.x > bounds.x 
                || particles[i].position.x < 0.f) {
                if (particles[i].position.x > bounds.x) particles[i].position.x = bounds.x;
                else particles[i].position.x = 0.f;
                particles[i].velocity.x *= -1;
            }
            if (particles[i].position.y > bounds.y 
                || particles[i].position.y < 0.f) {
                if (particles[i].position.y > bounds.y) particles[i].position.y = bounds.y;
                else particles[i].position.y = 0.f;
                particles[i].velocity.y *= -1;
            }
            if (particles[i].position.z > bounds.z 
                || particles[i].position.z < 0.f) {
                if (particles[i].position.z > bounds.z) particles[i].position.z = bounds.z;
                else particles[i].position.z = 0.f;
                particles[i].velocity.z *= -1;
            }
        }
    }
 }
