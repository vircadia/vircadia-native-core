//
//  Created by Bradley Austin Davis on 2016/08/21
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Context.h"

#include <assert.h>

#include <mutex>

#include <QtCore/QDebug>
#include <QtCore/QProcessEnvironment>
#include <QtCore/QThread>

#include <QtGui/QWindow>
#include <QtGui/QGuiApplication>

#include <shared/AbstractLoggerInterface.h>
#include <shared/GlobalAppProperties.h>
#include <GLMHelpers.h>
#include "GLLogging.h"

#ifdef Q_OS_WIN

#ifdef DEBUG
static bool enableDebugLogger = true;
#else
static const QString DEBUG_FLAG("HIFI_DEBUG_OPENGL");
static bool enableDebugLogger = QProcessEnvironment::systemEnvironment().contains(DEBUG_FLAG);
#endif

#endif

#include "Config.h"
#include "GLHelpers.h"

using namespace gl;


std::atomic<size_t> Context::_totalSwapchainMemoryUsage { 0 };

size_t Context::getSwapchainMemoryUsage() { return _totalSwapchainMemoryUsage.load(); }

size_t Context::evalSurfaceMemoryUsage(uint32_t width, uint32_t height, uint32_t pixelSize) {
    return width * height * pixelSize;
}

void Context::updateSwapchainMemoryUsage(size_t prevSize, size_t newSize) {
    if (prevSize == newSize) {
        return;
    }
    if (newSize > prevSize) {
        _totalSwapchainMemoryUsage.fetch_add(newSize - prevSize);
    } else {
        _totalSwapchainMemoryUsage.fetch_sub(prevSize - newSize);
    }
}


Context* Context::PRIMARY = nullptr;

Context::Context() {}

Context::Context(QWindow* window) {
    setWindow(window);
}

#ifdef Q_OS_WIN
void Context::destroyWin32Context(HGLRC hglrc) {
    wglDeleteContext(hglrc);
}
#endif

void Context::release() {
    doneCurrent();
#ifdef Q_OS_WIN
    if (_wrappedContext) {
        destroyContext(_wrappedContext);
        _wrappedContext = nullptr;
    }
    if (_hglrc) {
        destroyWin32Context(_hglrc);
        _hglrc = 0;
    }
    if (_hdc) {
        ReleaseDC(_hwnd, _hdc);
        _hdc = 0;
    }
    _hwnd = 0;
#else
    destroyContext(_context);
    _context = nullptr;
#endif
    _window = nullptr;
    if (PRIMARY == this) {
        PRIMARY = nullptr;
    }
    updateSwapchainMemoryCounter();
}

Context::~Context() {
    release();
}

void Context::updateSwapchainMemoryCounter() {
    if (_window) {
        auto newSize = _window->size();
        auto newMemSize = gl::Context::evalSurfaceMemoryUsage(newSize.width(), newSize.height(), (uint32_t) _swapchainPixelSize);
        gl::Context::updateSwapchainMemoryUsage(_swapchainMemoryUsage, newMemSize);
        _swapchainMemoryUsage = newMemSize;
    } else {
        // No window ? no more swapchain
        gl::Context::updateSwapchainMemoryUsage(_swapchainMemoryUsage, 0);
        _swapchainMemoryUsage = 0;
    }
}

void Context::setWindow(QWindow* window) {
    release();
    _window = window;

#ifdef Q_OS_WIN
    _hwnd = (HWND)window->winId();
#endif

    updateSwapchainMemoryCounter();
}

#ifdef Q_OS_WIN

bool Context::makeCurrent() {
    BOOL result = wglMakeCurrent(_hdc, _hglrc);
    assert(result);
    updateSwapchainMemoryCounter();

    return result;
}

void Context::swapBuffers() {
    SwapBuffers(_hdc);
}

void Context::doneCurrent() {
    wglMakeCurrent(0, 0);
}

