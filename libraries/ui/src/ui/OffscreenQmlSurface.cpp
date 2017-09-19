//
//  Created by Bradley Austin Davis on 2015-05-13
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "OffscreenQmlSurface.h"
#include "ImageProvider.h"

// Has to come before Qt GL includes
#include <gl/Config.h>

#include <unordered_set>
#include <unordered_map>

#include <QtWidgets/QWidget>
#include <QtQml/QtQml>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlComponent>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>
#include <QtQuick/QQuickRenderControl>
#include <QtCore/QThread>
#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>

#include <shared/NsightHelpers.h>
#include <shared/GlobalAppProperties.h>
#include <shared/QtHelpers.h>
#include <PerfStat.h>
#include <DependencyManager.h>
#include <NumericalConstants.h>
#include <Finally.h>
#include <PathUtils.h>
#include <AbstractUriHandler.h>
#include <AccountManager.h>
#include <NetworkAccessManager.h>
#include <GLMHelpers.h>

#include <gl/OffscreenGLCanvas.h>
#include <gl/GLHelpers.h>
#include <gl/Context.h>

#include "types/FileTypeProfile.h"
#include "types/HFWebEngineProfile.h"
#include "types/SoundEffect.h"

#include "Logging.h"

Q_LOGGING_CATEGORY(trace_render_qml, "trace.render.qml")
Q_LOGGING_CATEGORY(trace_render_qml_gl, "trace.render.qml.gl")
Q_LOGGING_CATEGORY(offscreenFocus, "hifi.offscreen.focus")

struct TextureSet {
    // The number of surfaces with this size
    size_t count { 0 };
    std::list<OffscreenQmlSurface::TextureAndFence> returnedTextures;
};

uint64_t uvec2ToUint64(const uvec2& v) {
    uint64_t result = v.x;
    result <<= 32;
    result |= v.y;
    return result;
}

class OffscreenTextures {
public:
    GLuint getNextTexture(const uvec2& size) {
        assert(QThread::currentThread() == qApp->thread());

        recycle();

        ++_activeTextureCount;
        auto sizeKey = uvec2ToUint64(size);
        assert(_textures.count(sizeKey));
        auto& textureSet = _textures[sizeKey];
        if (!textureSet.returnedTextures.empty()) {
            auto textureAndFence = textureSet.returnedTextures.front();
            textureSet.returnedTextures.pop_front();
            waitOnFence(static_cast<GLsync>(textureAndFence.second));
            return textureAndFence.first;
        }

        return createTexture(size);
    }

    void releaseSize(const uvec2& size) {
        assert(QThread::currentThread() == qApp->thread());
        auto sizeKey = uvec2ToUint64(size);
        assert(_textures.count(sizeKey));
        auto& textureSet = _textures[sizeKey];
        if (0 == --textureSet.count) {
            for (const auto& textureAndFence : textureSet.returnedTextures) {
                destroy(textureAndFence);
            }
            _textures.erase(sizeKey);
        }
    }

    void acquireSize(const uvec2& size) {
        assert(QThread::currentThread() == qApp->thread());
        auto sizeKey = uvec2ToUint64(size);
        auto& textureSet = _textures[sizeKey];
        ++textureSet.count;
    }

    // May be called on any thread
    void releaseTexture(const OffscreenQmlSurface::TextureAndFence & textureAndFence) {
        --_activeTextureCount;
        Lock lock(_mutex);
        _returnedTextures.push_back(textureAndFence);
    }

    void report() {
        if (randFloat() < 0.01f) {
            PROFILE_COUNTER(render_qml_gl, "offscreenTextures", {
                { "total", QVariant::fromValue(_allTextureCount.load()) },
                { "active", QVariant::fromValue(_activeTextureCount.load()) },
            });
            PROFILE_COUNTER(render_qml_gl, "offscreenTextureMemory", {
                { "value", QVariant::fromValue(_totalTextureUsage) }
            });
        }
    }

    size_t getUsedTextureMemory() { return _totalTextureUsage; }
private:
    static void waitOnFence(GLsync fence) {
        glWaitSync(fence, 0, GL_TIMEOUT_IGNORED);
        glDeleteSync(fence);
    }

    static size_t getMemoryForSize(const uvec2& size) {
        // Base size + mips
        return static_cast<size_t>(((size.x * size.y) << 2) * 1.33f);
    }

    void destroyTexture(GLuint texture) {
        --_allTextureCount;
        auto size = _textureSizes[texture];
        assert(getMemoryForSize(size) <= _totalTextureUsage);
        _totalTextureUsage -= getMemoryForSize(size);
        _textureSizes.erase(texture);
        glDeleteTextures(1, &texture);
    }

    void destroy(const OffscreenQmlSurface::TextureAndFence& textureAndFence) {
        waitOnFence(static_cast<GLsync>(textureAndFence.second));
        destroyTexture(textureAndFence.first);
    }

