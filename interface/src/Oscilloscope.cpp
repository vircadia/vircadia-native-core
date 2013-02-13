//
//  Oscilloscope.cpp
//  interface
//
//  Created by Philip on 1/28/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include "Oscilloscope.h"

Oscilloscope::Oscilloscope(int w,
                           int h, bool isOn) {
    width = w;
    height = h;
    data1 = new float[width];
    data2 = new float[width];
    for (int i = 0; i < width; i++) {
        data1[i] = 0.0;
        data2[i] = 0.0;
    }
    state = isOn;
    current_sample = 0;
}

void Oscilloscope::addData(float d, int channel, int position) {
    if (channel == 1) data1[position] = d;
    else data2[position] = d;
}

void Oscilloscope::render() {
    glColor3f(1,1,1);
    glBegin(GL_LINES);
    for (int i = 0; i < width; i++) {
        glVertex2f((float)i, height/2 + data1[i]*(float)height);
    }
    glEnd();
    
    glColor3f(0,1,1);
    glBegin(GL_LINES);
    for (int i = 0; i < width; i++) {
        glVertex2f((float)i, height/2 + data2[i]*(float)height);
    }
    glEnd();
    
}