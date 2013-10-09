//
//  Cloud.h
//  interface
//
//  Created by Philip Rosedale on 11/17/12.
//  Copyright (c) 2012 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__Cloud__
#define __interface__Cloud__

#include "Field.h"

class Cloud {
public:
    Cloud(int num, 
          glm::vec3 box,
          int wrap);
    
    void simulate(float deltaTime);
    void render();
    
private:
    struct Particle {
        glm::vec3 position, velocity, color;
       } *particles;
    
    unsigned int count;
    glm::vec3 bounds;
    bool wrapBounds;
    Field *field;
};

#endif
