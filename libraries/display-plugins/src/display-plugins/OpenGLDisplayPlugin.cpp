//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "OpenGLDisplayPlugin.h"

#include <condition_variable>

#include <QtCore/QCoreApplication>
#include <QtCore/QThread>
#include <QtCore/QTimer>

#include <QtOpenGL/QGLWidget>
#include <QtGui/QOpenGLContext>
#include <QtGui/QImage>

#include <NumericalConstants.h>
#include <DependencyManager.h>
#include <plugins/PluginContainer.h>
#include <gl/Config.h>
#include <gl/GLEscrow.h>
#include <GLMHelpers.h>

class PresentThread : public QThread, public Dependency {
    using Mutex = std::mutex;
    using Condition = std::condition_variable;
    using Lock = std::unique_lock<Mutex>;
public:

    ~PresentThread() {
        _shutdown = true;
        wait(); 
    }

    void setNewDisplayPlugin(OpenGLDisplayPlugin* plugin) {
        Lock lock(_mutex);
        _newPlugin = plugin;
    }

    void setContext(QGLContext * context) {
        // Move the OpenGL context to the present thread
        // Extra code because of the widget 'wrapper' context
        _context = context;
        _context->moveToThread(this);
        _context->contextHandle()->moveToThread(this);
    }

    virtual void run() override {
        Q_ASSERT(_context);
        while (!_shutdown) {
            if (_pendingMainThreadOperation) {
                {
                    Lock lock(_mutex);
                    // Move the context to the main thread
                    _context->moveToThread(qApp->thread());
                    _context->contextHandle()->moveToThread(qApp->thread());
                    _pendingMainThreadOperation = false;
                    // Release the main thread to do it's action
                    _condition.notify_one();
                }


                {
                    // Main thread does it's thing while we wait on the lock to release
                    Lock lock(_mutex);
                    _condition.wait(lock, [&] { return _finishedMainThreadOperation; });
                }
            }

            // Check before lock
            if (_newPlugin != nullptr) {
                Lock lock(_mutex);
                _context->makeCurrent();
                // Check if we have a new plugin to activate
                if (_newPlugin != nullptr) {
                    // Deactivate the old plugin
                    if (_activePlugin != nullptr) {
                        _activePlugin->uncustomizeContext();
                    }

                    _newPlugin->customizeContext();
                    _activePlugin = _newPlugin;
                    _newPlugin = nullptr;
                }
                _context->doneCurrent();
                lock.unlock();
            }

            // If there's no active plugin, just sleep
            if (_activePlugin == nullptr) {
                QThread::usleep(100);
                continue;
            }

            // take the latest texture and present it
            _context->makeCurrent();
            _activePlugin->present();
            _context->doneCurrent();
        }

        _context->doneCurrent();
        _context->moveToThread(qApp->thread());
        _context->contextHandle()->moveToThread(qApp->thread());
    }

    void withMainThreadContext(std::function<void()> f) {
        // Signal to the thread that there is work to be done on the main thread
        Lock lock(_mutex);
        _pendingMainThreadOperation = true;
        _finishedMainThreadOperation = false;
        _condition.wait(lock, [&] { return !_pendingMainThreadOperation; });

        _context->makeCurrent();
        f();
        _context->doneCurrent();

        // Move the context back to the presentation thread
        _context->moveToThread(this);
        _context->contextHandle()->moveToThread(this);

        // restore control of the context to the presentation thread and signal 
        // the end of the operation
        _finishedMainThreadOperation = true;
        lock.unlock();
        _condition.notify_one();
    }


private:
    void makeCurrent();
    void doneCurrent();

    bool _shutdown { false };
    Mutex _mutex;
    // Used to allow the main thread to perform context operations
    Condition _condition;
    bool _pendingMainThreadOperation { false };
    bool _finishedMainThreadOperation { false };
    QThread* _mainThread { nullptr };
    OpenGLDisplayPlugin* _newPlugin { nullptr };
    OpenGLDisplayPlugin* _activePlugin { nullptr };
    QGLContext* _context { nullptr };
};

bool OpenGLDisplayPlugin::_vsyncSupported = false;

OpenGLDisplayPlugin::OpenGLDisplayPlugin() {
    _sceneTextureEscrow.setRecycler([this](GLuint texture){
        cleanupForSceneTexture(texture);
        _container->releaseSceneTexture(texture);
    });

    _overlayTextureEscrow.setRecycler([this](GLuint texture) {
        _container->releaseOverlayTexture(texture);
    });

    connect(&_timer, &QTimer::timeout, this, [&] {
        if (_active && _sceneTextureEscrow.depth() < 1) {
            emit requestRender();
        }
    });
}

void OpenGLDisplayPlugin::cleanupForSceneTexture(uint32_t sceneTexture) {
    Lock lock(_mutex);
    Q_ASSERT(_sceneTextureToFrameIndexMap.contains(sceneTexture));
    _sceneTextureToFrameIndexMap.remove(sceneTexture);
}


void OpenGLDisplayPlugin::activate() {
    _timer.start(2);

    // Start the present thread if necessary
    auto presentThread = DependencyManager::get<PresentThread>();
    if (!presentThread) {
        auto widget = _container->getPrimaryWidget();

        // TODO: write the proper code for linux
#if defined(Q_OS_WIN)
        widget->makeCurrent();
        _vsyncSupported = wglewGetExtension("WGL_EXT_swap_control");
        widget->doneCurrent();
#endif

        DependencyManager::set<PresentThread>();
        presentThread = DependencyManager::get<PresentThread>();
        presentThread->setObjectName("Presentation Thread");
        presentThread->setContext(widget->context());
        // Start execution
        presentThread->start();
    }
    presentThread->setNewDisplayPlugin(this);
    DisplayPlugin::activate();
    emit requestRender();
}

