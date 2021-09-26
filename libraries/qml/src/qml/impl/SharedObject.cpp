//
//  Created by Bradley Austin Davis on 2018-01-04
//  Copyright 2013-2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "SharedObject.h"

#include <QtCore/qlogging.h>
#include <QtQuick/QQuickWindow>
#include <QtQuick/QQuickItem>
#include <QtQml/QQmlContext>
#include <QtQml/QQmlEngine>

#include <QtGui/QOpenGLContext>
#include <QPointer>

#include <NumericalConstants.h>
#include <shared/NsightHelpers.h>
#include <gl/QOpenGLContextWrapper.h>
#include <gl/GLHelpers.h>

#include "../OffscreenSurface.h"
#include "../Logging.h"

#include "Profiling.h"
#include "RenderControl.h"
#include "RenderEventHandler.h"
#include "TextureCache.h"
#include <ThreadHelpers.h>

// Time between receiving a request to render the offscreen UI actually triggering
// the render.  Could possibly be increased depending on the framerate we expect to
// achieve.
// This has the effect of capping the framerate at 200
static const int MIN_TIMER_MS = 5;

using namespace hifi::qml;
using namespace hifi::qml::impl;

TextureCache& SharedObject::getTextureCache() {
    static TextureCache offscreenTextures;
    return offscreenTextures;
}

#define OFFSCREEN_QML_SHARED_CONTEXT_PROPERTY "com.highfidelity.qml.gl.sharedContext"
void SharedObject::setSharedContext(QOpenGLContext* sharedContext) {
    qApp->setProperty(OFFSCREEN_QML_SHARED_CONTEXT_PROPERTY, QVariant::fromValue<void*>(sharedContext));
    if (QOpenGLContextWrapper::currentContext() != sharedContext) {
        qFatal("The shared context must be the current context when setting");
    }
}

QOpenGLContext* SharedObject::getSharedContext() {
    return static_cast<QOpenGLContext*>(qApp->property(OFFSCREEN_QML_SHARED_CONTEXT_PROPERTY).value<void*>());
}

SharedObject::SharedObject() {
#ifndef DISABLE_QML

    // Create render control
    _renderControl = new RenderControl();

    // Create a QQuickWindow that is associated with our render control.
    // This window never gets created or shown, meaning that it will never get an underlying native (platform) window.
    // NOTE: Must be created on the main thread so that OffscreenQmlSurface can send it events
    // NOTE: Must be created on the rendering thread or it will refuse to render,
    //       so we wait until after its ctor to move object/context to this thread.
    QQuickWindow::setDefaultAlphaBuffer(true);
    _quickWindow = new QQuickWindow(_renderControl);
    _quickWindow->setFormat(getDefaultOpenGLSurfaceFormat());
    _quickWindow->setColor(QColor(255, 255, 255, 0));
    _quickWindow->setClearBeforeRendering(true);

#endif

    QObject::connect(qApp, &QCoreApplication::aboutToQuit, this, &SharedObject::onAboutToQuit);
}

SharedObject::~SharedObject() {
    // After destroy returns, the rendering thread should be gone
    destroy();

    // _renderTimer is created with `this` as the parent, so need no explicit destruction
#ifndef DISABLE_QML
    // Destroy the event hand
    if (_renderObject) {
        delete _renderObject;
        _renderObject = nullptr;
    }

    if (_renderControl) {
        delete _renderControl;
        _renderControl = nullptr;
    }
#endif

    // already deleted objects will be reset to null by QPointer so it should be safe just iterate here
    for (auto qmlObject : _deletionList) {
        if (qmlObject) {
            // manually delete not-deleted-yet qml items
            QQmlEngine::setObjectOwnership(qmlObject, QQmlEngine::CppOwnership);
            delete qmlObject;
        }
    }

    if (_rootItem) {
        delete _rootItem;
        _rootItem = nullptr;
    }

#ifndef DISABLE_QML
    if (_quickWindow) {
        _quickWindow->destroy();
        delete _quickWindow;
        _quickWindow = nullptr;
    }
#endif
    if (_qmlContext) {
        auto engine = _qmlContext->engine();
        delete _qmlContext;
        _qmlContext = nullptr;
        releaseEngine(engine);
    }
}