    GLuint createTexture(const uvec2& size) {
        // Need a new texture
        uint32_t newTexture;
        glGenTextures(1, &newTexture);
        ++_allTextureCount;
        _textureSizes[newTexture] = size;
        _totalTextureUsage += getMemoryForSize(size);
        glBindTexture(GL_TEXTURE_2D, newTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 8.0f);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, -0.2f);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 8.0f);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
        return newTexture;
    }

    void recycle() {
        assert(QThread::currentThread() == qApp->thread());
        // First handle any global returns
        std::list<OffscreenQmlSurface::TextureAndFence> returnedTextures;
        {
            Lock lock(_mutex);
            returnedTextures.swap(_returnedTextures);
        }

        for (auto textureAndFence : returnedTextures) {
            GLuint texture = textureAndFence.first;
            uvec2 size = _textureSizes[texture];
            auto sizeKey = uvec2ToUint64(size);
            // Textures can be returned after all surfaces of the given size have been destroyed,
            // in which case we just destroy the texture
            if (!_textures.count(sizeKey)) {
                destroy(textureAndFence);
                continue;
            }
            _textures[sizeKey].returnedTextures.push_back(textureAndFence);
        }
    }

    using Mutex = std::mutex;
    using Lock = std::unique_lock<Mutex>;
    std::atomic<int> _allTextureCount;
    std::atomic<int> _activeTextureCount;
    std::unordered_map<uint64_t, TextureSet> _textures;
    std::unordered_map<GLuint, uvec2> _textureSizes;
    Mutex _mutex;
    std::list<OffscreenQmlSurface::TextureAndFence> _returnedTextures;
    size_t _totalTextureUsage { 0 };
} offscreenTextures;

class UrlHandler : public QObject {
    Q_OBJECT
public:
    Q_INVOKABLE bool canHandleUrl(const QString& url) {
        static auto handler = dynamic_cast<AbstractUriHandler*>(qApp);
        return handler->canAcceptURL(url);
    }

    Q_INVOKABLE bool handleUrl(const QString& url) {
        static auto handler = dynamic_cast<AbstractUriHandler*>(qApp);
        return handler->acceptURL(url);
    }
};

// Time between receiving a request to render the offscreen UI actually triggering
// the render.  Could possibly be increased depending on the framerate we expect to
// achieve.
// This has the effect of capping the framerate at 200
static const int MIN_TIMER_MS = 5;

class QMyQuickRenderControl : public QQuickRenderControl {
protected:
    QWindow* renderWindow(QPoint* offset) Q_DECL_OVERRIDE{
        if (nullptr == _renderWindow) {
            return QQuickRenderControl::renderWindow(offset);
        }
        if (nullptr != offset) {
            offset->rx() = offset->ry() = 0;
        }
        return _renderWindow;
    }

private:
    QWindow* _renderWindow{ nullptr };
    friend class OffscreenQmlRenderThread;
    friend class OffscreenQmlSurface;
};

size_t OffscreenQmlSurface::getUsedTextureMemory() {
    return offscreenTextures.getUsedTextureMemory();
}

class QmlNetworkAccessManager : public NetworkAccessManager {
public:
    friend class QmlNetworkAccessManagerFactory;
protected:
    QmlNetworkAccessManager(QObject* parent) : NetworkAccessManager(parent) { }
};

class QmlNetworkAccessManagerFactory : public QQmlNetworkAccessManagerFactory {
public:
    QNetworkAccessManager* create(QObject* parent) override;
};

QNetworkAccessManager* QmlNetworkAccessManagerFactory::create(QObject* parent) {
    return new QmlNetworkAccessManager(parent);
}

QString getEventBridgeJavascript() {
    // FIXME: Refactor with similar code in RenderableWebEntityItem
    QString javaScriptToInject;
    QFile webChannelFile(":qtwebchannel/qwebchannel.js");
    QFile createGlobalEventBridgeFile(PathUtils::resourcesPath() + "/html/createGlobalEventBridge.js");
    if (webChannelFile.open(QFile::ReadOnly | QFile::Text) &&
        createGlobalEventBridgeFile.open(QFile::ReadOnly | QFile::Text)) {
        QString webChannelStr = QTextStream(&webChannelFile).readAll();
        QString createGlobalEventBridgeStr = QTextStream(&createGlobalEventBridgeFile).readAll();
        javaScriptToInject = webChannelStr + createGlobalEventBridgeStr;
    } else {
        qCWarning(uiLogging) << "Unable to find qwebchannel.js or createGlobalEventBridge.js";
    }
    return javaScriptToInject;
}

class EventBridgeWrapper : public QObject {
    Q_OBJECT
    Q_PROPERTY(QObject* eventBridge READ getEventBridge CONSTANT);

public:
    EventBridgeWrapper(QObject* eventBridge, QObject* parent = nullptr) : QObject(parent), _eventBridge(eventBridge) {
    }

