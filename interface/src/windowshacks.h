//
//  windowshacks.h
//  hifi
//
//  Created by Brad Hefta-Gaub on 1/12/14.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//
//  hacks to get windows to compile
//

#ifndef __hifi__windowshacks__
#define __hifi__windowshacks__

#ifdef WIN32

#undef NOMINMAX

#define GLdouble GLdouble
#define GL_DOUBLE 0x140A


#define GL_VERTEX_PROGRAM_POINT_SIZE_ARB 0x8642
#define GL_RESCALE_NORMAL 0x803A
#define GL_VERTEX_PROGRAM_POINT_SIZE 0x8642
#define GL_CLAMP_TO_BORDER 0x812D

#include <cmath>
inline double roundf(double value) {
	return (value > 0.0) ? floor(value + 0.5) : ceil(value - 0.5);
}


#define round roundf

#endif // WIN32



#endif // __hifi__windowshacks__