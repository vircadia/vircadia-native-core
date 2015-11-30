//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "OpenGLDisplayPlugin.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QThread>
#include <QtCore/QTimer>

#include <QtOpenGL/QGLWidget>
#include <QtGui/QOpenGLContext>

#include <NumericalConstants.h>
#include <DependencyManager.h>
#include <plugins/PluginContainer.h>
#include <gl/Config.h>
#include <gl/GLEscrow.h>
#include <GLMHelpers.h>

class PresentThread : public QThread, public Dependency {
    using Mutex = std::mutex;
    using Lock = std::unique_lock<Mutex>;
    friend class OpenGLDisplayPlugin;
public:

    ~PresentThread() {
        _shutdown = true;
        wait(); 
    }

    void setNewDisplayPlugin(OpenGLDisplayPlugin* plugin) {
        Lock lock(_mutex);
        _newPlugin = plugin;
    }

    virtual void run() override {
        Q_ASSERT(_context);
        while (!_shutdown) {
            // Check before lock
            if (_newPlugin != nullptr) {
                Lock lock(_mutex);
                // Check if we have a new plugin to activate
                if (_newPlugin != nullptr) {
                    // Deactivate the old plugin
                    if (_activePlugin != nullptr) {
                        _activePlugin->uncustomizeContext();
                    }

                    _newPlugin->customizeContext();
                    _activePlugin = _newPlugin;
                    _newPlugin = nullptr;
                    _context->doneCurrent();
                }
                lock.unlock();
            }

            // If there's no active plugin, just sleep
            if (_activePlugin == nullptr) {
                QThread::usleep(100);
                continue;
            }

            // take the latest texture and present it
            _activePlugin->present();

        }
        _context->doneCurrent();
        _context->moveToThread(qApp->thread());
    }


private:
    bool _shutdown { false };
    Mutex _mutex;
    QThread* _mainThread { nullptr };
    OpenGLDisplayPlugin* _newPlugin { nullptr };
    OpenGLDisplayPlugin* _activePlugin { nullptr };
    QOpenGLContext* _context { nullptr };
};

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
        DependencyManager::set<PresentThread>();
        presentThread = DependencyManager::get<PresentThread>();
        auto widget = _container->getPrimaryWidget();
        auto glContext = widget->context();
        auto context = glContext->contextHandle();
        glContext->moveToThread(presentThread.data());
        context->moveToThread(presentThread.data());

        // Move the OpenGL context to the present thread
        presentThread->_context = context;

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

    bool makeCurrentResult = makeCurrent();
    Q_ASSERT(makeCurrentResult);

    // TODO: write the proper code for linux
#if defined(Q_OS_WIN)
    _vsyncSupported = wglewGetExtension("WGL_EXT_swap_control");
#endif
    enableVsync();

    using namespace oglplus;
    Context::BlendFunc(BlendFunction::SrcAlpha, BlendFunction::OneMinusSrcAlpha);
    Context::Disable(Capability::Blend);
    Context::Disable(Capability::DepthTest);
    Context::Disable(Capability::CullFace);

    _program = loadDefaultShader();
    _plane = loadPlane(_program);

    doneCurrent();
}

void OpenGLDisplayPlugin::uncustomizeContext() {
    makeCurrent();
    _program.reset();
    _plane.reset();
    doneCurrent();
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
    auto makeCurrentResult = makeCurrent();
    Q_ASSERT(makeCurrentResult);
    if (!makeCurrentResult) {
        qDebug() << "Failed to make current";
        return;
    }

    updateTextures();
    if (_currentSceneTexture) {
        internalPresent();
        updateFramerate();
    }
    doneCurrent();
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
bool OpenGLDisplayPlugin::makeCurrent() {
    static auto widget = _container->getPrimaryWidget();
    widget->makeCurrent();
    auto result = widget->context()->contextHandle() == QOpenGLContext::currentContext();
    Q_ASSERT(result);
    return result;
}

void OpenGLDisplayPlugin::doneCurrent() {
    static auto widget = _container->getPrimaryWidget();
    widget->doneCurrent();
}

void OpenGLDisplayPlugin::swapBuffers() {
    static auto widget = _container->getPrimaryWidget();
    widget->swapBuffers();
}