    QObject* getEventBridge() {
        return _eventBridge;
    }

private:
    QObject* _eventBridge;
};



#define SINGLE_QML_ENGINE 0

#if SINGLE_QML_ENGINE
static QQmlEngine* globalEngine{ nullptr };
static size_t globalEngineRefCount{ 0 };
#endif

void initializeQmlEngine(QQmlEngine* engine, QQuickWindow* window) {
    // register the pixmap image provider (used only for security image, for now)
    engine->addImageProvider(ImageProvider::PROVIDER_NAME, new ImageProvider());

    engine->setNetworkAccessManagerFactory(new QmlNetworkAccessManagerFactory);
    auto importList = engine->importPathList();
    importList.insert(importList.begin(), PathUtils::resourcesPath());
    engine->setImportPathList(importList);
    for (const auto& path : importList) {
        qDebug() << path;
    }

    if (!engine->incubationController()) {
        engine->setIncubationController(window->incubationController());
    }
    auto rootContext = engine->rootContext();
    rootContext->setContextProperty("GL", ::getGLContextData());
    rootContext->setContextProperty("urlHandler", new UrlHandler());
    rootContext->setContextProperty("resourceDirectoryUrl", QUrl::fromLocalFile(PathUtils::resourcesPath()));
    rootContext->setContextProperty("pathToFonts", "../../");
    rootContext->setContextProperty("ApplicationInterface", qApp);
    auto javaScriptToInject = getEventBridgeJavascript();
    if (!javaScriptToInject.isEmpty()) {
        rootContext->setContextProperty("eventBridgeJavaScriptToInject", QVariant(javaScriptToInject));
    }
    rootContext->setContextProperty("FileTypeProfile", new FileTypeProfile(rootContext));
    rootContext->setContextProperty("HFWebEngineProfile", new HFWebEngineProfile(rootContext));
    rootContext->setContextProperty("Paths", DependencyManager::get<PathUtils>().data());
}

QQmlEngine* acquireEngine(QQuickWindow* window) {
    Q_ASSERT(QThread::currentThread() == qApp->thread());

    QQmlEngine* result = nullptr;
    if (QThread::currentThread() != qApp->thread()) {
        qCWarning(uiLogging) << "Cannot acquire QML engine on any thread but the main thread";
    }
    static std::once_flag once;
    std::call_once(once, [] {
        qmlRegisterType<SoundEffect>("Hifi", 1, 0, "SoundEffect");
    });


#if SINGLE_QML_ENGINE
    if (!globalEngine) {
        Q_ASSERT(0 == globalEngineRefCount);
        globalEngine = new QQmlEngine();
        initializeQmlEngine(result, window);
        ++globalEngineRefCount;
    }
    result = globalEngine;
#else
    result = new QQmlEngine();
    initializeQmlEngine(result, window);
#endif

    return result;
}

void releaseEngine(QQmlEngine* engine) {
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

#define OFFSCREEN_QML_SHARED_CONTEXT_PROPERTY "com.highfidelity.qml.gl.sharedContext"
void OffscreenQmlSurface::setSharedContext(QOpenGLContext* sharedContext) {
    qApp->setProperty(OFFSCREEN_QML_SHARED_CONTEXT_PROPERTY, QVariant::fromValue<void*>(sharedContext));
}

QOpenGLContext* OffscreenQmlSurface::getSharedContext() {
    return static_cast<QOpenGLContext*>(qApp->property(OFFSCREEN_QML_SHARED_CONTEXT_PROPERTY).value<void*>());
}

void OffscreenQmlSurface::cleanup() {
    _canvas->makeCurrent();

    _renderControl->invalidate();
    delete _renderControl; // and invalidate

    if (_depthStencil) {
        glDeleteRenderbuffers(1, &_depthStencil);
        _depthStencil = 0;
    }
    if (_fbo) {
        glDeleteFramebuffers(1, &_fbo);
        _fbo = 0;
    }

    offscreenTextures.releaseSize(_size);

    _canvas->doneCurrent();
}

void OffscreenQmlSurface::render() {
    if (nsightActive()) {
        return;
    }
    if (_paused) {
        return;
    }

    PROFILE_RANGE(render_qml_gl, __FUNCTION__)
    _canvas->makeCurrent();

    _renderControl->sync();
    _quickWindow->setRenderTarget(_fbo, QSize(_size.x, _size.y));

    GLuint texture = offscreenTextures.getNextTexture(_size);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _fbo);
    glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    _renderControl->render();
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);


    {
        // If the most recent texture was unused, we can directly recycle it
        if (_latestTextureAndFence.first) {
            offscreenTextures.releaseTexture(_latestTextureAndFence);
            _latestTextureAndFence = { 0, 0 };
        }

        _latestTextureAndFence = { texture, glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0) };
        // Fence will be used in another thread / context, so a flush is required
        glFlush();
    }

    _quickWindow->resetOpenGLState();
    _lastRenderTime = usecTimestampNow();
    _canvas->doneCurrent();
}

