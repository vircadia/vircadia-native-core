//
//  Oscilloscope.h
//  interface
//
//  Created by Philip on 1/28/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__Oscilloscope__
#define __interface__Oscilloscope__

#include <glm.hpp>
#include "Util.h"
#include "World.h"
#include <GLUT/glut.h>
#include <iostream>

class Oscilloscope {
public:
    Oscilloscope(int width,
                int height);
    void addData(float data);
    void render();
private:
    int width;
    int height;
    float * data;
    int current_sample;
};
#endif /* defined(__interface__oscilloscope__) */
