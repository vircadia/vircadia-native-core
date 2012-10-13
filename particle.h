//
//  particle.h
//  interface
//
//  Created by Seiji Emery on 9/4/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#ifndef interface_particle_h
#define interface_particle_h

#include "glm/glm.hpp"

class ParticleSystem {
public:
    void simulate (float deltaTime);
    void draw ();

private:
    struct Particle {
        glm::vec3 position, velocity;
    } *particles;
    unsigned int particleCount;
    
    glm::vec3 bounds;
    const static bool wrapBounds = false;
    const static float gravity = 0.0001;
};

#endif
