//
//  cloud.cpp
//  interface
//
//  Created by Philip Rosedale on 11/17/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include <iostream>
#include "cloud.h"
#include "util.h"

Cloud::Cloud(int num, 
             glm::vec3 box,
             int wrap) {
    //  Create and initialize particles 
    int i;
    bounds = box;
    count = num;
    wrapBounds = wrap;
    particles = new Particle[count];
    
    for (i = 0; i < count; i++) {
        particles[i].position.x = randFloat()*box.x;
        particles[i].position.y = randFloat()*box.y;
        particles[i].position.z = randFloat()*box.z;
                
        particles[i].velocity.x = 0;  //randFloat() - 0.5;
        particles[i].velocity.y = 0;  //randFloat() - 0.5;
        particles[i].velocity.z = 0;  //randFloat() - 0.5;
        
    }
}


void Cloud::render() {
    
    float particle_attenuation_quadratic[] =  { 0.0f, 0.0f, 2.0f };
    
    glEnable( GL_TEXTURE_2D );
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glPointParameterfvARB( GL_POINT_DISTANCE_ATTENUATION_ARB, particle_attenuation_quadratic );
    
    float maxSize = 0.0f;
    glGetFloatv( GL_POINT_SIZE_MAX_ARB, &maxSize );
    glPointSize( maxSize );
    glPointParameterfARB( GL_POINT_SIZE_MAX_ARB, maxSize );
    glPointParameterfARB( GL_POINT_SIZE_MIN_ARB, 0.001f );
    
    glTexEnvf( GL_POINT_SPRITE_ARB, GL_COORD_REPLACE_ARB, GL_TRUE );
    glEnable( GL_POINT_SPRITE_ARB );
    glBegin( GL_POINTS );
        for (int i = 0; i < count; i++)
        {
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
    float verts[3], fadd[3], fval[3];
    for (i = 0; i < count; ++i) {
        
        // Update position 
        //particles[i].position += particles[i].velocity*deltaTime;
        particles[i].position += particles[i].velocity;

        
        // Drag: decay velocity
        const float CONSTANT_DAMPING = 1.0;
        particles[i].velocity *= (1.f - CONSTANT_DAMPING*deltaTime);
                
        // Read from field 
        verts[0] = particles[i].position.x;
        verts[1] = particles[i].position.y;
        verts[2] = particles[i].position.z;
        field_value(fval, &verts[0]);
        particles[i].velocity.x += fval[0];
        particles[i].velocity.y += fval[1];
        particles[i].velocity.z += fval[2];
        
        // Add back to field
        const float FIELD_COUPLE = 0.0000001;
        fadd[0] = particles[i].velocity.x*FIELD_COUPLE;
        fadd[1] = particles[i].velocity.y*FIELD_COUPLE;
        fadd[2] = particles[i].velocity.z*FIELD_COUPLE;
        field_add(fadd, &verts[0]);
            
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
