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
#include "util.h"
#include "world.h"
#include <GLUT/glut.h>

class ParticleSystem {
public:
    ParticleSystem(int num, 
                   glm::vec3 box, 
                   int wrap, 
                   float noiselevel,
                   float setscale,
                   float setgravity);
    
    void simulate(float deltaTime);
    void render();
    bool updateHand(glm::vec3 pos, glm::vec3 vel, float radius);

private:
    struct Particle {
        glm::vec3 position, velocity, color, link;
        int element;
        int parent;
        float radius;
        bool isColliding;
        int numSprung;
    } *particles;
    unsigned int count;
    
    glm::vec3 bounds;
    
    float radius;
    bool wrapBounds;
    float noise;
    float gravity;
    float scale; 
    glm::vec3 color;
    
    void link(int child, int parent);
    
    //  Manipulator from outside
    void resetHand();
    bool handActive; 
    bool handIsColliding;
    glm::vec3 handPos;
    glm::vec3 handVel;
    float handRadius;
    
};

#endif
