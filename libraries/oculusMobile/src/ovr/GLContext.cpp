//
//  Created by Bradley Austin Davis on 2018/11/15
//  Copyright 2013-2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "GLContext.h"

#include <array>
#include <vector>
#include <mutex>

#include <android/log.h>
#include <EGL/eglext.h>

using namespace ovr;

static void* getGlProcessAddress(const char *namez) {
    auto result = eglGetProcAddress(namez);
    return (void*)result;
}


void GLContext::initModule() {
    static std::once_flag once;
    std::call_once(once, [&]{
        gladLoadGLES2Loader(getGlProcessAddress);
    });
}

void APIENTRY debugMessageCallback(GLenum source,
                                   GLenum type,
                                   GLuint id,
                                   GLenum severity,
                                   GLsizei length,
                                   const GLchar* message,
                                   const void* userParam) {
    if (type == GL_DEBUG_TYPE_PERFORMANCE_KHR) {
        return;
    }
    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH:
        case GL_DEBUG_SEVERITY_MEDIUM:
            break;
        default:
            return;
    }

    __android_log_write(ANDROID_LOG_WARN, "QQQ_GL", message);
}

GLContext::~GLContext() {
    destroy();
}

EGLConfig GLContext::findConfig(EGLDisplay display) {
    // Do NOT use eglChooseConfig, because the Android EGL code pushes in multisample
    // flags in eglChooseConfig if the user has selected the "force 4x MSAA" option in
    // settings, and that is completely wasted for our warp target.
    std::vector<EGLConfig> configs;
    {
        const int MAX_CONFIGS = 1024;
        EGLConfig configsBuffer[MAX_CONFIGS];
        EGLint numConfigs = 0;
        if (eglGetConfigs(display, configsBuffer, MAX_CONFIGS, &numConfigs) == EGL_FALSE) {
            __android_log_print(ANDROID_LOG_WARN, "QQQ_GL", "Failed to fetch configs");
            return 0;
        }
        configs.resize(numConfigs);
        memcpy(configs.data(), configsBuffer, sizeof(EGLConfig) * numConfigs);
    }

    std::vector<std::pair<EGLint, EGLint>> configAttribs{
        { EGL_RED_SIZE, 8 },   { EGL_GREEN_SIZE, 8 },   { EGL_BLUE_SIZE, 8 }, { EGL_ALPHA_SIZE, 8 },
        { EGL_DEPTH_SIZE, 0 }, { EGL_STENCIL_SIZE, 0 }, { EGL_SAMPLES, 0 },
    };

    auto matchAttrib = [&](EGLConfig config, const std::pair<EGLint, EGLint>& attribAndValue) {
        EGLint value = 0;
        eglGetConfigAttrib(display, config, attribAndValue.first, &value);
        return (attribAndValue.second == value);
    };

    auto matchAttribFlags = [&](EGLConfig config, const std::pair<EGLint, EGLint>& attribAndValue) {
        EGLint value = 0;
        eglGetConfigAttrib(display, config, attribAndValue.first, &value);
        return (value & attribAndValue.second) == attribAndValue.second;
    };

    auto matchConfig = [&](EGLConfig config) {
        if (!matchAttribFlags(config, { EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT_KHR})) {
            return false;
        }
        // The pbuffer config also needs to be compatible with normal window rendering
        // so it can share textures with the window context.
        if (!matchAttribFlags(config, { EGL_SURFACE_TYPE, EGL_WINDOW_BIT | EGL_PBUFFER_BIT})) {
            return false;
        }

        for (const auto& attrib : configAttribs) {
            if (!matchAttrib(config, attrib)) {
                return false;
            }
        }

        return true;
    };


    for (const auto& config : configs) {
        if (matchConfig(config)) {
            return config;
        }
    }

    return 0;
}

bool GLContext::makeCurrent() {
    return eglMakeCurrent(display, surface, surface, context) != EGL_FALSE;
}

void GLContext::doneCurrent() {
    eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
}

bool GLContext::create(EGLDisplay display, EGLContext shareContext, bool noError) {
    this->display = display;

    auto config = findConfig(display);

    if (config == 0) {
        __android_log_print(ANDROID_LOG_WARN, "QQQ_GL", "Failed eglChooseConfig");
        return false;
    }

    EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 3,
                                noError ? EGL_CONTEXT_OPENGL_NO_ERROR_KHR : EGL_NONE, EGL_TRUE,
                                EGL_NONE };

    context = eglCreateContext(display, config, shareContext, contextAttribs);
    if (context == EGL_NO_CONTEXT) {
        __android_log_print(ANDROID_LOG_WARN, "QQQ_GL", "Failed eglCreateContext");
        return false;
    }

    const EGLint surfaceAttribs[] = { EGL_WIDTH, 16, EGL_HEIGHT, 16, EGL_NONE };
    surface = eglCreatePbufferSurface(display, config, surfaceAttribs);
    if (surface == EGL_NO_SURFACE) {
        __android_log_print(ANDROID_LOG_WARN, "QQQ_GL", "Failed eglCreatePbufferSurface");
        return false;
    }

    if (!makeCurrent()) {
        __android_log_print(ANDROID_LOG_WARN, "QQQ_GL", "Failed eglMakeCurrent");
        return false;
    }

    ovr::GLContext::initModule();

#ifndef NDEBUG
    glDebugMessageCallback(debugMessageCallback, this);
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
#endif
    return true;
}

void GLContext::destroy() {
    if (context != EGL_NO_CONTEXT) {
        eglDestroyContext(display, context);
        context = EGL_NO_CONTEXT;
    }

    if (surface != EGL_NO_SURFACE) {
        eglDestroySurface(display, surface);
        surface = EGL_NO_SURFACE;
    }
}
