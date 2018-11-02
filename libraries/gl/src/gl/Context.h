//
//  Created by Bradley Austin Davis on 2016/08/21
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_gl_context_h
#define hifi_gl_context_h

#include <stdint.h>
#include <QtGlobal>
#include <atomic>

#if defined(Q_OS_WIN)
#include <Windows.h>
#endif

class QSurface;
class QWindow;
class QOpenGLContext;
class QThread;
class QOpenGLDebugMessage;

#if defined(Q_OS_WIN) 
#define GL_CUSTOM_CONTEXT
#endif
namespace gl {

    class Context {
    protected:
        QWindow* _window { nullptr };
        static void destroyContext(QOpenGLContext* context);
#if defined(GL_CUSTOM_CONTEXT)
        uint32_t _version { 0x0401 };
        HWND _hwnd { 0 };
        HDC _hdc { 0 };
        HGLRC _hglrc { 0 };
        static void destroyWin32Context(HGLRC hglrc);
        QOpenGLContext* _wrappedContext { nullptr };
#else
        QOpenGLContext* _context { nullptr };
#endif
   
    private:
        Context(const Context& other);

    public:
        static bool enableDebugLogger();
        static void debugMessageHandler(const QOpenGLDebugMessage &debugMessage);
        static void setupDebugLogging(QOpenGLContext* context);
        
        Context();
        Context(QWindow* window);
        void release();
        virtual ~Context();

        void clear();
        void setWindow(QWindow* window);
        bool makeCurrent();
        static void makeCurrent(QOpenGLContext* context, QSurface* surface);
        void swapBuffers();
        void doneCurrent();
        virtual void create(QOpenGLContext* shareContext = nullptr);
        QOpenGLContext* qglContext();
        void moveToThread(QThread* thread);

        static size_t evalSurfaceMemoryUsage(uint32_t width, uint32_t height, uint32_t pixelSize);
        static size_t getSwapchainMemoryUsage();
        static void updateSwapchainMemoryUsage(size_t prevSize, size_t newSize);
        
     private:
        static std::atomic<size_t> _totalSwapchainMemoryUsage;

        size_t _swapchainMemoryUsage { 0 };
        size_t _swapchainPixelSize { 0 };
        void updateSwapchainMemoryCounter();
    };

    class OffscreenContext : public Context {
        using Parent = Context;
    protected:
        QWindow* _window { nullptr };
    public:
        virtual ~OffscreenContext();
        void create(QOpenGLContext* shareContext = nullptr) override;
    };

}

#endif // hifi_gpu_GPUConfig_h
