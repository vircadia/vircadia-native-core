//
//  Util.h
//  interface
//
//  Created by Philip Rosedale on 8/24/12.
//  Copyright (c) 2012 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__Util__
#define __interface__Util__

#include "glm.hpp"

float azimuth_to(glm::vec3 head_pos, glm::vec3 source_pos);
float angle_to(glm::vec3 head_pos, glm::vec3 source_pos, float render_yaw, float head_yaw);

void outstring(char * string, int length);
float randFloat();
void render_world_box();
void render_vector(glm::vec3 * vec);
void drawtext(int x, int y, float scale, float rotate, float thick, int mono, char *string, 
              float r=1.0, float g=1.0, float b=1.0);
void drawvec3(int x, int y, float scale, float rotate, float thick, int mono, glm::vec3 vec, 
              float r=1.0, float g=1.0, float b=1.0);
double diffclock(timeval clock1,timeval clock2);

class StDev {
public:
    StDev();
    void reset();
    void addValue(float v);
    float getAverage();
    float getStDev();
    int getSamples() {return sampleCount;};
private:
    float * data;
    int sampleCount = 0;
};

#endif
