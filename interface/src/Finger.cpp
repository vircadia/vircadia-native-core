//
//  Finger.cpp
//  interface
//
//  Created by Philip on 1/21/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include "Finger.h"

#ifdef _WIN32

float fminf( float x, float y )
{
  return x < y ? x : y;
}

#endif _WIN32

const int NUM_BEADS = 75;
const float RADIUS = 50;        //  Radius of beads around finger

const int NUM_PUCKS = 10;

Finger::Finger(int w, int h) {
    width = w;
    height = h; 
    pos.x = pos.y = 0;
    vel.x = vel.y = 0;
    target.x = target.y = 0;
    m = 1;
    pressure = 0;
    start = true;
    //  Create surface beads
    beads = new bead[NUM_BEADS];
    pucks = new puck[NUM_PUCKS];
    
    float alpha = 0;
    for (int i = 0; i < NUM_BEADS; i++) {
        beads[i].target.x = cosf(alpha)*RADIUS + 2.0f*(randFloat() - 0.5f);
        beads[i].target.y = sinf(alpha)*RADIUS + 2.0f*(randFloat() - 0.5f);
        beads[i].pos.x = beads[i].pos.y = 0.0;
        beads[i].vel.x = beads[i].vel.y = 0.0;
        alpha += 2.0f*PIf/NUM_BEADS;
        beads[i].color[0] = randFloat()*0.05f; beads[i].color[1] = randFloat()*0.05f; beads[i].color[2] = 0.75f + randFloat()*0.25f;
        beads[i].brightness = 0.0;
    }
    for (int i = 0; i < NUM_PUCKS; i++) {
        pucks[i].pos.x = randFloat()*width;
        pucks[i].pos.y = randFloat()*height;
        pucks[i].radius = 5.0f + randFloat()*30.0f;
        pucks[i].vel.x = pucks[i].vel.y = 0.0;
        pucks[i].mass = pucks[i].radius*pucks[i].radius/25.0f;   
    }
    
}

const float SHADOW_COLOR[4] = {0.0, 0.0, 0.0, 0.75};
const float SHADOW_OFFSET = 2.5;

void Finger::render() {
    glEnable(GL_POINT_SMOOTH);

    glPointSize(30.0f);
    glBegin(GL_POINTS);
    glColor4fv(SHADOW_COLOR);
    glVertex2f(pos.x + SHADOW_OFFSET, pos.y + SHADOW_OFFSET);
    glColor4f(0.0, 0.0, 0.7f, 1.0);
    glVertex2f(pos.x, pos.y);
    glEnd();
    
    //  Render Pucks
    for (int i = 0; i < NUM_PUCKS; i++) {
        glColor4fv(SHADOW_COLOR);
        glPointSize(pucks[i].radius*2.f);
        glBegin(GL_POINTS);
        glVertex2f(pucks[i].pos.x + SHADOW_OFFSET, pucks[i].pos.y + SHADOW_OFFSET);
        glColor4f(1.0, 0.0, 0.0, 1.0);
        glVertex2f(pucks[i].pos.x, pucks[i].pos.y);
        glEnd();
    }

    //  Render beads
    glPointSize(5.0f);
    glLineWidth(3.0);
    
    glBegin(GL_POINTS);
    for (int i = 0; i < NUM_BEADS; i++) {
        glColor4fv(SHADOW_COLOR);
        glVertex2f(beads[i].pos.x + SHADOW_OFFSET, beads[i].pos.y + SHADOW_OFFSET);
        glColor3f(beads[i].color[0]+beads[i].brightness, beads[i].color[1]+beads[i].brightness, beads[i].color[2]+beads[i].brightness);
        glVertex2f(beads[i].pos.x, beads[i].pos.y);
    }
    glEnd();
    
}

void Finger::setTarget(int x, int y) {
    target.x = static_cast<float>(x);
    target.y = static_cast<float>(y);
    if (start) {
        // On startup, set finger to first target location
        pos.x = target.x;
        pos.y = target.y;
        vel.x = vel.y = 0.0;
        start = false;
    }
}

