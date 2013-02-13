//
//  Oscilloscope.h
//  interface
//
//  Created by Philip on 1/28/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__Oscilloscope__
#define __interface__Oscilloscope__

#include <glm/glm.hpp>
#include "Util.h"
#include "world.h"
#include "InterfaceConfig.h"
#include <iostream>

class Oscilloscope {
public:
    Oscilloscope(int width,
                int height, bool isOn);
    void addData(float d, int position);
    void render(float r, float g, float b);
    void setState(bool s) {state = s;};
    bool getState() {return state;};
private:
    int width;
    int height;
    float * data;
    int current_sample;
    bool state;
};
#endif /* defined(__interface__oscilloscope__) */
