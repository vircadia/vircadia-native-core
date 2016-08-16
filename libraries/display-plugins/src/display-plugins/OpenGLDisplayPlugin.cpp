//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "OpenGLDisplayPlugin.h"

#include <condition_variable>
#include <queue>

#include <QtCore/QCoreApplication>
#include <QtCore/QThread>
#include <QtCore/QTimer>

#include <QtOpenGL/QGLWidget>
#include <QtGui/QImage>

#if defined(Q_OS_MAC)
#include <OpenGL/CGLCurrent.h>
#endif

#include <NumericalConstants.h>
#include <DependencyManager.h>
#include <GLMHelpers.h>

#include <gl/QOpenGLContextWrapper.h>
#include <gl/GLWidget.h>
#include <gl/Config.h>
#include <gl/GLEscrow.h>

#include <gpu/Texture.h>
#include <gpu/StandardShaderLib.h>
#include <gpu/gl/GLShared.h>
#include <gpu/gl/GLBackend.h>
#include <GeometryCache.h>

#include <FramebufferCache.h>
#include <shared/NsightHelpers.h>
#include <ui-plugins/PluginContainer.h>
#include <ui/Menu.h>
#include <CursorManager.h>

#include "CompositorHelper.h"

const char* SRGB_TO_LINEAR_FRAG = R"SCRIBE(

uniform sampler2D colorMap;

in vec2 varTexCoord0;

out vec4 outFragColor;

float sRGBFloatToLinear(float value) {
    const float SRGB_ELBOW = 0.04045;
    
    return (value <= SRGB_ELBOW) ? value / 12.92 : pow((value + 0.055) / 1.055, 2.4);
}

vec3 colorToLinearRGB(vec3 srgb) {
    return vec3(sRGBFloatToLinear(srgb.r), sRGBFloatToLinear(srgb.g), sRGBFloatToLinear(srgb.b));
}

void main(void) {
    outFragColor.a = 1.0;
    outFragColor.rgb = colorToLinearRGB(texture(colorMap, varTexCoord0).rgb);
}

)SCRIBE";