void SharedObject::create(OffscreenSurface* surface) {
    if (_rootItem) {
        qFatal("QML surface root item already set");
    }

#ifndef DISABLE_QML
    QObject::connect(_quickWindow, &QQuickWindow::focusObjectChanged, surface, &OffscreenSurface::onFocusObjectChanged);
#endif

    // Create a QML engine.
    auto qmlEngine = acquireEngine(surface);
    {
        PROFILE_RANGE(startup, "new QQmlContext");
        _qmlContext = new QQmlContext(qmlEngine->rootContext(), qmlEngine);
    }
    surface->onRootContextCreated(_qmlContext);
    emit surface->rootContextCreated(_qmlContext);

#ifndef DISABLE_QML
    if (!qmlEngine->incubationController()) {
        qmlEngine->setIncubationController(_quickWindow->incubationController());
    }
    _qmlContext->setContextProperty("offscreenWindow", QVariant::fromValue(_quickWindow));
#endif
}

void SharedObject::setRootItem(QQuickItem* rootItem) {
    if (_quit) {
        return;
    }

    _rootItem = rootItem;
#ifndef DISABLE_QML
    _rootItem->setSize(_quickWindow->size());

    // Create the render thread
    _renderThread = new QThread();
    QString name = objectName();
    _renderThread->setObjectName(name);
    QObject::connect(_renderThread, &QThread::started, [name] { setThreadName("QML SharedObject " + name.toStdString()); });
    _renderThread->start();

    // Create event handler for the render thread
    _renderObject = new RenderEventHandler(this, _renderThread);
    QCoreApplication::postEvent(this, new OffscreenEvent(OffscreenEvent::Initialize));

    QObject::connect(_renderControl, &QQuickRenderControl::renderRequested, this, &SharedObject::requestRender);
    QObject::connect(_renderControl, &QQuickRenderControl::sceneChanged, this, &SharedObject::requestRenderSync);
#endif
}

void SharedObject::destroy() {
    // `destroy` is idempotent, it can be called multiple times without issues
    if (_quit) {
        return;
    }

    if (!_rootItem) {
        return;
    }

    _paused = true;
#ifndef DISABLE_QML
    if (_renderTimer) {
        _renderTimer->stop();
        QObject::disconnect(_renderTimer);
    }

    if (_renderControl) {
        QObject::disconnect(_renderControl);
    }
#endif

    QObject::disconnect(qApp);

#ifndef DISABLE_QML
    {
        QMutexLocker lock(&_mutex);
        _quit = true;
        if (_renderObject) {
            QCoreApplication::postEvent(_renderObject, new OffscreenEvent(OffscreenEvent::Quit), Qt::HighEventPriority);
        }
    }
    // Block until the rendering thread has stopped
    // FIXME this is undesirable because this is blocking the main thread,
    // but I haven't found a reliable way to do this only at application
    // shutdown
    if (_renderThread) {
        _renderThread->wait();
        delete _renderThread;
        _renderThread = nullptr;
    }
#endif
}

#define SINGLE_QML_ENGINE 0

#if SINGLE_QML_ENGINE
static QQmlEngine* globalEngine{ nullptr };
static size_t globalEngineRefCount{ 0 };
#endif

QQmlEngine* SharedObject::acquireEngine(OffscreenSurface* surface) {
    PROFILE_RANGE(startup, "acquireEngine");
    Q_ASSERT(QThread::currentThread() == qApp->thread());

    QQmlEngine* result = nullptr;
    if (QThread::currentThread() != qApp->thread()) {
        qCWarning(qmlLogging) << "Cannot acquire QML engine on any thread but the main thread";
    }

#if SINGLE_QML_ENGINE
    if (!globalEngine) {
        Q_ASSERT(0 == globalEngineRefCount);
        globalEngine = new QQmlEngine();
        surface->initializeEngine(result);
    }
    ++globalEngineRefCount;
    result = globalEngine;
#else
    result = new QQmlEngine();
    surface->initializeEngine(result);
#endif

    return result;
}

