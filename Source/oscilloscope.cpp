//
//  oscilloscope.cpp
//  interface
//
//  Created by Philip on 1/28/13.
//  Copyright (c) 2013 Rosedale Lab. All rights reserved.
//

#include "oscilloscope.h"

Oscilloscope::Oscilloscope(int w,
                           int h) {
    width = w;
    height = h;
    data = new float[width];
    for (int i = 0; i < width; i++) {
        data[i] = 0.0;
    }
    current_sample = 0;
}

void Oscilloscope::addData(float d) {
    data[current_sample++] = d;
}

void Oscilloscope::render() {
    glBegin(GL_LINES);
    //glVertex2f(
    glEnd();
}