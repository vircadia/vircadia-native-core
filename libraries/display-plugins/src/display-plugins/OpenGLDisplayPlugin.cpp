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
#include <QtGui/QImage>

#if defined(Q_OS_MAC)
#include <OpenGL/CGLCurrent.h>
#endif
#include <gl/QOpenGLContextWrapper.h>
#include <gpu/Texture.h>
#include <gl/GLWidget.h>
#include <NumericalConstants.h>
#include <DependencyManager.h>
#include <shared/NsightHelpers.h>
#include <ui-plugins/PluginContainer.h>
#include <gl/Config.h>
#include <gl/GLEscrow.h>
#include <GLMHelpers.h>
#include <CursorManager.h>
#include "CompositorHelper.h"
#include <ui/Menu.h>


#if THREADED_PRESENT

// FIXME, for display plugins that don't block on something like vsync, just 
// cap the present rate at 200
// const static unsigned int MAX_PRESENT_RATE = 200;

class PresentThread : public QThread, public Dependency {
    using Mutex = std::mutex;
    using Condition = std::condition_variable;
    using Lock = std::unique_lock<Mutex>;
public:

    PresentThread() {
        connect(qApp, &QCoreApplication::aboutToQuit, [this] {
            shutdown();
        });
    }

    ~PresentThread() {
        shutdown();
    }

    void shutdown() {
        if (isRunning()) {
            // First ensure that we turn off any current display plugin
            setNewDisplayPlugin(nullptr);

            Lock lock(_mutex);
            _shutdown = true;
            _condition.wait(lock, [&] { return !_shutdown;  });
            qDebug() << "Present thread shutdown";
        }
    }


    void setNewDisplayPlugin(OpenGLDisplayPlugin* plugin) {
        Lock lock(_mutex);
        if (isRunning()) {
            _newPluginQueue.push(plugin);
            _condition.wait(lock, [=]()->bool { return _newPluginQueue.empty(); });
        }
    }

    void setContext(QGLContext * context) {
        // Move the OpenGL context to the present thread
        // Extra code because of the widget 'wrapper' context
        _context = context;
        _context->moveToThread(this);
    }


