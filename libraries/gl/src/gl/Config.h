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

#include <QtCore/QtGlobal>

#if defined(USE_GLES)
// Minimum GL ES version required is 3.2
#define GL_MIN_VERSION_MAJOR 0x03
#define GL_MIN_VERSION_MINOR 0x02
#define GL_DEFAULT_VERSION_MAJOR GL_MIN_VERSION_MAJOR
#define GL_DEFAULT_VERSION_MINOR GL_MIN_VERSION_MINOR
#else
// Minimum desktop GL version required is 4.1
#define GL_MIN_VERSION_MAJOR 0x04
#define GL_MIN_VERSION_MINOR 0x01
#define GL_DEFAULT_VERSION_MAJOR 0x04
#define GL_DEFAULT_VERSION_MINOR 0x05
#endif

#define MINIMUM_GL_VERSION ((GL_MIN_VERSION_MAJOR << 8) | GL_MIN_VERSION_MINOR)

#if defined(Q_OS_ANDROID)
#include <EGL/egl.h>
#include <GLES3/gl32.h>
#define GL_DEPTH_COMPONENT32_OES 0x81A7
#define GL_FRAMEBUFFER_SRGB_EXT 0x8DB9
#define GL_R16_EXT 0x822A
#define GL_R16_SNORM_EXT 0x8F98
#define GL_SLUMINANCE8_EXT 0x8C47
#else
#include <glad/glad.h>
#ifndef GL_SLUMINANCE8_EXT
#define GL_SLUMINANCE8_EXT 0x8C47
#endif
// Prevent inclusion of System GL headers
#ifndef __gl_h_
#define __gl_h_
#endif
#endif

// Platform specific code to load the GL functions
namespace gl {
    void initModuleGl();
    int getSwapInterval();
    void setSwapInterval(int swapInterval);
}

#endif // hifi_gpu_GPUConfig_h
