//
//
//  Created by Bradley Austin Davis on 2018/11/15
//  Copyright 2013-2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include <memory>

#include <EGL/egl.h>
#include <glad/glad.h>

namespace ovr {

struct GLContext {
    using Pointer = std::shared_ptr<GLContext>;
    EGLSurface surface{ EGL_NO_SURFACE };
    EGLContext context{ EGL_NO_CONTEXT };
    EGLDisplay display{ EGL_NO_DISPLAY };

    ~GLContext();
    static EGLConfig findConfig(EGLDisplay display);
    bool makeCurrent();
    void doneCurrent();
    bool create(EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY), EGLContext shareContext = EGL_NO_CONTEXT);
    void destroy();
    operator bool() const { return context != EGL_NO_CONTEXT; }
    static void initModule();
};

}


#define CHECK_GL_ERROR() if(false) {}