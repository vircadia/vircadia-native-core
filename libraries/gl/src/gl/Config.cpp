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

#if defined(Q_OS_WIN)
#elif defined(Q_OS_ANDROID)
#elif defined(Q_OS_MAC)
#include <OpenGL/OpenGL.h>
#include <OpenGL/CGLTypes.h>
#include <OpenGL/CGLCurrent.h>
#include <dlfcn.h>
#else
#include <GL/glx.h>
#include <dlfcn.h>
#endif



#if defined(Q_OS_WIN)

static void* getGlProcessAddress(const char *namez) {
    auto result = wglGetProcAddress(namez);
    if (!result) {
        static HMODULE glModule = nullptr;
        if (!glModule) {
            glModule = LoadLibraryW(L"opengl32.dll");
        }
        result = GetProcAddress(glModule, namez);
    }
    if (!result) {
        OutputDebugStringA(namez);
        OutputDebugStringA("\n");
    }
    return (void*)result;
}

typedef BOOL(APIENTRYP PFNWGLSWAPINTERVALEXTPROC)(int interval);
typedef int (APIENTRYP PFNWGLGETSWAPINTERVALEXTPROC) (void);
typedef BOOL(APIENTRYP PFNWGLCHOOSEPIXELFORMATARBPROC)(HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats);
typedef HGLRC(APIENTRYP PFNWGLCREATECONTEXTATTRIBSARBPROC)(HDC hDC, HGLRC hShareContext, const int *attribList);

PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT;
PFNWGLGETSWAPINTERVALEXTPROC wglGetSwapIntervalEXT;
PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB;
PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB;

#elif defined(Q_OS_ANDROID)

static void* getGlProcessAddress(const char *namez) {
    auto result = eglGetProcAddress(namez);
    return (void*)result;
}

#elif defined(Q_OS_MAC)

static void* getGlProcessAddress(const char *namez) {
    static void* GL_LIB = nullptr;
    if (nullptr == GL_LIB) {
        GL_LIB = dlopen("/System/Library/Frameworks/OpenGL.framework/Versions/Current/OpenGL", RTLD_NOW | RTLD_GLOBAL);
    }
    return dlsym(GL_LIB, namez);
}

#else


typedef Bool (*PFNGLXQUERYCURRENTRENDERERINTEGERMESAPROC) (int attribute, unsigned int *value);
PFNGLXQUERYCURRENTRENDERERINTEGERMESAPROC QueryCurrentRendererIntegerMESA;

static void* getGlProcessAddress(const char *namez) {
    return (void*)glXGetProcAddressARB((const GLubyte*)namez);
}

#endif



void gl::initModuleGl() {
    static std::once_flag once;
    std::call_once(once, [] {
#if defined(Q_OS_WIN)
        wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)getGlProcessAddress("wglSwapIntervalEXT");
        wglGetSwapIntervalEXT = (PFNWGLGETSWAPINTERVALEXTPROC)getGlProcessAddress("wglGetSwapIntervalEXT");
        wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)getGlProcessAddress("wglChoosePixelFormatARB");
        wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)getGlProcessAddress("wglCreateContextAttribsARB");
#endif

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
        QueryCurrentRendererIntegerMESA = (PFNGLXQUERYCURRENTRENDERERINTEGERMESAPROC)getGlProcessAddress("glXQueryCurrentRendererIntegerMESA");
#endif

#if defined(USE_GLES)
        gladLoadGLES2Loader(getGlProcessAddress);
#else
        gladLoadGLLoader(getGlProcessAddress);
#endif
    });
}

int gl::getSwapInterval() {
#if defined(Q_OS_WIN)
    return wglGetSwapIntervalEXT();
#elif defined(Q_OS_MAC)
    GLint interval;
    CGLGetParameter(CGLGetCurrentContext(), kCGLCPSwapInterval, &interval);
    return interval;
#else 
    // TODO: Fill in for linux
    return 1;
#endif
}

void gl::setSwapInterval(int interval) {
#if defined(Q_OS_WIN)
    wglSwapIntervalEXT(interval);
#elif defined(Q_OS_MAC)
    CGLSetParameter(CGLGetCurrentContext(), kCGLCPSwapInterval, &interval);
#elif defined(Q_OS_ANDROID)
    eglSwapInterval(eglGetCurrentDisplay(), interval);
#else
    Q_UNUSED(interval);
#endif
}

bool gl::queryCurrentRendererIntegerMESA(int attr, unsigned int *value) {
    #if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    if (QueryCurrentRendererIntegerMESA) {
        return QueryCurrentRendererIntegerMESA(attr, value);
    }
    #endif

    *value = 0;
    return false;
}