void Finger::simulate(float deltaTime) {
        //  Using the new target position x and y as where the mouse intends the finger to be, move the finger
    if (!start) {
        
        //  Move the finger
        float distance = glm::distance(pos, target);
        //float spring_length = 0;
        const float SPRING_FORCE = 1000.0;
        
        if (distance > 0.1) {
            vel += glm::normalize(target - pos)*deltaTime*SPRING_FORCE*distance;
        }
        // Decay Velocity (Drag)

        vel *= 0.8;
        //vel *= (1.f - CONSTANT_DAMPING*deltaTime);
        
        //  Update position
        pos += vel*deltaTime;
        
        //  Update the beads
        const float BEAD_SPRING_FORCE = 500.0;
        const float BEAD_NEIGHBOR_FORCE = 200.0;
        const float HARD_SPHERE_FORCE = 2500.0;
        const float PRESSURE_FACTOR = 0.00;
        float separation, contact;
        float newPressure = 0;
        int n1, n2;
        float d1, d2;
        
        for (int i = 0; i < NUM_BEADS; i++) {
            distance = glm::distance(beads[i].pos, pos + beads[i].target * (1.f + pressure*PRESSURE_FACTOR));
            if (distance > 0.1) {
                beads[i].vel += glm::normalize((pos + (beads[i].target*(1.f + pressure*PRESSURE_FACTOR))) - beads[i].pos)*deltaTime*BEAD_SPRING_FORCE*distance;
            }
            //  Add forces from 2 neighboring beads
            if ((i-1)>=0) n1 = i - 1;
            else n1 = NUM_PUCKS - 1;
            if ((i+1)<NUM_PUCKS) n2 = i + 1;
            else n2 = 0;
            d1 = glm::distance(beads[i].pos, beads[n1].pos);
            if (d1 > 0.01) beads[i].vel += glm::normalize(beads[n1].pos - beads[i].pos)*deltaTime*BEAD_NEIGHBOR_FORCE*distance;
            d2 = glm::distance(beads[i].pos, beads[n2].pos);
            if (d2 > 0.01) beads[i].vel += glm::normalize(beads[n2].pos - beads[i].pos)*deltaTime*BEAD_NEIGHBOR_FORCE*distance;

            
            //  Look for hard collision with pucks
            for (int j = 0; j < NUM_PUCKS; j++) {
               
                separation = glm::distance(beads[i].pos, pucks[j].pos);
                contact = 2.5f + pucks[j].radius;
                
                //  Hard Sphere Scattering
                
                if (separation < contact) {
                    beads[i].vel += glm::normalize(beads[i].pos - pucks[j].pos)*
                        deltaTime*HARD_SPHERE_FORCE*(contact - separation);
                    pucks[j].vel -= glm::normalize(beads[i].pos - pucks[j].pos)*
                        deltaTime*HARD_SPHERE_FORCE*(contact - separation)/pucks[j].mass;
                    if (beads[i].brightness < 0.5) beads[i].brightness = 0.5;
                    beads[i].brightness *= 1.1f;
                } else {
                    beads[i].brightness *= 0.95f;
                }
                beads[i].brightness = fminf(beads[i].brightness, 1.f);
            }
            
            
            //  Decay velocity, move beads
            beads[i].vel *= 0.85;
            beads[i].pos += beads[i].vel*deltaTime;
            
            //  Measure pressure for next run
            newPressure += glm::distance(beads[i].pos, pos);
        }
        if (fabs(newPressure - NUM_BEADS*RADIUS) < 100.f) pressure = newPressure - (NUM_BEADS*RADIUS);
        
        //  Update the pucks for any pressure they have received
        for (int j = 0; j < NUM_PUCKS; j++) {
            pucks[j].vel *= 0.99;
            
            if (pucks[j].radius < 25.0) pucks[j].pos += pucks[j].vel*deltaTime;
            
            //  wrap at edges
            if (pucks[j].pos.x > width) pucks[j].pos.x = 0.0;
            if (pucks[j].pos.x < 0) pucks[j].pos.x = static_cast<float>(width);
            if (pucks[j].pos.y > height) pucks[j].pos.y = 0.0;
            if (pucks[j].pos.y < 0) pucks[j].pos.y = static_cast<float>(height);
        }
        
    }
}