bool OffscreenQmlSurface::fetchTexture(TextureAndFence& textureAndFence) {
    textureAndFence = { 0, 0 };

    if (0 == _latestTextureAndFence.first) {
        return false;
    }

    // Ensure writes to the latest texture are complete before before returning it for reading
    textureAndFence = _latestTextureAndFence;
    _latestTextureAndFence = { 0, 0 };
    return true;
}

std::function<void(uint32_t, void*)> OffscreenQmlSurface::getDiscardLambda() {
    return [](uint32_t texture, void* fence) {
        offscreenTextures.releaseTexture({ texture, static_cast<GLsync>(fence) });
    };
}

bool OffscreenQmlSurface::allowNewFrame(uint8_t fps) {
    // If we already have a pending texture, don't render another one
    // i.e. don't render faster than the consumer context, since it wastes
    // GPU cycles on producing output that will never be seen
    if (0 != _latestTextureAndFence.first) {
        return false;
    }

    auto minRenderInterval = USECS_PER_SECOND / fps;
    auto lastInterval = usecTimestampNow() - _lastRenderTime;
    return (lastInterval > minRenderInterval);
}

OffscreenQmlSurface::OffscreenQmlSurface() {
}

OffscreenQmlSurface::~OffscreenQmlSurface() {
    QObject::disconnect(&_updateTimer);
    QObject::disconnect(qApp);

    cleanup();
    auto engine = _qmlContext->engine();
    _canvas->deleteLater();
    _rootItem->deleteLater();
    _quickWindow->deleteLater();
    releaseEngine(engine);
}

void OffscreenQmlSurface::onAboutToQuit() {
    _paused = true;
    QObject::disconnect(&_updateTimer);
}

void OffscreenQmlSurface::create() {
    qCDebug(uiLogging) << "Building QML surface";

    _renderControl = new QMyQuickRenderControl();
    connect(_renderControl, &QQuickRenderControl::renderRequested, this, [this] { _render = true; });
    connect(_renderControl, &QQuickRenderControl::sceneChanged, this, [this] { _render = _polish = true; });

    QQuickWindow::setDefaultAlphaBuffer(true);

    // Create a QQuickWindow that is associated with our render control.
    // This window never gets created or shown, meaning that it will never get an underlying native (platform) window.
    // NOTE: Must be created on the main thread so that OffscreenQmlSurface can send it events
    // NOTE: Must be created on the rendering thread or it will refuse to render,
    //       so we wait until after its ctor to move object/context to this thread.
    _quickWindow = new QQuickWindow(_renderControl);
    _quickWindow->setColor(QColor(255, 255, 255, 0));
    _quickWindow->setClearBeforeRendering(false);

    _renderControl->_renderWindow = _proxyWindow;

    _canvas = new OffscreenGLCanvas();
    if (!_canvas->create(getSharedContext())) {
        qFatal("Failed to create OffscreenGLCanvas");
        return;
    };

    connect(_quickWindow, &QQuickWindow::focusObjectChanged, this, &OffscreenQmlSurface::onFocusObjectChanged);

    // acquireEngine interrogates the GL context, so we need to have the context current here
    if (!_canvas->makeCurrent()) {
        qFatal("Failed to make context current for QML Renderer");
        return;
    }
    
    // Create a QML engine.
    auto qmlEngine = acquireEngine(_quickWindow);

    _qmlContext = new QQmlContext(qmlEngine->rootContext());

    _qmlContext->setContextProperty("offscreenWindow", QVariant::fromValue(getWindow()));
    _qmlContext->setContextProperty("eventBridge", this);
    _qmlContext->setContextProperty("webEntity", this);

    // FIXME Compatibility mechanism for existing HTML and JS that uses eventBridgeWrapper
    // Find a way to flag older scripts using this mechanism and wanr that this is deprecated
    _qmlContext->setContextProperty("eventBridgeWrapper", new EventBridgeWrapper(this, _qmlContext));
    _renderControl->initialize(_canvas->getContext());

    // When Quick says there is a need to render, we will not render immediately. Instead,
    // a timer with a small interval is used to get better performance.
    QObject::connect(&_updateTimer, &QTimer::timeout, this, &OffscreenQmlSurface::updateQuick);
    QObject::connect(qApp, &QCoreApplication::aboutToQuit, this, &OffscreenQmlSurface::onAboutToQuit);
    _updateTimer.setTimerType(Qt::PreciseTimer);
    _updateTimer.setInterval(MIN_TIMER_MS); // 5ms, Qt::PreciseTimer required
    _updateTimer.start();
}

static uvec2 clampSize(const uvec2& size, uint32_t maxDimension) {
    return glm::clamp(size, glm::uvec2(1), glm::uvec2(maxDimension));
}

static QSize clampSize(const QSize& qsize, uint32_t maxDimension) {
    return fromGlm(clampSize(toGlm(qsize), maxDimension));
}

