//
//  cloud.h
//  interface
//
//  Created by Philip Rosedale on 11/17/12.
//  Copyright (c) 2012 High Fidelity, Inc. All rights reserved.
//

#ifndef interface_cloud_h
#define interface_cloud_h

#include "field.h"

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
};

#endif