void SharedObject::releaseEngine(QQmlEngine* engine) {
    Q_ASSERT(QThread::currentThread() == qApp->thread());
#if SINGLE_QML_ENGINE
    Q_ASSERT(0 != globalEngineRefCount);
    if (0 == --globalEngineRefCount) {
        globalEngine->deleteLater();
        globalEngine = nullptr;
    }
#else
    engine->deleteLater();
#endif
}

bool SharedObject::event(QEvent* e) {
#ifndef DISABLE_QML
    switch (static_cast<OffscreenEvent::Type>(e->type())) {
        case OffscreenEvent::Initialize:
            onInitialize();
            return true;

        case OffscreenEvent::Render:
            onRender();
            return true;

        default:
            break;
    }
#endif
    return QObject::event(e);
}

// Called by the render event handler, from the render thread
void SharedObject::initializeRenderControl(QOpenGLContext* context) {
    if (context->shareContext() != getSharedContext()) {
        qFatal("QML rendering context has no share context");
    }

#ifndef DISABLE_QML
    if (!nsightActive()) {
        _renderControl->initialize(context);
    }
#endif
}

void SharedObject::releaseTextureAndFence() {
#ifndef DISABLE_QML
    QMutexLocker lock(&_mutex);
    // If the most recent texture was unused, we can directly recycle it
    if (_latestTextureAndFence.first) {
        getTextureCache().releaseTexture(_latestTextureAndFence);
        _latestTextureAndFence = TextureAndFence{ 0, 0 };
    }
#endif
}

void SharedObject::setRenderTarget(uint32_t fbo, const QSize& size) {
#ifndef DISABLE_QML
    _quickWindow->setRenderTarget(fbo, size);
#endif
}

QSize SharedObject::getSize() const {
    QMutexLocker locker(&_mutex);
    return _size;
}

void SharedObject::setSize(const QSize& size) {
    if (getSize() == size) {
        return;
    }

    {
        QMutexLocker locker(&_mutex);
        _size = size;
    }

    qCDebug(qmlLogging) << "Offscreen UI resizing to " << size.width() << "x" << size.height();
#ifndef DISABLE_QML
    _quickWindow->setGeometry(QRect(QPoint(), size));
    _quickWindow->contentItem()->setSize(size);

    if (_rootItem) {
        _qmlContext->setContextProperty("surfaceSize", size);
        _rootItem->setSize(size);
    }

    requestRenderSync();
#endif
}

void SharedObject::setMaxFps(uint8_t maxFps) {
    QMutexLocker locker(&_mutex);
    _maxFps = maxFps;
}

bool SharedObject::preRender(bool sceneGraphSync) {
#ifndef DISABLE_QML
    QMutexLocker lock(&_mutex);
    if (_paused) {
        if (sceneGraphSync) {
            wake();
        }
        return false;
    }

    if (sceneGraphSync) {
        bool syncResult = true;
        if (!nsightActive()) {
            PROFILE_RANGE(render_qml_gl, "sync")
            syncResult = _renderControl->sync();
        }
        wake();
        if (!syncResult) {
            return false;
        }
    }
#endif

    return true;
}

void SharedObject::shutdownRendering(const QSize& size) {
    QMutexLocker locker(&_mutex);
    if (size != QSize()) {
        getTextureCache().releaseSize(size);
        if (_latestTextureAndFence.first) {
            getTextureCache().releaseTexture(_latestTextureAndFence);
        }
    }
#ifndef DISABLE_QML
    _renderControl->invalidate();
#endif
    wake();
}