void OffscreenQmlSurface::resize(const QSize& newSize_, bool forceResize) {

    if (!_quickWindow) {
        return;
    }

    const uint32_t MAX_OFFSCREEN_DIMENSION = 4096;
    const QSize newSize = clampSize(newSize_, MAX_OFFSCREEN_DIMENSION);
    if (!forceResize && newSize == _quickWindow->geometry().size()) {
        return;
    }

    _qmlContext->setContextProperty("surfaceSize", newSize);

    if (_rootItem) {
        _rootItem->setSize(newSize);
    }

    // Update our members
    _quickWindow->setGeometry(QRect(QPoint(), newSize));
    _quickWindow->contentItem()->setSize(newSize);

    // Qt bug in 5.4 forces this check of pixel ratio,
    // even though we're rendering offscreen.
    uvec2 newOffscreenSize = toGlm(newSize);
    if (newOffscreenSize == _size) {
        return;
    }

    qCDebug(uiLogging) << "Offscreen UI resizing to " << newSize.width() << "x" << newSize.height();
    gl::withSavedContext([&] {
        _canvas->makeCurrent();

        // Release hold on the textures of the old size
        if (uvec2() != _size) {
            // If the most recent texture was unused, we can directly recycle it
            if (_latestTextureAndFence.first) {
                offscreenTextures.releaseTexture(_latestTextureAndFence);
                _latestTextureAndFence = { 0, 0 };
            }
            offscreenTextures.releaseSize(_size);
        }

        _size = newOffscreenSize;

        // Acquire the new texture size
        if (uvec2() != _size) {
            offscreenTextures.acquireSize(_size);
            if (_depthStencil) {
                glDeleteRenderbuffers(1, &_depthStencil);
                _depthStencil = 0;
            }
            glGenRenderbuffers(1, &_depthStencil);
            glBindRenderbuffer(GL_RENDERBUFFER, _depthStencil);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, _size.x, _size.y);
            if (!_fbo) {
                glGenFramebuffers(1, &_fbo);
            }
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _fbo);
            glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _depthStencil);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        }

        _canvas->doneCurrent();
    });
}

QQuickItem* OffscreenQmlSurface::getRootItem() {
    return _rootItem;
}

void OffscreenQmlSurface::setBaseUrl(const QUrl& baseUrl) {
    _qmlContext->setBaseUrl(baseUrl);
}

void OffscreenQmlSurface::load(const QUrl& qmlSource, bool createNewContext, std::function<void(QQmlContext*, QObject*)> onQmlLoadedCallback) {
    if (QThread::currentThread() != thread()) {
        qCWarning(uiLogging) << "Called load on a non-surface thread";
    }
    // Synchronous loading may take a while; restart the deadlock timer
    QMetaObject::invokeMethod(qApp, "updateHeartbeat", Qt::DirectConnection);

    QQmlContext* targetContext = _qmlContext;
    if (_rootItem && createNewContext) {
        targetContext = new QQmlContext(targetContext);
    }

    QUrl finalQmlSource = qmlSource;
    if ((qmlSource.isRelative() && !qmlSource.isEmpty()) || qmlSource.scheme() == QLatin1String("file")) {
        finalQmlSource = _qmlContext->resolvedUrl(qmlSource);
    }

    auto qmlComponent = new QQmlComponent(_qmlContext->engine(), finalQmlSource, QQmlComponent::PreferSynchronous);
    if (qmlComponent->isLoading()) {
        connect(qmlComponent, &QQmlComponent::statusChanged, this,
            [this, qmlComponent, targetContext, onQmlLoadedCallback](QQmlComponent::Status) {
            finishQmlLoad(qmlComponent, targetContext, onQmlLoadedCallback);
        });
        return;
    }

    finishQmlLoad(qmlComponent, targetContext, onQmlLoadedCallback);
}

void OffscreenQmlSurface::loadInNewContext(const QUrl& qmlSource, std::function<void(QQmlContext*, QObject*)> onQmlLoadedCallback) {
    load(qmlSource, true, onQmlLoadedCallback);
}

void OffscreenQmlSurface::load(const QUrl& qmlSource, std::function<void(QQmlContext*, QObject*)> onQmlLoadedCallback) {
    load(qmlSource, false, onQmlLoadedCallback);
}

void OffscreenQmlSurface::clearCache() {
    _qmlContext->engine()->clearComponentCache();
}

