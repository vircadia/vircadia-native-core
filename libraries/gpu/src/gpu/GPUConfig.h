//
//  GPUConfig.h
//  libraries/gpu/src/gpu
//
//  Created by Sam Gateau on 12/4/14.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef gpu__GPUConfig__
#define gpu__GPUConfig__

#define GL_GLEXT_PROTOTYPES 1

#if defined(APPLE)
#include <OpenGL/glext.h>

#elif defined(UNIX)
#include <GL/gl.h>
#include <GL/glext.h>

#elif defined(WIN32)
#include <GL/glew.h>
#include <GL/wglew.h>

#endif

#endif