    virtual void run() override {
        OpenGLDisplayPlugin* currentPlugin{ nullptr };
        thread()->setPriority(QThread::HighestPriority);
        Q_ASSERT(_context);
        while (!_shutdown) {
            if (_pendingMainThreadOperation) {
                {
                    Lock lock(_mutex);
                    // Move the context to the main thread
                    _context->moveToThread(qApp->thread());
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

            // Check for a new display plugin
            {
                Lock lock(_mutex);
                _context->makeCurrent();
                // Check if we have a new plugin to activate
                while (!_newPluginQueue.empty()) {
                    auto newPlugin = _newPluginQueue.front();
                    if (newPlugin != currentPlugin) {
                        // Deactivate the old plugin
                        if (currentPlugin != nullptr) {
                            try {
                                currentPlugin->uncustomizeContext();
                            } catch (const oglplus::Error& error) {
                                qWarning() << "OpenGL error in uncustomizeContext: " << error.what();
                            }
                        }

                        if (newPlugin) {
                            try {
                                newPlugin->customizeContext();
                            } catch (const oglplus::Error& error) {
                                qWarning() << "OpenGL error in customizeContext: " << error.what();
                            }
                        }
                        currentPlugin = newPlugin;
                        _newPluginQueue.pop();
                        _condition.notify_one();
                    }
                }
                _context->doneCurrent();
            }

            // If there's no active plugin, just sleep
            if (currentPlugin == nullptr) {
                // Minimum sleep ends up being about 2 ms anyway
                QThread::msleep(1);
                continue;
            }

            // take the latest texture and present it
            _context->makeCurrent();
            if (isCurrentContext(_context->contextHandle())) {
                try {
                    currentPlugin->present();
                } catch (const oglplus::Error& error) {
                    qWarning() << "OpenGL error in presentation: " << error.what();
                }
                _context->doneCurrent();
            } else {
                qWarning() << "Makecurrent failed";
            }
        }

        Lock lock(_mutex);
        _context->moveToThread(qApp->thread());
        _shutdown = false;
        _condition.notify_one();
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
    std::queue<OpenGLDisplayPlugin*> _newPluginQueue;
    QGLContext* _context { nullptr };
};

#endif


OpenGLDisplayPlugin::OpenGLDisplayPlugin() {
    _sceneTextureEscrow.setRecycler([this](const gpu::TexturePointer& texture){
        cleanupForSceneTexture(texture);
        _container->releaseSceneTexture(texture);
    });
    _overlayTextureEscrow.setRecycler([this](const gpu::TexturePointer& texture) {
        _container->releaseOverlayTexture(texture);
    });
}

void OpenGLDisplayPlugin::cleanupForSceneTexture(const gpu::TexturePointer& sceneTexture) {
    withRenderThreadLock([&] {
        Q_ASSERT(_sceneTextureToFrameIndexMap.contains(sceneTexture));
        _sceneTextureToFrameIndexMap.remove(sceneTexture);
    });
}


bool OpenGLDisplayPlugin::activate() {
    if (!_cursorsData.size()) {
        auto& cursorManager = Cursor::Manager::instance();
        for (const auto iconId : cursorManager.registeredIcons()) {
            auto& cursorData = _cursorsData[iconId];
            auto iconPath = cursorManager.getIconImage(iconId);
            auto image = QImage(iconPath);
            image = image.mirrored();
            image = image.convertToFormat(QImage::Format_RGBA8888);
            cursorData.image = image;
            cursorData.size = toGlm(image.size());
            cursorData.hotSpot = vec2(0.5f);
        }
    }
    if (!_container) {
        return false;
    }
    _vsyncSupported = _container->getPrimaryWidget()->isVsyncSupported();

#if THREADED_PRESENT
    // Start the present thread if necessary
    QSharedPointer<PresentThread> presentThread;
    if (DependencyManager::isSet<PresentThread>()) {
        presentThread = DependencyManager::get<PresentThread>();
    } else {
        auto widget = _container->getPrimaryWidget();
        DependencyManager::set<PresentThread>();
        presentThread = DependencyManager::get<PresentThread>();
        presentThread->setObjectName("Presentation Thread");
        presentThread->setContext(widget->context());
        // Start execution
        presentThread->start();
    }
    _presentThread = presentThread.data();
#endif
    
    // Child classes may override this in order to do things like initialize
    // libraries, etc
    if (!internalActivate()) {
        return false;
    }

#if THREADED_PRESENT

    // This should not return until the new context has been customized
    // and the old context (if any) has been uncustomized
    presentThread->setNewDisplayPlugin(this);
#else
    static auto widget = _container->getPrimaryWidget();
    widget->makeCurrent();
    customizeContext();
    _container->makeRenderingContextCurrent();
#endif

    auto compositorHelper = DependencyManager::get<CompositorHelper>();
    connect(compositorHelper.data(), &CompositorHelper::alphaChanged, [this] {
        auto compositorHelper = DependencyManager::get<CompositorHelper>();
        auto animation = new QPropertyAnimation(this, "overlayAlpha");
        animation->setDuration(200);
        animation->setEndValue(compositorHelper->getAlpha());
        animation->start();
    });

    if (isHmd() && (getHmdScreen() >= 0)) {
        _container->showDisplayPluginsTools();
    }

    return Parent::activate();
}

void OpenGLDisplayPlugin::deactivate() {
    
    auto compositorHelper = DependencyManager::get<CompositorHelper>();
    disconnect(compositorHelper.data());

#if THREADED_PRESENT
    auto presentThread = DependencyManager::get<PresentThread>();
    // Does not return until the GL transition has completeed
    presentThread->setNewDisplayPlugin(nullptr);
#else
    static auto widget = _container->getPrimaryWidget();
    widget->makeCurrent();
    uncustomizeContext();
    _container->makeRenderingContextCurrent();
#endif
    internalDeactivate();

    _container->showDisplayPluginsTools(false);
    if (!_container->currentDisplayActions().isEmpty()) {
        auto menu = _container->getPrimaryMenu();
        foreach(auto itemInfo, _container->currentDisplayActions()) {
            menu->removeMenuItem(itemInfo.first, itemInfo.second);
        }
        _container->currentDisplayActions().clear();
    }
    Parent::deactivate();
}


void OpenGLDisplayPlugin::customizeContext() {
#if THREADED_PRESENT
    auto presentThread = DependencyManager::get<PresentThread>();
    Q_ASSERT(thread() == presentThread->thread());
#endif
    enableVsync();

    for (auto& cursorValue : _cursorsData) {
        auto& cursorData = cursorValue.second;
        if (!cursorData.texture) {
            const auto& image = cursorData.image;
            glGenTextures(1, &cursorData.texture);
            glBindTexture(GL_TEXTURE_2D, cursorData.texture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width(), image.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, image.constBits());
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glGenerateMipmap(GL_TEXTURE_2D);
        }
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    using namespace oglplus;
    Context::BlendFunc(BlendFunction::SrcAlpha, BlendFunction::OneMinusSrcAlpha);
    Context::Disable(Capability::Blend);
    Context::Disable(Capability::DepthTest);
    Context::Disable(Capability::CullFace);

    _program = loadDefaultShader();

    auto uniforms = _program->ActiveUniforms();
    while (!uniforms.Empty()) {
        auto uniform = uniforms.Front();
        if (uniform.Name() == "mvp") {
            _mvpUniform = uniform.Index();
        }
        if (uniform.Name() == "alpha") {
            _alphaUniform = uniform.Index();
        }
        uniforms.Next();
    }
  
    _plane = loadPlane(_program);

    _compositeFramebuffer = std::make_shared<BasicFramebufferWrapper>();
    _compositeFramebuffer->Init(getRecommendedRenderSize());
}

void OpenGLDisplayPlugin::uncustomizeContext() {
    _compositeFramebuffer.reset();
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

void OpenGLDisplayPlugin::submitSceneTexture(uint32_t frameIndex, const gpu::TexturePointer& sceneTexture) {
    if (_lockCurrentTexture) {
        _container->releaseSceneTexture(sceneTexture);
        return;
    }

    withRenderThreadLock([&] {
        _sceneTextureToFrameIndexMap[sceneTexture] = frameIndex;
    });

    // Submit it to the presentation thread via escrow
    _sceneTextureEscrow.submit(sceneTexture);

#if THREADED_PRESENT
#else
    static auto widget = _container->getPrimaryWidget();
    widget->makeCurrent();
    present();
    _container->makeRenderingContextCurrent();
#endif
}

void OpenGLDisplayPlugin::submitOverlayTexture(const gpu::TexturePointer& overlayTexture) {
    // Submit it to the presentation thread via escrow
    _overlayTextureEscrow.submit(overlayTexture);
}

void OpenGLDisplayPlugin::updateTextures() {
    // FIXME intrduce a GPU wait instead of a CPU/GPU sync point?
#if THREADED_PRESENT
    if (_sceneTextureEscrow.fetchSignaledAndRelease(_currentSceneTexture)) {
#else
    if (_sceneTextureEscrow.fetchAndReleaseWithGpuWait(_currentSceneTexture)) {
#endif
        updateFrameData();
        _newFrameRate.increment();
    } 

    _overlayTextureEscrow.fetchSignaledAndRelease(_currentOverlayTexture);
}

void OpenGLDisplayPlugin::updateFrameData() {
    withPresentThreadLock([&] {
        auto previousFrameIndex = _currentPresentFrameIndex;
        _currentPresentFrameIndex = _sceneTextureToFrameIndexMap[_currentSceneTexture];
        auto skippedCount = (_currentPresentFrameIndex - previousFrameIndex) - 1;
        _droppedFrameRate.increment(skippedCount);
    });
}

void OpenGLDisplayPlugin::compositeOverlay() {
    using namespace oglplus;

    auto compositorHelper = DependencyManager::get<CompositorHelper>();

    useProgram(_program);
    // set the alpha
    Uniform<float>(*_program, _alphaUniform).Set(_compositeOverlayAlpha);
    // check the alpha
    // Overlay draw
    if (isStereo()) {
        Uniform<glm::mat4>(*_program, _mvpUniform).Set(mat4());
        for_each_eye([&](Eye eye) {
            eyeViewport(eye);
            drawUnitQuad();
        });
    } else {
        // Overlay draw
        Uniform<glm::mat4>(*_program, _mvpUniform).Set(mat4());
        drawUnitQuad();
    }
    // restore the alpha
    Uniform<float>(*_program, _alphaUniform).Set(1.0);
}

void OpenGLDisplayPlugin::compositePointer() {
    using namespace oglplus;
    auto compositorHelper = DependencyManager::get<CompositorHelper>();

    useProgram(_program);
    // set the alpha
    Uniform<float>(*_program, _alphaUniform).Set(_compositeOverlayAlpha);
    Uniform<glm::mat4>(*_program, _mvpUniform).Set(compositorHelper->getReticleTransform(glm::mat4()));
    if (isStereo()) {
        for_each_eye([&](Eye eye) {
            eyeViewport(eye);
            drawUnitQuad();
        });
    } else {
        drawUnitQuad();
    }
    Uniform<glm::mat4>(*_program, _mvpUniform).Set(mat4());
    // restore the alpha
    Uniform<float>(*_program, _alphaUniform).Set(1.0);
}

void OpenGLDisplayPlugin::compositeScene() {
    using namespace oglplus;
    useProgram(_program);
    Uniform<glm::mat4>(*_program, _mvpUniform).Set(mat4());
    drawUnitQuad();
}

void OpenGLDisplayPlugin::compositeLayers() {
    using namespace oglplus;
    auto targetRenderSize = getRecommendedRenderSize();
    if (!_compositeFramebuffer || _compositeFramebuffer->size != targetRenderSize) {
        _compositeFramebuffer = std::make_shared<BasicFramebufferWrapper>();
        _compositeFramebuffer->Init(targetRenderSize);
    }
    _compositeFramebuffer->Bound(Framebuffer::Target::Draw, [&] {
        Context::Viewport(targetRenderSize.x, targetRenderSize.y);
        auto sceneTextureId = getSceneTextureId();
        auto overlayTextureId = getOverlayTextureId();
        glBindTexture(GL_TEXTURE_2D, sceneTextureId);
        compositeScene();
        if (overlayTextureId) {
            glBindTexture(GL_TEXTURE_2D, overlayTextureId);
            Context::Enable(Capability::Blend);
            Context::BlendFunc(BlendFunction::SrcAlpha, BlendFunction::OneMinusSrcAlpha);
            compositeOverlay();

            auto compositorHelper = DependencyManager::get<CompositorHelper>();
            if (compositorHelper->getReticleVisible()) {
                auto& cursorManager = Cursor::Manager::instance();
                const auto& cursorData = _cursorsData[cursorManager.getCursor()->getIcon()];
                glBindTexture(GL_TEXTURE_2D, cursorData.texture);
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, overlayTextureId);
                compositePointer();
                glBindTexture(GL_TEXTURE_2D, 0);
                glActiveTexture(GL_TEXTURE0);
            }
            glBindTexture(GL_TEXTURE_2D, 0);
            Context::Disable(Capability::Blend);
        }
        compositeExtra();
    });
}

void OpenGLDisplayPlugin::internalPresent() {
    using namespace oglplus;
    const uvec2& srcSize = _compositeFramebuffer->size;
    uvec2 dstSize = getSurfacePixels();
    _compositeFramebuffer->Bound(FramebufferTarget::Read, [&] {
        Context::BlitFramebuffer(
            0, 0, srcSize.x, srcSize.y,
            0, 0, dstSize.x, dstSize.y,
            BufferSelectBit::ColorBuffer, BlitFilter::Nearest);
    });
    swapBuffers();
}

void OpenGLDisplayPlugin::present() {
    incrementPresentCount();

    PROFILE_RANGE_EX(__FUNCTION__, 0xff00ff00, (uint64_t)presentCount())

    updateTextures();
    if (_currentSceneTexture) {
        // Write all layers to a local framebuffer
        compositeLayers();
        // Take the composite framebuffer and send it to the output device
        internalPresent();
        _presentRate.increment();
        _activeProgram.reset();
    }
}

float OpenGLDisplayPlugin::newFramePresentRate() const {
    return _newFrameRate.rate();
}

float OpenGLDisplayPlugin::droppedFrameRate() const {
    float result;
    withRenderThreadLock([&] {
        result = _droppedFrameRate.rate();
    });
    return result;
}

float OpenGLDisplayPlugin::presentRate() const {
    return _presentRate.rate();
}

void OpenGLDisplayPlugin::drawUnitQuad() {
    useProgram(_program);
    _plane->Use();
    _plane->Draw();
}

void OpenGLDisplayPlugin::enableVsync(bool enable) {
    if (!_vsyncSupported) {
        return;
    }
#if defined(Q_OS_WIN)
    wglSwapIntervalEXT(enable ? 1 : 0);
#elif defined(Q_OS_MAC)
    GLint interval = enable ? 1 : 0;
    CGLSetParameter(CGLGetCurrentContext(), kCGLCPSwapInterval, &interval);
#else
    // TODO: Fill in for linux
    return;
#endif
}

bool OpenGLDisplayPlugin::isVsyncEnabled() {
    if (!_vsyncSupported) {
        return true;
    }
#if defined(Q_OS_WIN)
    return wglGetSwapIntervalEXT() != 0;
#elif defined(Q_OS_MAC)
    GLint interval;
    CGLGetParameter(CGLGetCurrentContext(), kCGLCPSwapInterval, &interval);
    return interval != 0;
#else
    // TODO: Fill in for linux
    return true;
#endif
}

void OpenGLDisplayPlugin::swapBuffers() {
    static auto widget = _container->getPrimaryWidget();
    widget->swapBuffers();
}

void OpenGLDisplayPlugin::withMainThreadContext(std::function<void()> f) const {
#if THREADED_PRESENT
    static auto presentThread = DependencyManager::get<PresentThread>();
    presentThread->withMainThreadContext(f);
    _container->makeRenderingContextCurrent();
#else
    static auto widget = _container->getPrimaryWidget();
    widget->makeCurrent();
    f();
    _container->makeRenderingContextCurrent();
#endif
}

QImage OpenGLDisplayPlugin::getScreenshot() const {
    using namespace oglplus;
    auto width = _compositeFramebuffer->size.x;
    if (isHmd()) {
        width /= 2;
    }

    QImage screenshot(width, _compositeFramebuffer->size.y, QImage::Format_RGBA8888);
    withMainThreadContext([&] {
        Framebuffer::Bind(Framebuffer::Target::Read, _compositeFramebuffer->fbo);
        Context::ReadPixels(0, 0, width, _compositeFramebuffer->size.y, enums::PixelDataFormat::RGBA, enums::PixelDataType::UnsignedByte, screenshot.bits());
    });
    return screenshot.mirrored(false, true);
}

uint32_t OpenGLDisplayPlugin::getSceneTextureId() const {
    if (!_currentSceneTexture) {
        return 0;
    }
    
    return _currentSceneTexture->getHardwareId(); 
}

uint32_t OpenGLDisplayPlugin::getOverlayTextureId() const {
    if (!_currentOverlayTexture) {
        return 0;
    }
    return _currentOverlayTexture->getHardwareId();
}

void OpenGLDisplayPlugin::eyeViewport(Eye eye) const {
    using namespace oglplus;
    uvec2 vpSize = _compositeFramebuffer->size;
    vpSize.x /= 2;
    uvec2 vpPos;
    if (eye == Eye::Right) {
        vpPos.x = vpSize.x;
    }
    Context::Viewport(vpPos.x, vpPos.y, vpSize.x, vpSize.y);
}

glm::uvec2 OpenGLDisplayPlugin::getSurfacePixels() const {
    uvec2 result;
    auto window = _container->getPrimaryWidget();
    if (window) {
        result = toGlm(window->geometry().size() * window->devicePixelRatio());
    }
    return result;
}

glm::uvec2 OpenGLDisplayPlugin::getSurfaceSize() const {
    uvec2 result;
    auto window = _container->getPrimaryWidget();
    if (window) {
        result = toGlm(window->geometry().size());
    }
    return result;
}

bool OpenGLDisplayPlugin::hasFocus() const {
    auto window = _container->getPrimaryWidget();
    return window ? window->hasFocus() : false;
}

void OpenGLDisplayPlugin::useProgram(const ProgramPtr& program) {
    if (_activeProgram != program) {
        program->Bind();
        _activeProgram = program;
    }
}

void OpenGLDisplayPlugin::assertIsRenderThread() const {
    Q_ASSERT(QThread::currentThread() != _presentThread);
}

void OpenGLDisplayPlugin::assertIsPresentThread() const {
    Q_ASSERT(QThread::currentThread() == _presentThread);
}

bool OpenGLDisplayPlugin::beginFrameRender(uint32_t frameIndex) {
    withRenderThreadLock([&] {
        _compositeOverlayAlpha = _overlayAlpha;
    });
    return Parent::beginFrameRender(frameIndex);
}
