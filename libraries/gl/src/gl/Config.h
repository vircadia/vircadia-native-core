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

#ifndef hifi_gpu_GPUConfig_h
#define hifi_gpu_GPUConfig_h


#define GL_GLEXT_PROTOTYPES 1

#define GPU_CORE 1
#define GPU_LEGACY 0
#define GPU_CORE_41 410
#define GPU_CORE_43 430
#define GPU_CORE_MINIMUM GPU_CORE_41

#if defined(__APPLE__)

#include <GL/glew.h>

#define GPU_FEATURE_PROFILE GPU_CORE
#define GPU_INPUT_PROFILE GPU_CORE_41

#include <OpenGL/gl.h>
#include <OpenGL/glext.h>

#elif defined(WIN32)
#include <GL/glew.h>
#include <GL/wglew.h>

#define GPU_FEATURE_PROFILE GPU_CORE
#define GPU_INPUT_PROFILE GPU_CORE_43

#else

#include <GL/glew.h>

#define GPU_FEATURE_PROFILE GPU_CORE
#define GPU_INPUT_PROFILE GPU_CORE_43

#endif


#if (GPU_INPUT_PROFILE == GPU_CORE_43)
// Deactivate SSBO for now, we've run into some issues
// on GL 4.3 capable GPUs not behaving as expected
//#define GPU_SSBO_DRAW_CALL_INFO
#endif


#endif // hifi_gpu_GPUConfig_h
