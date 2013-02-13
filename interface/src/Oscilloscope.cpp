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
    data = new float[width];
    for (int i = 0; i < width; i++) {
        data[i] = 0.0;
    }
    state = isOn;
    current_sample = 0;
}

void Oscilloscope::addData(float d, int position) {
    data[position] = d;
}

void Oscilloscope::render(float r, float g, float b) {
    glColor3f(r,g,b);
    glBegin(GL_LINES);
    for (int i = 0; i < width; i++) {
        glVertex2f((float)i, height/2 + data[i]*(float)height);
    }
    glEnd();
}