void OffscreenQmlSurface::finishQmlLoad(QQmlComponent* qmlComponent, QQmlContext* qmlContext, std::function<void(QQmlContext*, QObject*)> onQmlLoadedCallback) {
    disconnect(qmlComponent, &QQmlComponent::statusChanged, this, 0);
    if (qmlComponent->isError()) {
        for (const auto& error : qmlComponent->errors()) {
            qCWarning(uiLogging) << error.url() << error.line() << error;
        }
        qmlComponent->deleteLater();
        return;
    }

    QObject* newObject = qmlComponent->beginCreate(qmlContext);
    if (qmlComponent->isError()) {
        for (const auto& error : qmlComponent->errors()) {
            qCWarning(uiLogging) << error.url() << error.line() << error;
        }
        if (!_rootItem) {
            qFatal("Unable to finish loading QML root");
        }
        qmlComponent->deleteLater();
        return;
    }

    qmlContext->engine()->setObjectOwnership(this, QQmlEngine::CppOwnership);

    // All quick items should be focusable
    QQuickItem* newItem = qobject_cast<QQuickItem*>(newObject);
    if (newItem) {
        // Make sure we make items focusable (critical for
        // supporting keyboard shortcuts)
        newItem->setFlag(QQuickItem::ItemIsFocusScope, true);
    }

    // Make sure we will call callback for this codepath
    // Call this before qmlComponent->completeCreate() otherwise ghost window appears
    if (newItem && _rootItem) {
        onQmlLoadedCallback(qmlContext, newObject);
    }

    QObject* eventBridge = qmlContext->contextProperty("eventBridge").value<QObject*>();
    if (qmlContext != _qmlContext && eventBridge && eventBridge != this) {
        // FIXME Compatibility mechanism for existing HTML and JS that uses eventBridgeWrapper
        // Find a way to flag older scripts using this mechanism and wanr that this is deprecated
        qmlContext->setContextProperty("eventBridgeWrapper", new EventBridgeWrapper(eventBridge, qmlContext));
    }

    qmlComponent->completeCreate();
    qmlComponent->deleteLater();

    // If we already have a root, just set a couple of flags and the ancestry
    if (newItem && _rootItem) {
        // Allow child windows to be destroyed from JS
        QQmlEngine::setObjectOwnership(newObject, QQmlEngine::JavaScriptOwnership);
        newObject->setParent(_rootItem);
        if (newItem) {
            newItem->setParentItem(_rootItem);
        }
        return;
    }

    if (!newItem) {
        qFatal("Could not load object as root item");
        return;
    }

    connect(newItem, SIGNAL(sendToScript(QVariant)), this, SIGNAL(fromQml(QVariant)));

    // The root item is ready. Associate it with the window.
    _rootItem = newItem;
    _rootItem->setParentItem(_quickWindow->contentItem());
    _rootItem->setSize(_quickWindow->renderTargetSize());
    // Call this callback after rootitem is set, otherwise VrMenu wont work
    onQmlLoadedCallback(qmlContext, newObject);
}

void OffscreenQmlSurface::updateQuick() {
    offscreenTextures.report();
    // If we're
    //   a) not set up
    //   b) already rendering a frame
    //   c) rendering too fast
    // then skip this
    if (!allowNewFrame(_maxFps)) {
        return;
    }

    if (_polish) {
        PROFILE_RANGE(render_qml, "OffscreenQML polish")
        _renderControl->polishItems();
        _polish = false;
    }

    if (_render) {
        render();
        _render = false;
    }
}

QPointF OffscreenQmlSurface::mapWindowToUi(const QPointF& sourcePosition, QObject* sourceObject) {
    vec2 sourceSize;
    if (dynamic_cast<QWidget*>(sourceObject)) {
        sourceSize = toGlm(((QWidget*)sourceObject)->size());
    } else if (dynamic_cast<QWindow*>(sourceObject)) {
        sourceSize = toGlm(((QWindow*)sourceObject)->size());
    }
    vec2 offscreenPosition = toGlm(sourcePosition);
    offscreenPosition /= sourceSize;
    offscreenPosition *= vec2(toGlm(_quickWindow->size()));
    return QPointF(offscreenPosition.x, offscreenPosition.y);
}

QPointF OffscreenQmlSurface::mapToVirtualScreen(const QPointF& originalPoint, QObject* originalWidget) {
    return _mouseTranslator(originalPoint);
}

///////////////////////////////////////////////////////
//
// Event handling customization
//

bool OffscreenQmlSurface::filterEnabled(QObject* originalDestination, QEvent* event) const {
    if (_quickWindow == originalDestination) {
        return false;
    }
    // Only intercept events while we're in an active state
    if (_paused) {
        return false;
    }
    return true;
}