void GLAPIENTRY debugMessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
    if (GL_DEBUG_SEVERITY_NOTIFICATION == severity) {
        return;
    }
    qCDebug(glLogging) << "OpenGL: " << message;
    
    // For high severity errors, force a sync to the log, since we might crash 
    // before the log file was flushed otherwise.  Performance hit here
    if (GL_DEBUG_SEVERITY_HIGH == severity) {
        AbstractLoggerInterface* logger = AbstractLoggerInterface::get();
        if (logger) {
            // FIXME find a way to force the log file to sync that doesn't lead to a deadlock
            // logger->sync();
        }
    }
}

// FIXME build the PFD based on the 
static const PIXELFORMATDESCRIPTOR pfd =    // pfd Tells Windows How We Want Things To Be
{
    sizeof(PIXELFORMATDESCRIPTOR),         // Size Of This Pixel Format Descriptor
    1,                                      // Version Number
    PFD_DRAW_TO_WINDOW |                    // Format Must Support Window
    PFD_SUPPORT_OPENGL |                    // Format Must Support OpenGL
    PFD_DOUBLEBUFFER,                       // Must Support Double Buffering
    PFD_TYPE_RGBA,                          // Request An RGBA Format
    24,                                     // Select Our Color Depth
    0, 0, 0, 0, 0, 0,                       // Color Bits Ignored
    1,                                      // Alpha Buffer
    0,                                      // Shift Bit Ignored
    0,                                      // No Accumulation Buffer
    0, 0, 0, 0,                             // Accumulation Bits Ignored
    24,                                     // 24 Bit Z-Buffer (Depth Buffer)  
    8,                                      // 8 Bit Stencil Buffer
    0,                                      // No Auxiliary Buffer
    PFD_MAIN_PLANE,                         // Main Drawing Layer
    0,                                      // Reserved
    0, 0, 0                                 // Layer Masks Ignored
};

void setupPixelFormatSimple(HDC hdc) {
    auto pixelFormat = ChoosePixelFormat(hdc, &pfd);
    if (pixelFormat == 0) {
        throw std::runtime_error("Unable to create initial context");
    }

    if (SetPixelFormat(hdc, pixelFormat, &pfd) == FALSE) {
        throw std::runtime_error("Unable to create initial context");
    }
}

