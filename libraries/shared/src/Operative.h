//
//  Operative.h
//  hifi
//
//  Created by Stephen Birarda on 7/1/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__Operative__
#define __hifi__Operative__

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class Operative {
public:
    Operative();
    
    bool volatile shouldStop;
    void run();
private:
    void renderMovingBug();
    void removeOldBug();
    
    glm::vec3 _bugPosition;
    glm::vec3 _bugDirection;
    glm::vec3 _bugPathCenter;
    
    float _bugPathRadius;
    float _bugPathTheta;
    float _bugRotation;
    float _bugAngleDelta;
    bool _moveBugInLine;
    
    unsigned long _packetsSent;
    unsigned long _bytesSent;
};

#endif /* defined(__hifi__Operative__) */