bool OffscreenQmlSurface::eventFilter(QObject* originalDestination, QEvent* event) {
    if (!filterEnabled(originalDestination, event)) {
        return false;
    }
#ifdef DEBUG
    // Don't intercept our own events, or we enter an infinite recursion
    QObject* recurseTest = originalDestination;
    while (recurseTest) {
        Q_ASSERT(recurseTest != _rootItem && recurseTest != _quickWindow);
        recurseTest = recurseTest->parent();
    }
#endif

    switch (event->type()) {
        case QEvent::Resize: {
            QResizeEvent* resizeEvent = static_cast<QResizeEvent*>(event);
            QWidget* widget = dynamic_cast<QWidget*>(originalDestination);
            if (widget) {
                this->resize(resizeEvent->size());
            }
            break;
        }

        case QEvent::KeyPress:
        case QEvent::KeyRelease: {
            event->ignore();
            if (QCoreApplication::sendEvent(_quickWindow, event)) {
                return event->isAccepted();
            }
            break;
        }

        case QEvent::Wheel: {
            QWheelEvent* wheelEvent = static_cast<QWheelEvent*>(event);
            QPointF transformedPos = mapToVirtualScreen(wheelEvent->pos(), originalDestination);
            QWheelEvent mappedEvent(
                    transformedPos,
                    wheelEvent->delta(),  wheelEvent->buttons(),
                    wheelEvent->modifiers(), wheelEvent->orientation());
            mappedEvent.ignore();
            if (QCoreApplication::sendEvent(_quickWindow, &mappedEvent)) {
                return mappedEvent.isAccepted();
            }
            break;
        }

        // Fall through
        case QEvent::MouseButtonDblClick:
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseMove: {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            QPointF transformedPos = mapToVirtualScreen(mouseEvent->localPos(), originalDestination);
            QMouseEvent mappedEvent(mouseEvent->type(),
                    transformedPos,
                    mouseEvent->screenPos(), mouseEvent->button(),
                    mouseEvent->buttons(), mouseEvent->modifiers());
            if (event->type() == QEvent::MouseMove) {
                // TODO - this line necessary for the QML Tooltop to work (which is not currently being used), but it causes interface to crash on launch on a fresh install
                // need to investigate into why this crash is happening.
                //_qmlContext->setContextProperty("lastMousePosition", transformedPos);
            }
            mappedEvent.ignore();
            if (QCoreApplication::sendEvent(_quickWindow, &mappedEvent)) {
                return mappedEvent.isAccepted();
            }
            break;
        }

        default:
            break;
    }

    return false;
}

void OffscreenQmlSurface::pause() {
    _paused = true;
}

void OffscreenQmlSurface::resume() {
    _paused = false;
    _render = true;

    if (getRootItem()) {
        getRootItem()->setProperty("eventBridge", QVariant::fromValue(this));
    }
}

bool OffscreenQmlSurface::isPaused() const {
    return _paused;
}

void OffscreenQmlSurface::setProxyWindow(QWindow* window) {
    _proxyWindow = window;
    if (_renderControl) {
        _renderControl->_renderWindow = window;
    }
}

QObject* OffscreenQmlSurface::getEventHandler() {
    return getWindow();
}

QQuickWindow* OffscreenQmlSurface::getWindow() {
    return _quickWindow;
}

QSize OffscreenQmlSurface::size() const {
    return _quickWindow->geometry().size();
}

QQmlContext* OffscreenQmlSurface::getSurfaceContext() {
    return _qmlContext;
}

void OffscreenQmlSurface::focusDestroyed(QObject *obj) {
    _currentFocusItem = nullptr;
}

void OffscreenQmlSurface::onFocusObjectChanged(QObject* object) {
    QQuickItem* item = dynamic_cast<QQuickItem*>(object);
    if (!item) {
        setFocusText(false);
        _currentFocusItem = nullptr;
        return;
    }

    QInputMethodQueryEvent query(Qt::ImEnabled);
    qApp->sendEvent(object, &query);
    setFocusText(query.value(Qt::ImEnabled).toBool());

    if (_currentFocusItem) {
        disconnect(_currentFocusItem, &QObject::destroyed, this, 0);
    }

    // Raise and lower keyboard for QML text fields.
    // HTML text fields are handled in emitWebEvent() methods - testing READ_ONLY_PROPERTY prevents action for HTML files.
    const char* READ_ONLY_PROPERTY = "readOnly";
    bool raiseKeyboard = item->hasActiveFocus() && item->property(READ_ONLY_PROPERTY) == false;
    if (_currentFocusItem && !raiseKeyboard) {
        setKeyboardRaised(_currentFocusItem, false);
    }
    setKeyboardRaised(item, raiseKeyboard);  // Always set focus so that alphabetic / numeric setting is updated.

    _currentFocusItem = item;
    connect(_currentFocusItem, &QObject::destroyed, this, &OffscreenQmlSurface::focusDestroyed);
}

void OffscreenQmlSurface::setFocusText(bool newFocusText) {
    if (newFocusText != _focusText) {
        _focusText = newFocusText;
        emit focusTextChanged(_focusText);
    }
}

// UTF-8 encoded symbols
static const uint8_t SHIFT_ARROW[] = { 0xE2, 0x87, 0xAA, 0x00 };
static const uint8_t NUMERIC_SHIFT_ARROW[] = { 0xE2, 0x87, 0xA8, 0x00 };
static const uint8_t BACKSPACE_SYMBOL[] = { 0xE2, 0x86, 0x90, 0x00 };
static const uint8_t LEFT_ARROW[] = { 0xE2, 0x9D, 0xAC, 0x00 };
static const uint8_t RIGHT_ARROW[] = { 0xE2, 0x9D, 0xAD, 0x00 };
static const uint8_t RETURN_SYMBOL[] = { 0xE2, 0x8F, 0x8E, 0x00 };
static const char PUNCTUATION_STRING[] = "123";
static const char ALPHABET_STRING[] = "abc";

