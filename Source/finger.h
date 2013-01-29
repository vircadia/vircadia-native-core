//
//  finger.h
//  interface
//
//  Created by Philip on 1/21/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__finger__
#define __interface__finger__

#include "glm.hpp"
#include "util.h"
#include "world.h"
#include <GLUT/glut.h>

#include <iostream>

class Finger {
public:
    Finger(int w, int h);
    void simulate(float deltaTime);
    void render();
    void setTarget(int x, int y);
private:
    int width, height;                          //  Screen size in pixels 
    bool start;
    glm::vec2 pos, vel;                         //  Position and velocity of finger 
    float m;                                    //  Velocity of finger
    float pressure;                             //  Internal pressure of skin vessel as function of measured volume
    glm::vec2 target;                           //  Where the mouse is
    // little beads used to render the dynamic skin surface
    struct bead {
        glm::vec2 target, pos, vel;
        float color[3];
        float brightness;
    } *beads;
    struct puck {
        glm::vec2 pos, vel;
        float brightness;
        float radius;
        float mass;
    } *pucks;
};

#endif /* defined(__interface__finger__) */