void OpenGLDisplayPlugin::stop() {
    _timer.stop();
}

void OpenGLDisplayPlugin::deactivate() {
    _timer.stop();
    DisplayPlugin::deactivate();
}

void OpenGLDisplayPlugin::customizeContext() {
    auto presentThread = DependencyManager::get<PresentThread>();
    Q_ASSERT(thread() == presentThread->thread());

    enableVsync();

    using namespace oglplus;
    Context::BlendFunc(BlendFunction::SrcAlpha, BlendFunction::OneMinusSrcAlpha);
    Context::Disable(Capability::Blend);
    Context::Disable(Capability::DepthTest);
    Context::Disable(Capability::CullFace);

    _program = loadDefaultShader();
    _plane = loadPlane(_program);
}

void OpenGLDisplayPlugin::uncustomizeContext() {
    _program.reset();
    _plane.reset();
}

// Pressing Alt (and Meta) key alone activates the menubar because its style inherits the
// SHMenuBarAltKeyNavigation from QWindowsStyle. This makes it impossible for a scripts to
// receive keyPress events for the Alt (and Meta) key in a reliable manner.
//
// This filter catches events before QMenuBar can steal the keyboard focus.
// The idea was borrowed from
// http://www.archivum.info/qt-interest@trolltech.com/2006-09/00053/Re-(Qt4)-Alt-key-focus-QMenuBar-(solved).html

// Pass input events on to the application
bool OpenGLDisplayPlugin::eventFilter(QObject* receiver, QEvent* event) {
    switch (event->type()) {
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseButtonDblClick:
        case QEvent::MouseMove:
        case QEvent::Wheel:

        case QEvent::TouchBegin:
        case QEvent::TouchEnd:
        case QEvent::TouchUpdate:

        case QEvent::FocusIn:
        case QEvent::FocusOut:

        case QEvent::KeyPress:
        case QEvent::KeyRelease:
        case QEvent::ShortcutOverride:

        case QEvent::DragEnter:
        case QEvent::Drop:

        case QEvent::Resize:
            if (QCoreApplication::sendEvent(QCoreApplication::instance(), event)) {
                return true;
            }
            break;
        default:
            break;
    }
    return false;
}

void OpenGLDisplayPlugin::submitSceneTexture(uint32_t frameIndex, uint32_t sceneTexture, const glm::uvec2& sceneSize) {
    {
        Lock lock(_mutex);
        _sceneTextureToFrameIndexMap[sceneTexture] = frameIndex;
    }

    // Submit it to the presentation thread via escrow
    _sceneTextureEscrow.submit(sceneTexture);
}

void OpenGLDisplayPlugin::submitOverlayTexture(GLuint sceneTexture, const glm::uvec2& sceneSize) {
    // Submit it to the presentation thread via escrow
    _overlayTextureEscrow.submit(sceneTexture);
}

void OpenGLDisplayPlugin::updateTextures() {
    _currentSceneTexture = _sceneTextureEscrow.fetchAndRelease(_currentSceneTexture);
    _currentOverlayTexture = _overlayTextureEscrow.fetchAndRelease(_currentOverlayTexture);
}

void OpenGLDisplayPlugin::updateFramerate() {
    uint64_t now = usecTimestampNow();
    static uint64_t lastSwapEnd { now };
    uint64_t diff = now - lastSwapEnd;
    lastSwapEnd = now;
    if (diff != 0) {
        Lock lock(_mutex);
        _usecsPerFrame.updateAverage(diff);
    }
}


void OpenGLDisplayPlugin::internalPresent() {
    using namespace oglplus;
    uvec2 size = getSurfacePixels();
    Context::Viewport(size.x, size.y);
    Context::Clear().DepthBuffer();
    glBindTexture(GL_TEXTURE_2D, _currentSceneTexture);
    drawUnitQuad();
    swapBuffers();
    updateFramerate();
}

void OpenGLDisplayPlugin::present() {
    updateTextures();
    if (_currentSceneTexture) {
        internalPresent();
        updateFramerate();
    }
}

float OpenGLDisplayPlugin::presentRate() {
    float result { -1.0f }; 
    {
        Lock lock(_mutex);
        result = _usecsPerFrame.getAverage();
        result = 1.0f / result; 
        result *= USECS_PER_SECOND;
    }
    return result;
}

void OpenGLDisplayPlugin::drawUnitQuad() {
    _program->Bind();
    _plane->Draw();
}

void OpenGLDisplayPlugin::enableVsync(bool enable) {
    if (!_vsyncSupported) {
        return;
    }
#ifdef Q_OS_WIN
    wglSwapIntervalEXT(enable ? 1 : 0);
#endif
}

bool OpenGLDisplayPlugin::isVsyncEnabled() {
    if (!_vsyncSupported) {
        return true;
    }
#ifdef Q_OS_WIN
    return wglGetSwapIntervalEXT() != 0;
#else
    return true;
#endif
}

void OpenGLDisplayPlugin::swapBuffers() {
    static auto widget = _container->getPrimaryWidget();
    widget->swapBuffers();
}

void OpenGLDisplayPlugin::withMainThreadContext(std::function<void()> f) const {
    static auto presentThread = DependencyManager::get<PresentThread>();
    presentThread->withMainThreadContext(f);
    _container->makeRenderingContextCurrent();
}

QImage OpenGLDisplayPlugin::getScreenshot() const {
    QImage result;
    withMainThreadContext([&] {
        static auto widget = _container->getPrimaryWidget();
        result = widget->grabFrameBuffer();
    });
    return result;
}
