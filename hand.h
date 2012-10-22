//
//  hand.h
//  interface
//
//  Created by Philip Rosedale on 10/13/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#ifndef interface_hand_h
#define interface_hand_h

#include "glm/glm.hpp"
#include <iostream>
#include "util.h"
#include "field.h"
#include "world.h"
#include <GLUT/glut.h>

const float RADIUS_RANGE = 10.0; 

class Hand {
public:
    Hand(float initradius, glm::vec3 color);
    void simulate (float deltaTime);
    void render ();
    void reset ();
    void setNoise (float mag) { noise = mag; };
    void addVel (glm::vec3 add) { velocity += add; }; 
    glm::vec3 getPos() { return position; };
    void setPos(glm::vec3 newpos) { position = newpos; };
    float getRadius() { return radius; };
    void  setRadius(float newradius) { radius = newradius; };
    void setColliding(bool newcollide) { isColliding = newcollide; };
private:
    glm::vec3 position, velocity, color;
    float noise;
    float radius;
    bool isColliding;
    
};


#endif
