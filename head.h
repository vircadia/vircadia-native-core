//
//  head.h
//  interface
//
//  Created by Philip Rosedale on 9/11/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#ifndef interface_head_h
#define interface_head_h

#include <iostream>
#include "field.h"
#include "world.h"
#include <GLUT/glut.h>

class Head {
    float pitch;
    float yaw;
    float roll;
public:
    void setPitch(float);
    void getPitch(float);
    void render();
    void update();
};

#endif
