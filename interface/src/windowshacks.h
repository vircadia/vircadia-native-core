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

#undef NOMINMAX
#define GLdouble GLdouble
#define GL_DOUBLE 0x140A
#define GL_VERTEX_PROGRAM_POINT_SIZE_ARB 0x8642
#define GL_RESCALE_NORMAL 0x803A


#endif // __hifi__windowshacks__