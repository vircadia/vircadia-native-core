//
//  oscilloscope.h
//  interface
//
//  Created by Philip on 1/28/13.
//  Copyright (c) 2013 Rosedale Lab. All rights reserved.
//

#ifndef __interface__oscilloscope__
#define __interface__oscilloscope__

#include "glm.hpp"
#include "util.h"
#include "world.h"
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