void Context::create() {
    if (!PRIMARY) {
        PRIMARY = static_cast<Context*>(qApp->property(hifi::properties::gl::PRIMARY_CONTEXT).value<void*>());
    }

    if (PRIMARY) {
        _version = PRIMARY->_version;
    }

    assert(0 != _hwnd);
    assert(0 == _hdc);
    auto hwnd = _hwnd;
    // Create a temporary context to initialize glew
    static std::once_flag once;
    std::call_once(once, [&] {
        // If the previous run crashed, force GL debug logging on
        if (qApp->property(hifi::properties::CRASHED).toBool()) {
            enableDebugLogger = true;
        }

        auto hdc = GetDC(hwnd);
        setupPixelFormatSimple(hdc);
        auto glrc = wglCreateContext(hdc);
        BOOL makeCurrentResult;
        makeCurrentResult = wglMakeCurrent(hdc, glrc);
        if (!makeCurrentResult) {
            throw std::runtime_error("Unable to create initial context");
        }
        glewExperimental = true;
        glewInit();
        if (glewIsSupported("GL_VERSION_4_5")) {
            _version = 0x0405;
        } else if (glewIsSupported("GL_VERSION_4_3")) {
            _version = 0x0403;
        }
        glGetError();
        wglMakeCurrent(0, 0);
        wglDeleteContext(glrc);
        ReleaseDC(hwnd, hdc);
    });

    _hdc = GetDC(_hwnd);
    static int pixelFormat = 0;
    static PIXELFORMATDESCRIPTOR pfd;
    if (!pixelFormat) {
        memset(&pfd, 0, sizeof(pfd));
        pfd.nSize = sizeof(pfd);
        std::vector<int> formatAttribs;
        formatAttribs.push_back(WGL_DRAW_TO_WINDOW_ARB);
        formatAttribs.push_back(GL_TRUE);
        formatAttribs.push_back(WGL_SUPPORT_OPENGL_ARB);
        formatAttribs.push_back(GL_TRUE);
        formatAttribs.push_back(WGL_DOUBLE_BUFFER_ARB);
        formatAttribs.push_back(GL_TRUE);
        formatAttribs.push_back(WGL_PIXEL_TYPE_ARB);
        formatAttribs.push_back(WGL_TYPE_RGBA_ARB);
        formatAttribs.push_back(WGL_COLOR_BITS_ARB);
        formatAttribs.push_back(32);
        formatAttribs.push_back(WGL_DEPTH_BITS_ARB);
        formatAttribs.push_back(24);
        formatAttribs.push_back(WGL_STENCIL_BITS_ARB);
        formatAttribs.push_back(8);
#ifdef NATIVE_SRGB_FRAMEBUFFER
     //   formatAttribs.push_back(WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB);
     //   formatAttribs.push_back(GL_TRUE);
#endif
        // terminate the list
        formatAttribs.push_back(0);
        UINT numFormats;
        wglChoosePixelFormatARB(_hdc, &formatAttribs[0], NULL, 1, &pixelFormat, &numFormats);
        DescribePixelFormat(_hdc, pixelFormat, sizeof(pfd), &pfd);
    }
    // The swap chain  pixel size for swap chains is : rgba32 + depth24stencil8
    // We don't apply the length of the swap chain into this pixelSize since it is not vsible for the Process (on windows).
    _swapchainPixelSize = 32 + 32;
    updateSwapchainMemoryCounter();

    SetPixelFormat(_hdc, pixelFormat, &pfd);
    {
        std::vector<int> contextAttribs;
        uint32_t majorVersion = _version >> 8;
        uint32_t minorVersion = _version & 0xFF;
        contextAttribs.push_back(WGL_CONTEXT_MAJOR_VERSION_ARB);
        contextAttribs.push_back(majorVersion);
        contextAttribs.push_back(WGL_CONTEXT_MINOR_VERSION_ARB);
        contextAttribs.push_back(minorVersion);
        contextAttribs.push_back(WGL_CONTEXT_PROFILE_MASK_ARB);
        contextAttribs.push_back(WGL_CONTEXT_CORE_PROFILE_BIT_ARB);
        contextAttribs.push_back(WGL_CONTEXT_FLAGS_ARB);
        if (enableDebugLogger) {
            contextAttribs.push_back(WGL_CONTEXT_DEBUG_BIT_ARB);
        } else {
            contextAttribs.push_back(0);
        }
        contextAttribs.push_back(0);
        auto shareHglrc = PRIMARY ? PRIMARY->_hglrc : 0;
        _hglrc = wglCreateContextAttribsARB(_hdc, shareHglrc, &contextAttribs[0]);
    }

    if (_hglrc == 0) {
        throw std::runtime_error("Could not create GL context");
    }

    if (!PRIMARY) {
        PRIMARY = this;
        qApp->setProperty(hifi::properties::gl::PRIMARY_CONTEXT, QVariant::fromValue((void*)PRIMARY));
    }

    if (enableDebugLogger) {
        makeCurrent();
        glDebugMessageCallback(debugMessageCallback, NULL);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
        doneCurrent();
    }
}
#endif

void Context::clear() {
    glClearColor(0, 0, 0, 1);
    QSize windowSize = _window->size() * _window->devicePixelRatio();
    glViewport(0, 0, windowSize.width(), windowSize.height());
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    swapBuffers();
}


OffscreenContext::~OffscreenContext() {
    _window->deleteLater();
}

void OffscreenContext::create() {
    if (!_window) {
        _window = new QWindow();
        _window->setFlags(Qt::MSWindowsOwnDC);
        _window->setSurfaceType(QSurface::OpenGLSurface);
        _window->create();
        setWindow(_window);
        QSize windowSize = _window->size() * _window->devicePixelRatio();
        qCDebug(glLogging) << "New Offscreen GLContext, window size = " << windowSize.width() << " , " << windowSize.height();
        QGuiApplication::processEvents();
    }
    Parent::create();
}
