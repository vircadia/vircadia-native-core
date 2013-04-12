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

static const double	ZERO				= 0.0;
static const double	ONE					= 1.0;
static const double	ONE_HALF			= 0.5;
static const double	ONE_THIRD			= 0.3333333;
static const double	PIE					= 3.14159265359;
static const double	PI_TIMES_TWO		= 3.14159265359 * 2.0;
static const double PI_OVER_180			= 3.14159265359 / 180.0;
static const double EPSILON				= 0.00001;	//smallish number - used as margin of error for some computations 
static const double SQUARE_ROOT_OF_2	= sqrt(2);	
static const double SQUARE_ROOT_OF_3	= sqrt(3);	
static const double METER				= 1.0; 
static const double DECIMETER			= 0.1; 
static const double CENTIMETER			= 0.01; 
static const double MILLIIMETER			= 0.001; 

float azimuth_to(glm::vec3 head_pos, glm::vec3 source_pos);
float angle_to(glm::vec3 head_pos, glm::vec3 source_pos, float render_yaw, float head_yaw);

float randFloat();
void render_world_box();
void render_vector(glm::vec3 * vec);
int widthText(float scale, int mono, char *string);
void drawtext(int x, int y, float scale, float rotate, float thick, int mono, 
              char const* string, float r=1.0, float g=1.0, float b=1.0);
void drawvec3(int x, int y, float scale, float rotate, float thick, int mono, glm::vec3 vec, 
              float r=1.0, float g=1.0, float b=1.0);
double diffclock(timeval *clock1,timeval *clock2);


glm::vec3 operator* (float lhs, const glm::vec3& rhs);
glm::vec3 operator* (const glm::vec3& lhs, float rhs);

void drawGroundPlaneGrid( float size, int resolution );

#endif