extern QThread* RENDER_THREAD;

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
        // FIXME determine the best priority balance between this and the main thread...
        // It may be dependent on the display plugin being used, since VR plugins should 
        // have higher priority on rendering (although we could say that the Oculus plugin
        // doesn't need that since it has async timewarp).
        // A higher priority here 
        setPriority(QThread::HighPriority);
        OpenGLDisplayPlugin* currentPlugin{ nullptr };
        Q_ASSERT(_context);
        _context->makeCurrent();
        Q_ASSERT(isCurrentContext(_context->contextHandle()));
        while (!_shutdown) {
            if (_pendingMainThreadOperation) {
                PROFILE_RANGE("MainThreadOp") 
                {
                    Lock lock(_mutex);
                    _context->doneCurrent();
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
                // Check if we have a new plugin to activate
                while (!_newPluginQueue.empty()) {
                    auto newPlugin = _newPluginQueue.front();
                    if (newPlugin != currentPlugin) {
                        // Deactivate the old plugin
                        if (currentPlugin != nullptr) {
                            _context->makeCurrent();
                            currentPlugin->uncustomizeContext();
                            CHECK_GL_ERROR();
                            _context->doneCurrent();
                        }

                        if (newPlugin) {
                            bool hasVsync = true;
                            bool wantVsync = newPlugin->wantVsync();
                            _context->makeCurrent();
#if defined(Q_OS_WIN)
                            wglSwapIntervalEXT(wantVsync ? 1 : 0);
                            hasVsync = wglGetSwapIntervalEXT() != 0;
#elif defined(Q_OS_MAC)
                            GLint interval = wantVsync ? 1 : 0;
                            newPlugin->swapBuffers();
                            CGLSetParameter(CGLGetCurrentContext(), kCGLCPSwapInterval, &interval);
                            newPlugin->swapBuffers();
                            CGLGetParameter(CGLGetCurrentContext(), kCGLCPSwapInterval, &interval);
                            hasVsync = interval != 0;
#else
                            // TODO: Fill in for linux
                            Q_UNUSED(wantVsync);
#endif
                            newPlugin->setVsyncEnabled(hasVsync);
                            newPlugin->customizeContext();
                            CHECK_GL_ERROR();
                            _context->doneCurrent();
                        }
                        currentPlugin = newPlugin;
                        _newPluginQueue.pop();
                        _condition.notify_one();
                    }
                }
            }

            // If there's no active plugin, just sleep
            if (currentPlugin == nullptr) {
                // Minimum sleep ends up being about 2 ms anyway
                QThread::msleep(1);
                continue;
            }

            // Execute the frame and present it to the display device.
            _context->makeCurrent();
            {
                PROFILE_RANGE("PluginPresent")
                currentPlugin->present();
                CHECK_GL_ERROR();
            }
            _context->doneCurrent();
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
    if (!RENDER_THREAD) {
        RENDER_THREAD = _presentThread;
    }
    
    // Child classes may override this in order to do things like initialize
    // libraries, etc
    if (!internalActivate()) {
        return false;
    }


    // This should not return until the new context has been customized
    // and the old context (if any) has been uncustomized
    presentThread->setNewDisplayPlugin(this);

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

    auto presentThread = DependencyManager::get<PresentThread>();
    // Does not return until the GL transition has completeed
    presentThread->setNewDisplayPlugin(nullptr);
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
    auto presentThread = DependencyManager::get<PresentThread>();
    Q_ASSERT(thread() == presentThread->thread());

    getGLBackend()->setCameraCorrection(mat4());

    for (auto& cursorValue : _cursorsData) {
        auto& cursorData = cursorValue.second;
        if (!cursorData.texture) {
            auto image = cursorData.image;
            if (image.format() != QImage::Format_ARGB32) {
                image = image.convertToFormat(QImage::Format_ARGB32);
            }
            if ((image.width() > 0) && (image.height() > 0)) {

                cursorData.texture.reset(
                    gpu::Texture::create2D(
                        gpu::Element(gpu::VEC4, gpu::NUINT8, gpu::RGBA), 
                        image.width(), image.height(), 
                        gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_MIP_LINEAR)));
                auto usage = gpu::Texture::Usage::Builder().withColor().withAlpha();
                cursorData.texture->setUsage(usage.build());
                cursorData.texture->assignStoredMip(0, gpu::Element(gpu::VEC4, gpu::NUINT8, gpu::RGBA), image.byteCount(), image.constBits());
                cursorData.texture->autoGenerateMips(-1);
            }
        }
    }

    if (!_presentPipeline) {
        {
            auto vs = gpu::StandardShaderLib::getDrawUnitQuadTexcoordVS();
            auto ps = gpu::StandardShaderLib::getDrawTexturePS();
            gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);
            gpu::Shader::makeProgram(*program);
            gpu::StatePointer state = gpu::StatePointer(new gpu::State());
            state->setDepthTest(gpu::State::DepthTest(false));
            state->setScissorEnable(true);
            _simplePipeline = gpu::Pipeline::create(program, state);
        }

        {
            auto vs = gpu::StandardShaderLib::getDrawUnitQuadTexcoordVS();
            auto ps = gpu::Shader::createPixel(std::string(SRGB_TO_LINEAR_FRAG));
            gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);
            gpu::Shader::makeProgram(*program);
            gpu::StatePointer state = gpu::StatePointer(new gpu::State());
            state->setDepthTest(gpu::State::DepthTest(false));
            state->setScissorEnable(true);
            _presentPipeline = gpu::Pipeline::create(program, state);
        }

        {
            auto vs = gpu::StandardShaderLib::getDrawUnitQuadTexcoordVS();
            auto ps = gpu::StandardShaderLib::getDrawTexturePS();
            gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);
            gpu::Shader::makeProgram(*program);
            gpu::StatePointer state = gpu::StatePointer(new gpu::State());
            state->setDepthTest(gpu::State::DepthTest(false));
            state->setBlendFunction(true, 
                gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA,
                gpu::State::FACTOR_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::ONE);
            _overlayPipeline = gpu::Pipeline::create(program, state);
        }

        {
            auto vs = gpu::StandardShaderLib::getDrawTransformUnitQuadVS();
            auto ps = gpu::StandardShaderLib::getDrawTexturePS();
            gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);
            gpu::Shader::makeProgram(*program);
            gpu::StatePointer state = gpu::StatePointer(new gpu::State());
            state->setDepthTest(gpu::State::DepthTest(false));
            state->setBlendFunction(true,
                gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA,
                gpu::State::FACTOR_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::ONE);
            _cursorPipeline = gpu::Pipeline::create(program, state);
        }
    }
    auto renderSize = getRecommendedRenderSize();
    _compositeFramebuffer = gpu::FramebufferPointer(gpu::Framebuffer::create(gpu::Element::COLOR_RGBA_32, renderSize.x, renderSize.y));
}

