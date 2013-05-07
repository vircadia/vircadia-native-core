//
//  Util.h
//  interface
//
//  Created by Philip Rosedale on 8/24/12.
//  Copyright (c) 2012 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__Util__
#define __interface__Util__

#ifdef _WIN32
#include "Systime.h"
#else
#include <sys/time.h>
#endif

#include <glm/glm.hpp>

#include <Orientation.h>

// the standard sans serif font family
#define SANS_FONT_FAMILY "Helvetica"

// the standard mono font family
#define MONO_FONT_FAMILY "Courier"


void eulerToOrthonormals(glm::vec3 * angles, glm::vec3 * fwd, glm::vec3 * left, glm::vec3 * up);

float azimuth_to(glm::vec3 head_pos, glm::vec3 source_pos);
float angle_to(glm::vec3 head_pos, glm::vec3 source_pos, float render_yaw, float head_yaw);

float randFloat();
void render_world_box();
int widthText(float scale, int mono, char const* string);
float widthChar(float scale, int mono, char ch);
void drawtext(int x, int y, float scale, float rotate, float thick, int mono, 
              char const* string, float r=1.0, float g=1.0, float b=1.0);
void drawvec3(int x, int y, float scale, float rotate, float thick, int mono, glm::vec3 vec, 
              float r=1.0, float g=1.0, float b=1.0);

void drawVector(glm::vec3* vector);

float angleBetween(glm::vec3 * v1, glm::vec3 * v2); 

double diffclock(timeval *clock1,timeval *clock2);

void drawGroundPlaneGrid(float size);

void renderDiskShadow(glm::vec3 position, glm::vec3 upDirection, float radius, float darkness);

void renderOrientationDirections( glm::vec3 position, Orientation orientation, float size );


class oTestCase {
public:
    float yaw;
    float pitch;
    float roll;
    
    float frontX;    
    float frontY;
    float frontZ;    
    
    float upX;    
    float upY;
    float upZ;    
    
    float rightX;    
    float rightY;
    float rightZ;    
    
    oTestCase(
        float yaw, float pitch, float roll, 
        float frontX, float frontY, float frontZ,
        float upX, float upY, float upZ,
        float rightX, float rightY, float rightZ
    ) : 
        yaw(yaw),pitch(pitch),roll(roll),
        frontX(frontX),frontY(frontY),frontZ(frontZ),
        upX(upX),upY(upY),upZ(upZ),
        rightX(rightX),rightY(rightY),rightZ(rightZ)
    {};
};


void testOrientationClass();

#endif
