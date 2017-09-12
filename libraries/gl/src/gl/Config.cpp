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

#include "Config.h"

#include <mutex>

#if defined(Q_OS_ANDROID)
PFNGLQUERYCOUNTEREXTPROC glQueryCounterEXT = NULL;
PFNGLGETQUERYOBJECTUI64VEXTPROC glGetQueryObjectui64vEXT = NULL;
PFNGLFRAMEBUFFERTEXTUREEXTPROC glFramebufferTextureEXT = NULL;
#endif

void gl::initModuleGl() {
    static std::once_flag once;
    std::call_once(once, [] {
#if defined(Q_OS_ANDROID)
        glQueryCounterEXT = (PFNGLQUERYCOUNTEREXTPROC)eglGetProcAddress("glQueryCounterEXT");
        glGetQueryObjectui64vEXT = (PFNGLGETQUERYOBJECTUI64VEXTPROC)eglGetProcAddress("glGetQueryObjectui64vEXT");
        glFramebufferTextureEXT = (PFNGLFRAMEBUFFERTEXTUREEXTPROC)eglGetProcAddress("glFramebufferTextureEXT");
#else
        glewExperimental = true;
        glewInit();
#endif
    });
}

