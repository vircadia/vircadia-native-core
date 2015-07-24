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

#define GPU_CORE 1
#define GPU_LEGACY 0

#if defined(__APPLE__)
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>

#define GPU_FEATURE_PROFILE GPU_LEGACY
#define GPU_TRANSFORM_PROFILE GPU_LEGACY

#elif defined(WIN32)
#include <GL/glew.h>
#include <GL/wglew.h>

#define GPU_FEATURE_PROFILE GPU_CORE
#define GPU_TRANSFORM_PROFILE GPU_CORE

#elif defined(ANDROID)

#else
#include <GL/gl.h>
#include <GL/glext.h>

#define GPU_FEATURE_PROFILE GPU_LEGACY
#define GPU_TRANSFORM_PROFILE GPU_LEGACY

#endif

#endif
