//
//  Particle.cpp
//  interface
//
//  Created by Seiji Emery on 9/4/12.
//  Copyright (c) 2012 High Fidelity, Inc. All rights reserved.
//

#include "Particle.h"

#define NUM_ELEMENTS 4

glm::vec3 color0(1,0,0);             //  Motionless particle
glm::vec3 color1(0,1,0);             //  Spring force 
glm::vec3 color2(0,0,1);
glm::vec3 color3(0,1,1);

float radii[NUM_ELEMENTS] = {0.3f, 0.5f, 0.2f, 0.4f};

ParticleSystem::ParticleSystem(int num, 
                               glm::vec3 box, 
                               int wrap, 
                               float noiselevel, 
                               float setscale,
                               float setgravity) {
    //  Create and initialize particles 
    int element;
    bounds = box;
    count = num;
    wrapBounds = false;
    noise = noiselevel;
    gravity = setgravity;
    scale = setscale; 
    particles = new Particle[count];
    
    for (unsigned i = 0; i < count; i++) {
        particles[i].position.x = randFloat()*box.x;
        particles[i].position.y = randFloat()*box.y;
        particles[i].position.z = randFloat()*box.z;
        
        // Constrain to a small box in center
        //particles[i].position.x = randFloat()+box.x/2.0;
        //particles[i].position.y = randFloat()+box.y/2.0;
        //particles[i].position.z = randFloat()+box.z/2.0;
        
        particles[i].velocity.x = 0;
        particles[i].velocity.y = 0;
        particles[i].velocity.z = 0;
        
        particles[i].parent = 0; 
        particles[i].link *= 0;
        
        element = 1; //rand()%NUM_ELEMENTS;
        particles[i].element = element; 
        
        if (element == 0) particles[i].color = color0;
        else if (element == 1) particles[i].color = color1;
        else if (element == 2) particles[i].color = color2;
        else if (element == 3) particles[i].color = color3;
        
        particles[i].radius = 0.10f;  //radii[element]*scale;
        particles[i].isColliding = false;
        
        
    }
}

bool ParticleSystem::updateHand(glm::vec3 pos, glm::vec3 vel, float radius) {
    handPos = pos;
    handVel = vel;
    handRadius = radius;
    return handIsColliding;
}

void ParticleSystem::resetHand() {
    handActive = false; 
    handIsColliding = false;
    handPos = glm::vec3(0,0,0);
    handVel = glm::vec3(0,0,0);
    handRadius = 0;
}

void ParticleSystem::render() {
    for (unsigned int i = 0; i < count; ++i) {
        glPushMatrix();
            glTranslatef(particles[i].position.x, particles[i].position.y, particles[i].position.z);
            if (particles[i].numSprung == 0) glColor3f(1,1,1);
            else if (particles[i].numSprung == 1) glColor3f(0,1,0);
            else if (particles[i].numSprung == 2) glColor3f(1,1,0);
            else if (particles[i].numSprung >= 3) glColor3f(1,0,0);
            glutSolidSphere(particles[i].radius, 15, 15);
        glPopMatrix();
    }
}

void ParticleSystem::link(int child, int parent) {
    particles[child].parent = parent; 
    particles[child].velocity *= 0.5;
    particles[parent].velocity += particles[child].velocity;
    particles[child].velocity *= 0.0;
    particles[child].color = glm::vec3(1,1,0);
    particles[child].link = particles[parent].position - particles[child].position;
}

void ParticleSystem::simulate (float deltaTime) {
    unsigned int i, j;
    for (i = 0; i < count; ++i) {
        if (particles[i].element != 0) {
            
            if (particles[i].parent == 0) {
                // Move particles
                particles[i].position += particles[i].velocity * deltaTime;
                
                // Add gravity
                particles[i].velocity.y -= gravity*deltaTime;
                
                // Drag: decay velocity
                const float CONSTANT_DAMPING = 0.1f;
                particles[i].velocity *= (1.f - CONSTANT_DAMPING*deltaTime);
               
                // Add velocity from field
                //Field::addTo(particles[i].velocity);
                //particles[i].velocity += Field::valueAt(particles[i].position);
           
                // Add noise 
                const float RAND_VEL = 0.05f;
                if (noise) {
                    if (1) {
                        particles[i].velocity += glm::vec3((randFloat() - 0.5)*RAND_VEL,
                                                           (randFloat() - 0.5)*RAND_VEL,
                                                           (randFloat() - 0.5)*RAND_VEL);
                        }
                    if (randFloat() < noise*deltaTime) {
                        particles[i].velocity += glm::vec3((randFloat() - 0.5)*RAND_VEL*100,
                                                           (randFloat() - 0.5)*RAND_VEL*100,
                                                           (randFloat() - 0.5)*RAND_VEL*100);

                        }
                    } 
            } else {
                particles[i].position = particles[particles[i].parent].position + particles[i].link;
            }
            
            
            //  Check for collision with manipulator hand 
            particles[i].isColliding = (glm::vec3(particles[i].position - handPos).length() < 
                                        (radius + handRadius));
            
            //  Check for collision with other balls
            float separation;
            const float HARD_SPHERE_FORCE = 100.0;
            const float SPRING_FORCE = 10.0;
            const float SPRING_DAMPING = 0.5;
            float spring_length = 0.5; //2*radii[1];
            float spring_range = spring_length * 1.2f;
            float contact; 
            
            particles[i].isColliding = false; 
            particles[i].numSprung = 0;  
            
            for (j = 0; j < count; j++) {
                if ((j != i) && 
                    (!particles[i].parent)) {
                    separation = glm::distance(particles[i].position, particles[j].position);
                    contact = particles[i].radius + particles[j].radius;
                    
                    //  Hard Sphere Scattering
                    
                    if (separation < contact) {
                        particles[i].velocity += glm::normalize(particles[i].position - particles[j].position)*deltaTime*HARD_SPHERE_FORCE*(contact - separation);
                        particles[i].isColliding = true;
                    } 
                    
                    //  Spring Action
                    if ((particles[i].element == 1) && (separation < spring_range)) {
                            particles[i].velocity += glm::normalize(particles[i].position - particles[j].position)*deltaTime*SPRING_FORCE*(spring_length - separation);
                            particles[i].velocity *= (1.f - SPRING_DAMPING*deltaTime);
                            particles[i].numSprung++; 
                    }
                     
                    //  Link!
                    if ((particles[i].parent == 0) && 
                        (particles[j].parent != i) &&
                        (separation > 0.9*(particles[j].radius + particles[i].radius)) &&
                        (separation < 1.0*(particles[j].radius + particles[i].radius)) ) {
                            //  Link i to j!!
                            //link(i, j);
                        }
                    
                        
                }
            }
            
            if (!particles[i].parent) {
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
    }
}