static bool equals(const QByteArray& byteArray, const uint8_t* ptr) {
    int i;
    for (i = 0; i < byteArray.size(); i++) {
        if ((char)ptr[i] != byteArray[i]) {
            return false;
        }
    }
    return ptr[i] == 0x00;
}

void OffscreenQmlSurface::synthesizeKeyPress(QString key) {
    auto eventHandler = getEventHandler();
    if (eventHandler) {
        auto utf8Key = key.toUtf8();

        int scanCode = (int)utf8Key[0];
        QString keyString = key;
        if (equals(utf8Key, SHIFT_ARROW) || equals(utf8Key, NUMERIC_SHIFT_ARROW) ||
            equals(utf8Key, (uint8_t*)PUNCTUATION_STRING) || equals(utf8Key, (uint8_t*)ALPHABET_STRING)) {
            return;  // ignore
        } else if (equals(utf8Key, BACKSPACE_SYMBOL)) {
            scanCode = Qt::Key_Backspace;
            keyString = "\x08";
        } else if (equals(utf8Key, RETURN_SYMBOL)) {
            scanCode = Qt::Key_Return;
            keyString = "\x0d";
        } else if (equals(utf8Key, LEFT_ARROW)) {
            scanCode = Qt::Key_Left;
            keyString = "";
        } else if (equals(utf8Key, RIGHT_ARROW)) {
            scanCode = Qt::Key_Right;
            keyString = "";
        }

        QKeyEvent* pressEvent = new QKeyEvent(QEvent::KeyPress, scanCode, Qt::NoModifier, keyString);
        QKeyEvent* releaseEvent = new QKeyEvent(QEvent::KeyRelease, scanCode, Qt::NoModifier, keyString);
        QCoreApplication::postEvent(eventHandler, pressEvent);
        QCoreApplication::postEvent(eventHandler, releaseEvent);
    }
}

void OffscreenQmlSurface::setKeyboardRaised(QObject* object, bool raised, bool numeric) {
    if (!object) {
        return;
    }

    // if HMD is being worn, allow keyboard to open.  allow it to close, HMD or not.
    if (!raised || qApp->property(hifi::properties::HMD).toBool()) {
        QQuickItem* item = dynamic_cast<QQuickItem*>(object);
        while (item) {
            // Numeric value may be set in parameter from HTML UI; for QML UI, detect numeric fields here.
            numeric = numeric || QString(item->metaObject()->className()).left(7) == "SpinBox";

            if (item->property("keyboardRaised").isValid()) {
                // FIXME - HMD only: Possibly set value of "keyboardEnabled" per isHMDMode() for use in WebView.qml.
                if (item->property("punctuationMode").isValid()) {
                    item->setProperty("punctuationMode", QVariant(numeric));
                }
                item->setProperty("keyboardRaised", QVariant(raised));
                return;
            }
            item = dynamic_cast<QQuickItem*>(item->parentItem());
        }
    }
}

void OffscreenQmlSurface::emitScriptEvent(const QVariant& message) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "emitScriptEvent", Qt::QueuedConnection, Q_ARG(QVariant, message));
    } else {
        emit scriptEventReceived(message);
    }
}

void OffscreenQmlSurface::emitWebEvent(const QVariant& message) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "emitWebEvent", Qt::QueuedConnection, Q_ARG(QVariant, message));
    } else {
        // Special case to handle raising and lowering the virtual keyboard.
        const QString RAISE_KEYBOARD = "_RAISE_KEYBOARD";
        const QString RAISE_KEYBOARD_NUMERIC = "_RAISE_KEYBOARD_NUMERIC";
        const QString LOWER_KEYBOARD = "_LOWER_KEYBOARD";
        QString messageString = message.type() == QVariant::String ? message.toString() : "";
        if (messageString.left(RAISE_KEYBOARD.length()) == RAISE_KEYBOARD) {
            setKeyboardRaised(_currentFocusItem, true, messageString == RAISE_KEYBOARD_NUMERIC);
        } else if (messageString == LOWER_KEYBOARD) {
            setKeyboardRaised(_currentFocusItem, false);
        } else {
            emit webEventReceived(message);
        }
    }
}

void OffscreenQmlSurface::sendToQml(const QVariant& message) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "emitQmlEvent", Qt::QueuedConnection, Q_ARG(QVariant, message));
    } else if (_rootItem) {
        // call fromScript method on qml root
        QMetaObject::invokeMethod(_rootItem, "fromScript", Qt::QueuedConnection, Q_ARG(QVariant, message));
    }
}


#include "OffscreenQmlSurface.moc"
