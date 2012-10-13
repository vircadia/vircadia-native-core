//
//  particle.cpp
//  interface
//
//  Created by Seiji Emery on 9/4/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "particle.h"

void ParticleSystem::simulate (float deltaTime) {
    for (unsigned int i = 0; i < particleCount; ++i) {
        
        // Move particles
        particles[i].position += particles[i].velocity * deltaTime;
        
        // Add gravity
        particles[i].velocity.y -= gravity;
        
        // Drag: decay velocity
        particles[i].velocity *= 0.99;
        
        // Add velocity from field
        //Field::addTo(particles[i].velocity);
        //particles[i].velocity += Field::valueAt(particles[i].position);
   
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
                particles[i].velocity.x *= -1;
            }
            if (particles[i].position.y > bounds.y 
            || particles[i].position.y < 0.f) {
                particles[i].velocity.y *= -1;
            }
            if (particles[i].position.z > bounds.z 
            || particles[i].position.z < 0.f) {
                particles[i].velocity.z *= -1;
            }
        }
    }
}