void OpenGLDisplayPlugin::uncustomizeContext() {
    _presentPipeline.reset();
    _cursorPipeline.reset();
    _overlayPipeline.reset();
    _compositeFramebuffer.reset();
    withPresentThreadLock([&] {
        _currentFrame.reset();
        while (!_newFrameQueue.empty()) {
            _gpuContext->consumeFrameUpdates(_newFrameQueue.front());
            _newFrameQueue.pop();
        }
    });
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

void OpenGLDisplayPlugin::submitFrame(const gpu::FramePointer& newFrame) {
    if (_lockCurrentTexture) {
        return;
    }

    withNonPresentThreadLock([&] {
        _newFrameQueue.push(newFrame);
    });
}

void OpenGLDisplayPlugin::updateFrameData() {
    withPresentThreadLock([&] {
        gpu::FramePointer oldFrame = _currentFrame;
        uint32_t skippedCount = 0;
        if (!_newFrameQueue.empty()) {
            // We're changing frames, so we can cleanup any GL resources that might have been used by the old frame
            _gpuContext->recycle();
        }
        while (!_newFrameQueue.empty()) {
            _currentFrame = _newFrameQueue.front();
            _newFrameQueue.pop();
            _gpuContext->consumeFrameUpdates(_currentFrame);
            if (_currentFrame && oldFrame) {
                skippedCount += (_currentFrame->frameIndex - oldFrame->frameIndex) - 1;
            }
        }
        _droppedFrameRate.increment(skippedCount);
    });
}

void OpenGLDisplayPlugin::compositeOverlay() {
    render([&](gpu::Batch& batch){
        batch.enableStereo(false);
        batch.setFramebuffer(_compositeFramebuffer);
        batch.setPipeline(_overlayPipeline);
        batch.setResourceTexture(0, _currentFrame->overlay);
        if (isStereo()) {
            for_each_eye([&](Eye eye) {
                batch.setViewportTransform(eyeViewport(eye));
                batch.draw(gpu::TRIANGLE_STRIP, 4);
            });
        } else {
            batch.setViewportTransform(ivec4(uvec2(0), _currentFrame->framebuffer->getSize()));
            batch.draw(gpu::TRIANGLE_STRIP, 4);
        }
    });
}

void OpenGLDisplayPlugin::compositePointer() {
    auto& cursorManager = Cursor::Manager::instance();
    const auto& cursorData = _cursorsData[cursorManager.getCursor()->getIcon()];
    auto cursorTransform = DependencyManager::get<CompositorHelper>()->getReticleTransform(glm::mat4());
    render([&](gpu::Batch& batch) {
        batch.enableStereo(false);
        batch.setProjectionTransform(mat4());
        batch.setFramebuffer(_compositeFramebuffer);
        batch.setPipeline(_cursorPipeline);
        batch.setResourceTexture(0, cursorData.texture);
        batch.resetViewTransform();
        batch.setModelTransform(cursorTransform);
        if (isStereo()) {
            for_each_eye([&](Eye eye) {
                batch.setViewportTransform(eyeViewport(eye));
                batch.draw(gpu::TRIANGLE_STRIP, 4);
            });
        } else {
            batch.setViewportTransform(ivec4(uvec2(0), _currentFrame->framebuffer->getSize()));
            batch.draw(gpu::TRIANGLE_STRIP, 4);
        }
    });
}

void OpenGLDisplayPlugin::compositeScene() {
    render([&](gpu::Batch& batch) {
        batch.enableStereo(false);
        batch.setFramebuffer(_compositeFramebuffer);
        batch.setViewportTransform(ivec4(uvec2(), _compositeFramebuffer->getSize()));
        batch.setStateScissorRect(ivec4(uvec2(), _compositeFramebuffer->getSize()));
        batch.resetViewTransform();
        batch.setProjectionTransform(mat4());
        batch.setPipeline(_simplePipeline);
        batch.setResourceTexture(0, _currentFrame->framebuffer->getRenderBuffer(0));
        batch.draw(gpu::TRIANGLE_STRIP, 4);
    });
}

void OpenGLDisplayPlugin::compositeLayers() {
    auto renderSize = getRecommendedRenderSize();
    if (!_compositeFramebuffer || _compositeFramebuffer->getSize() != renderSize) {
        _compositeFramebuffer = gpu::FramebufferPointer(gpu::Framebuffer::create(gpu::Element::COLOR_RGBA_32, renderSize.x, renderSize.y));
    }

    {
        PROFILE_RANGE_EX("compositeScene", 0xff0077ff, (uint64_t)presentCount())
        compositeScene();
    }

    {
        PROFILE_RANGE_EX("compositeOverlay", 0xff0077ff, (uint64_t)presentCount())
        compositeOverlay();
    }
    auto compositorHelper = DependencyManager::get<CompositorHelper>();
    if (compositorHelper->getReticleVisible()) {
        PROFILE_RANGE_EX("compositePointer", 0xff0077ff, (uint64_t)presentCount())
        compositePointer();
    }

    {
        PROFILE_RANGE_EX("compositeExtra", 0xff0077ff, (uint64_t)presentCount())
        compositeExtra();
    }
}

void OpenGLDisplayPlugin::internalPresent() {
    render([&](gpu::Batch& batch) {
        batch.enableStereo(false);
        batch.resetViewTransform();
        batch.setFramebuffer(gpu::FramebufferPointer());
        batch.setViewportTransform(ivec4(uvec2(0), getSurfacePixels()));
        batch.setResourceTexture(0, _compositeFramebuffer->getRenderBuffer(0));
        batch.setPipeline(_presentPipeline);
        batch.draw(gpu::TRIANGLE_STRIP, 4);
    });
    swapBuffers();
}

void OpenGLDisplayPlugin::present() {
    PROFILE_RANGE_EX(__FUNCTION__, 0xffffff00, (uint64_t)presentCount())
    updateFrameData();
    incrementPresentCount();

    {
        PROFILE_RANGE_EX("recycle", 0xff00ff00, (uint64_t)presentCount())
        _gpuContext->recycle();
    }

    if (_currentFrame) {
        {
            // Execute the frame rendering commands
            PROFILE_RANGE_EX("execute", 0xff00ff00, (uint64_t)presentCount())
            _gpuContext->executeFrame(_currentFrame);
        }

        // Write all layers to a local framebuffer
        {
            PROFILE_RANGE_EX("composite", 0xff00ffff, (uint64_t)presentCount())
            compositeLayers();
        }

        // Take the composite framebuffer and send it to the output device
        {
            PROFILE_RANGE_EX("internalPresent", 0xff00ffff, (uint64_t)presentCount())
            internalPresent();
        }
        _presentRate.increment();
    }
}

float OpenGLDisplayPlugin::newFramePresentRate() const {
    return _newFrameRate.rate();
}

float OpenGLDisplayPlugin::droppedFrameRate() const {
    float result;
    withNonPresentThreadLock([&] {
        result = _droppedFrameRate.rate();
    });
    return result;
}

float OpenGLDisplayPlugin::presentRate() const {
    return _presentRate.rate();
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
    auto size = _compositeFramebuffer->getSize();
    auto glBackend = const_cast<OpenGLDisplayPlugin&>(*this).getGLBackend();
    QImage screenshot(size.x, size.y, QImage::Format_ARGB32);
    withMainThreadContext([&] {
        glBackend->downloadFramebuffer(_compositeFramebuffer, ivec4(uvec2(0), size), screenshot);
    });
    return screenshot.mirrored(false, true);
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

void OpenGLDisplayPlugin::assertNotPresentThread() const {
    Q_ASSERT(QThread::currentThread() != _presentThread);
}

void OpenGLDisplayPlugin::assertIsPresentThread() const {
    Q_ASSERT(QThread::currentThread() == _presentThread);
}

bool OpenGLDisplayPlugin::beginFrameRender(uint32_t frameIndex) {
    withNonPresentThreadLock([&] {
        _compositeOverlayAlpha = _overlayAlpha;
    });
    return Parent::beginFrameRender(frameIndex);
}

ivec4 OpenGLDisplayPlugin::eyeViewport(Eye eye) const {
    uvec2 vpSize = _currentFrame->framebuffer->getSize();
    vpSize.x /= 2;
    uvec2 vpPos;
    if (eye == Eye::Right) {
        vpPos.x = vpSize.x;
    }
    return ivec4(vpPos, vpSize);
}

gpu::gl::GLBackend* OpenGLDisplayPlugin::getGLBackend() {
    if (!_gpuContext || !_gpuContext->getBackend()) {
        return nullptr;
    }
    auto backend = _gpuContext->getBackend().get();
#if defined(Q_OS_MAC)
    // Should be dynamic_cast, but that doesn't work in plugins on OSX
    auto glbackend = static_cast<gpu::gl::GLBackend*>(backend);
#else
    auto glbackend = dynamic_cast<gpu::gl::GLBackend*>(backend);
#endif

    return glbackend;
}

void OpenGLDisplayPlugin::render(std::function<void(gpu::Batch& batch)> f) {
    gpu::Batch batch;
    f(batch);
    _gpuContext->executeBatch(batch);
}