bool SharedObject::isQuit() const {
    QMutexLocker locker(&_mutex);
    return _quit;
}

void SharedObject::requestRender() {
    if (_quit) {
        return;
    }
    _renderRequested = true;
}

void SharedObject::requestRenderSync() {
    if (_quit) {
        return;
    }
    _renderRequested = true;
    _syncRequested = true;
}

bool SharedObject::fetchTexture(TextureAndFence& textureAndFence) {
    QMutexLocker locker(&_mutex);
    if (!_latestTextureAndFence.first) {
        return false;
    }
    textureAndFence = { 0, 0 };
    std::swap(textureAndFence, _latestTextureAndFence);
    return true;
}

void SharedObject::addToDeletionList(QObject* object) {
    _deletionList.append(QPointer<QObject>(object));
}

void SharedObject::setProxyWindow(QWindow* window) {
#ifndef DISABLE_QML
    _proxyWindow = window;
    _renderControl->setRenderWindow(window);
#endif
}

void SharedObject::wait() {
    _cond.wait(&_mutex);
}

void SharedObject::wake() {
    _cond.wakeOne();
}

void SharedObject::onInitialize() {
#ifndef DISABLE_QML
    // Associate root item with the window.
    _rootItem->setParentItem(_quickWindow->contentItem());
    _renderControl->prepareThread(_renderThread);

    // Set up the render thread
    QCoreApplication::postEvent(_renderObject, new OffscreenEvent(OffscreenEvent::Initialize));

    requestRender();

    // Set up timer to trigger renders
    _renderTimer = new QTimer(this);
    QObject::connect(_renderTimer, &QTimer::timeout, this, &SharedObject::onTimer);

    _renderTimer->setTimerType(Qt::PreciseTimer);
    _renderTimer->setInterval(MIN_TIMER_MS);  // 5ms, Qt::PreciseTimer required
    _renderTimer->start();
#endif
}

void SharedObject::onRender() {
#ifndef DISABLE_QML
    PROFILE_RANGE(render_qml, __FUNCTION__);
    if (_quit) {
        return;
    }

    if (_syncRequested) {
        _renderControl->polishItems();
        QMutexLocker lock(&_mutex);
        QCoreApplication::postEvent(_renderObject, new OffscreenEvent(OffscreenEvent::RenderSync));
        // sync and render request, main and render threads must be synchronized
        wait();
        _syncRequested = false;
    } else {
        QCoreApplication::postEvent(_renderObject, new OffscreenEvent(OffscreenEvent::Render));
    }
    _renderRequested = false;
#endif
}

void SharedObject::onTimer() {
    getTextureCache().report();
    if (!_renderRequested) {
        return;
    }

    {
        QMutexLocker locker(&_mutex);
        // Don't queue more than one frame at a time
        if (_latestTextureAndFence.first) {
            return;
        }

        if (!_maxFps) {
            return;
        }
        auto minRenderInterval = USECS_PER_SECOND / _maxFps;
        auto lastInterval = usecTimestampNow() - _lastRenderTime;
        // Don't exceed the framerate limit
        if (lastInterval < minRenderInterval) {
            return;
        }
    }

#ifndef DISABLE_QML
    QCoreApplication::postEvent(this, new OffscreenEvent(OffscreenEvent::Render));
#endif
}

void SharedObject::onAboutToQuit() {
    destroy();
}

void SharedObject::updateTextureAndFence(const TextureAndFence& newTextureAndFence) {
    QMutexLocker locker(&_mutex);
    // If the most recent texture was unused, we can directly recycle it
    if (_latestTextureAndFence.first) {
        getTextureCache().releaseTexture(_latestTextureAndFence);
        _latestTextureAndFence = { 0, 0 };
    }

    _latestTextureAndFence = newTextureAndFence;
}

void SharedObject::pause() {
    _paused = true;
}

void SharedObject::resume() {
    _paused = false;
    requestRender();
}

bool SharedObject::isPaused() const {
    return _paused;
}
