//
//  GPUConfig.h
//  libraries/gpu/src/gpu
//
//  Created by Brad Hefta-Gaub on 12/17/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef gpu__GLUTConfig__
#define gpu__GLUTConfig__

// TODO: remove these once we migrate away from GLUT calls
#if defined(__APPLE__)
#include <GLUT/glut.h>
#elif defined(WIN32)
#include <GL/glut.h>
#else
#include <GL/glut.h>
#endif


#endif // gpu__GLUTConfig__
