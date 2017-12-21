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
#include <QOpenGLFramebufferObject>
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
#include <gl/Context.h>

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
#include <TextureCache.h>
#include "CompositorHelper.h"
#include "Logging.h"

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
            qCDebug(displayPlugins) << "Present thread shutdown";
        }
    }


    void setNewDisplayPlugin(OpenGLDisplayPlugin* plugin) {
        Lock lock(_mutex);
        if (isRunning()) {
            _newPluginQueue.push(plugin);
            _condition.wait(lock, [=]()->bool { return _newPluginQueue.empty(); });
        }
    }

    void setContext(gl::Context* context) {
        // Move the OpenGL context to the present thread
        // Extra code because of the widget 'wrapper' context
        _context = context;
        _context->moveToThread(this);
    }


    virtual void run() override {
        PROFILE_SET_THREAD_NAME("Present Thread");

        // FIXME determine the best priority balance between this and the main thread...
        // It may be dependent on the display plugin being used, since VR plugins should
        // have higher priority on rendering (although we could say that the Oculus plugin
        // doesn't need that since it has async timewarp).
        // A higher priority here
        setPriority(QThread::HighPriority);
        OpenGLDisplayPlugin* currentPlugin{ nullptr };
        Q_ASSERT(_context);
        _context->makeCurrent();
        while (!_shutdown) {
            if (_pendingMainThreadOperation) {
                PROFILE_RANGE(render, "MainThreadOp")
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
                            QThread::setPriority(newPlugin->getPresentPriority());
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
                PROFILE_RANGE(render, "PluginPresent")
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
    gl::Context* _context { nullptr };
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
        auto animation = new QPropertyAnimation(this, "hudAlpha");
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

bool OpenGLDisplayPlugin::startStandBySession() {
    if (!activateStandBySession()) {
        return false;
    }
    return Parent::startStandBySession();
}

void OpenGLDisplayPlugin::endSession() {
    deactivateSession();
    Parent::endSession();
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

                cursorData.texture = gpu::Texture::createStrict(
                    gpu::Element(gpu::VEC4, gpu::NUINT8, gpu::RGBA),
                    image.width(), image.height(),
                    gpu::Texture::MAX_NUM_MIPS,
                    gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_MIP_LINEAR));
                cursorData.texture->setSource("cursor texture");
                auto usage = gpu::Texture::Usage::Builder().withColor().withAlpha();
                cursorData.texture->setUsage(usage.build());
                cursorData.texture->setStoredMipFormat(gpu::Element(gpu::VEC4, gpu::NUINT8, gpu::RGBA));
                cursorData.texture->assignStoredMip(0, image.byteCount(), image.constBits());
                cursorData.texture->setAutoGenerateMips(true);
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
            _hudPipeline = gpu::Pipeline::create(program, state);
        }

        {
            auto vs = gpu::StandardShaderLib::getDrawUnitQuadTexcoordVS();
            auto ps = gpu::StandardShaderLib::getDrawTextureMirroredXPS();
            gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);
            gpu::Shader::makeProgram(*program);
            gpu::StatePointer state = gpu::StatePointer(new gpu::State());
            state->setDepthTest(gpu::State::DepthTest(false));
            state->setBlendFunction(true,
                gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA,
                gpu::State::FACTOR_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::ONE);
            _mirrorHUDPipeline = gpu::Pipeline::create(program, state);
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
    updateCompositeFramebuffer();
}

void OpenGLDisplayPlugin::uncustomizeContext() {
    _presentPipeline.reset();
    _cursorPipeline.reset();
    _hudPipeline.reset();
    _mirrorHUDPipeline.reset();
    _compositeFramebuffer.reset();
    withPresentThreadLock([&] {
        _currentFrame.reset();
        _lastFrame = nullptr;
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
    withNonPresentThreadLock([&] {
        _newFrameQueue.push(newFrame);
    });
}

void OpenGLDisplayPlugin::renderFromTexture(gpu::Batch& batch, const gpu::TexturePointer texture, glm::ivec4 viewport, const glm::ivec4 scissor) {
    renderFromTexture(batch, texture, viewport, scissor, gpu::FramebufferPointer());
}

void OpenGLDisplayPlugin::renderFromTexture(gpu::Batch& batch, const gpu::TexturePointer texture, glm::ivec4 viewport, const glm::ivec4 scissor, gpu::FramebufferPointer copyFbo /*=gpu::FramebufferPointer()*/) {
    auto fbo = gpu::FramebufferPointer();
    batch.enableStereo(false);
    batch.resetViewTransform();
    batch.setFramebuffer(fbo);
    batch.clearColorFramebuffer(gpu::Framebuffer::BUFFER_COLOR0, vec4(0));
    batch.setStateScissorRect(scissor);
    batch.setViewportTransform(viewport);
    batch.setResourceTexture(0, texture);
    batch.setPipeline(_presentPipeline);
    batch.draw(gpu::TRIANGLE_STRIP, 4);
    if (copyFbo) {
        gpu::Vec4i copyFboRect(0, 0, copyFbo->getWidth(), copyFbo->getHeight());
        gpu::Vec4i sourceRect(scissor.x, scissor.y, scissor.x + scissor.z, scissor.y + scissor.w);
        float aspectRatio = (float)scissor.w / (float) scissor.z; // height/width
        // scale width first
        int xOffset = 0;
        int yOffset = 0;
        int newWidth = copyFbo->getWidth();
        int newHeight = std::round(aspectRatio * (float) copyFbo->getWidth());
        if (newHeight > copyFbo->getHeight()) {
            // ok, so now fill height instead
            newHeight = copyFbo->getHeight();
            newWidth = std::round((float)copyFbo->getHeight() / aspectRatio);
            xOffset = (copyFbo->getWidth() - newWidth) / 2;
        } else {
            yOffset = (copyFbo->getHeight() - newHeight) / 2;
        }
        gpu::Vec4i copyRect(xOffset, yOffset, xOffset + newWidth, yOffset + newHeight);
        batch.setFramebuffer(copyFbo);

        batch.resetViewTransform();
        batch.setViewportTransform(copyFboRect);
        batch.setStateScissorRect(copyFboRect);
        batch.clearColorFramebuffer(gpu::Framebuffer::BUFFER_COLOR0, {0.0f, 0.0f, 0.0f, 1.0f});
        batch.blit(fbo, sourceRect, copyFbo, copyRect);
    }
}

void OpenGLDisplayPlugin::updateFrameData() {
    PROFILE_RANGE(render, __FUNCTION__)
    if (_lockCurrentTexture) {
        return;
    }
    withPresentThreadLock([&] {
        if (!_newFrameQueue.empty()) {
            // We're changing frames, so we can cleanup any GL resources that might have been used by the old frame
            _gpuContext->recycle();
        }
        if (_newFrameQueue.size() > 1) {
            _droppedFrameRate.increment(_newFrameQueue.size() - 1);
        }
        while (!_newFrameQueue.empty()) {
            _currentFrame = _newFrameQueue.front();
            _newFrameQueue.pop();
            _gpuContext->consumeFrameUpdates(_currentFrame);
        }
    });
}

std::function<void(gpu::Batch&, const gpu::TexturePointer&, bool mirror)> OpenGLDisplayPlugin::getHUDOperator() {
    return [this](gpu::Batch& batch, const gpu::TexturePointer& hudTexture, bool mirror) {
        if (_hudPipeline) {
            batch.enableStereo(false);
            batch.setPipeline(mirror ? _mirrorHUDPipeline : _hudPipeline);
            batch.setResourceTexture(0, hudTexture);
            if (isStereo()) {
                for_each_eye([&](Eye eye) {
                    batch.setViewportTransform(eyeViewport(eye));
                    batch.draw(gpu::TRIANGLE_STRIP, 4);
                });
            } else {
                batch.setViewportTransform(ivec4(uvec2(0), _compositeFramebuffer->getSize()));
                batch.draw(gpu::TRIANGLE_STRIP, 4);
            }
        }
    };
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
            batch.setViewportTransform(ivec4(uvec2(0), _compositeFramebuffer->getSize()));
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
    updateCompositeFramebuffer();

    {
        PROFILE_RANGE_EX(render_detail, "compositeScene", 0xff0077ff, (uint64_t)presentCount())
        compositeScene();
    }

#ifdef HIFI_ENABLE_NSIGHT_DEBUG
    if (false) // do not draw the HUD if running nsight debug
#endif
    {
        PROFILE_RANGE_EX(render_detail, "handleHUDBatch", 0xff0077ff, (uint64_t)presentCount())
        auto hudOperator = getHUDOperator();
        withPresentThreadLock([&] {
            _hudOperator = hudOperator;
        });
    }

    {
        PROFILE_RANGE_EX(render_detail, "compositeExtra", 0xff0077ff, (uint64_t)presentCount())
        compositeExtra();
    }

    // Draw the pointer last so it's on top of everything
    auto compositorHelper = DependencyManager::get<CompositorHelper>();
    if (compositorHelper->getReticleVisible()) {
        PROFILE_RANGE_EX(render_detail, "compositePointer", 0xff0077ff, (uint64_t)presentCount())
            compositePointer();
    }
}

void OpenGLDisplayPlugin::internalPresent() {
    render([&](gpu::Batch& batch) {
        // Note: _displayTexture must currently be the same size as the display.
        uvec2 dims = _displayTexture ? uvec2(_displayTexture->getDimensions()) : getSurfacePixels();
        auto viewport = ivec4(uvec2(0),  dims);
        renderFromTexture(batch, _displayTexture ? _displayTexture : _compositeFramebuffer->getRenderBuffer(0), viewport, viewport);
     });
    swapBuffers();
    _presentRate.increment();
}

void OpenGLDisplayPlugin::present() {
    auto frameId = (uint64_t)presentCount();
    PROFILE_RANGE_EX(render, __FUNCTION__, 0xffffff00, frameId)
    uint64_t startPresent = usecTimestampNow();
    {
        PROFILE_RANGE_EX(render, "updateFrameData", 0xff00ff00, frameId)
        updateFrameData();
    }
    incrementPresentCount();

    if (_currentFrame) {
        {
            withPresentThreadLock([&] {
                _renderRate.increment();
                if (_currentFrame.get() != _lastFrame) {
                    _newFrameRate.increment();
                }
                _lastFrame = _currentFrame.get();
            });
            // Execute the frame rendering commands
            PROFILE_RANGE_EX(render, "execute", 0xff00ff00, frameId)
            _gpuContext->executeFrame(_currentFrame);
        }

        // Write all layers to a local framebuffer
        {
            PROFILE_RANGE_EX(render, "composite", 0xff00ffff, frameId)
            compositeLayers();
        }

        // Take the composite framebuffer and send it to the output device
        {
            PROFILE_RANGE_EX(render, "internalPresent", 0xff00ffff, frameId)
            internalPresent();
        }

        gpu::Backend::freeGPUMemSize.set(gpu::gl::getFreeDedicatedMemory());
    }
    _movingAveragePresent.addSample((float)(usecTimestampNow() - startPresent));
}

float OpenGLDisplayPlugin::newFramePresentRate() const {
    return _newFrameRate.rate();
}

float OpenGLDisplayPlugin::droppedFrameRate() const {
    return _droppedFrameRate.rate();
}

float OpenGLDisplayPlugin::presentRate() const {
    return _presentRate.rate();
}

void OpenGLDisplayPlugin::resetPresentRate() {
    // FIXME
    // _presentRate = RateCounter<100>();
}

float OpenGLDisplayPlugin::renderRate() const {
    return _renderRate.rate();
}


void OpenGLDisplayPlugin::swapBuffers() {
    static auto context = _container->getPrimaryWidget()->context();
    context->swapBuffers();
}

void OpenGLDisplayPlugin::withMainThreadContext(std::function<void()> f) const {
    static auto presentThread = DependencyManager::get<PresentThread>();
    presentThread->withMainThreadContext(f);
    _container->makeRenderingContextCurrent();
}

bool OpenGLDisplayPlugin::setDisplayTexture(const QString& name) {
    // Note: it is the caller's responsibility to keep the network texture in cache.
    if (name.isEmpty()) {
        _displayTexture.reset();
        onDisplayTextureReset();
        return true;
    }
    auto textureCache = DependencyManager::get<TextureCache>();
    auto displayNetworkTexture = textureCache->getTexture(name);
    if (!displayNetworkTexture) {
        return false;
    }
    _displayTexture = displayNetworkTexture->getGPUTexture();
    return !!_displayTexture;
}

QImage OpenGLDisplayPlugin::getScreenshot(float aspectRatio) const {
    auto size = _compositeFramebuffer->getSize();
    if (isHmd()) {
        size.x /= 2;
    }
    auto bestSize = size;
    uvec2 corner(0);
    if (aspectRatio != 0.0f) { // Pick out the largest piece of the center that produces the requested width/height aspectRatio
        if (ceil(size.y * aspectRatio) < size.x) {
            bestSize.x = round(size.y * aspectRatio);
        } else {
            bestSize.y = round(size.x / aspectRatio);
        }
        corner.x = round((size.x - bestSize.x) / 2.0f);
        corner.y = round((size.y - bestSize.y) / 2.0f);
    }
    auto glBackend = const_cast<OpenGLDisplayPlugin&>(*this).getGLBackend();
    QImage screenshot(bestSize.x, bestSize.y, QImage::Format_ARGB32);
    withMainThreadContext([&] {
        glBackend->downloadFramebuffer(_compositeFramebuffer, ivec4(corner, bestSize), screenshot);
    });
    return screenshot.mirrored(false, true);
}

QImage OpenGLDisplayPlugin::getSecondaryCameraScreenshot() const {
    auto textureCache = DependencyManager::get<TextureCache>();
    auto secondaryCameraFramebuffer = textureCache->getSpectatorCameraFramebuffer();
    gpu::Vec4i region(0, 0, secondaryCameraFramebuffer->getWidth(), secondaryCameraFramebuffer->getHeight());

    auto glBackend = const_cast<OpenGLDisplayPlugin&>(*this).getGLBackend();
    QImage screenshot(region.z, region.w, QImage::Format_ARGB32);
    withMainThreadContext([&] {
        glBackend->downloadFramebuffer(secondaryCameraFramebuffer, region, screenshot);
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
        _compositeHUDAlpha = _hudAlpha;
    });
    return Parent::beginFrameRender(frameIndex);
}

ivec4 OpenGLDisplayPlugin::eyeViewport(Eye eye) const {
    uvec2 vpSize = _compositeFramebuffer->getSize();
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


OpenGLDisplayPlugin::~OpenGLDisplayPlugin() {
}

void OpenGLDisplayPlugin::updateCompositeFramebuffer() {
    auto renderSize = getRecommendedRenderSize();
    if (!_compositeFramebuffer || _compositeFramebuffer->getSize() != renderSize) {
        _compositeFramebuffer = gpu::FramebufferPointer(gpu::Framebuffer::create("OpenGLDisplayPlugin::composite", gpu::Element::COLOR_RGBA_32, renderSize.x, renderSize.y));
    }
}

void OpenGLDisplayPlugin::copyTextureToQuickFramebuffer(NetworkTexturePointer networkTexture, QOpenGLFramebufferObject* target, GLsync* fenceSync) {
    auto glBackend = const_cast<OpenGLDisplayPlugin&>(*this).getGLBackend();
    withMainThreadContext([&] {
        GLuint sourceTexture = glBackend->getTextureID(networkTexture->getGPUTexture());
        GLuint targetTexture = target->texture();
        GLuint fbo[2] {0, 0};

        // need mipmaps for blitting texture
        glGenerateTextureMipmap(sourceTexture);

        // create 2 fbos (one for initial texture, second for scaled one)
        glCreateFramebuffers(2, fbo);

        // setup source fbo
        glBindFramebuffer(GL_FRAMEBUFFER, fbo[0]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sourceTexture, 0);

        GLint texWidth = networkTexture->getWidth();
        GLint texHeight = networkTexture->getHeight();

        // setup destination fbo
        glBindFramebuffer(GL_FRAMEBUFFER, fbo[1]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, targetTexture, 0);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);


        // maintain aspect ratio, filling the width first if possible.  If that makes the height too
        // much, fill height instead. TODO: only do this when texture changes
        GLint newX = 0;
        GLint newY = 0;
        float aspectRatio = (float)texHeight / (float)texWidth;
        GLint newWidth = target->width();
        GLint newHeight = std::round(aspectRatio * (float) target->width());
        if (newHeight > target->height()) {
            newHeight = target->height();
            newWidth = std::round((float)target->height() / aspectRatio);
            newX = (target->width() - newWidth) / 2;
        } else {
            newY = (target->height() - newHeight) / 2;
        }
        glBlitNamedFramebuffer(fbo[0], fbo[1], 0, 0, texWidth, texHeight, newX, newY, newX + newWidth, newY + newHeight, GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT, GL_NEAREST);

        // don't delete the textures!
        glDeleteFramebuffers(2, fbo);
        *fenceSync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    });
}

