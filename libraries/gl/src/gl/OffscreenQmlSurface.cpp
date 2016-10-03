//
//  Created by Bradley Austin Davis on 2015-05-13
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "OffscreenQmlSurface.h"
#include "Config.h"

#include <queue>
#include <set>
#include <map>

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
#include <PerfStat.h>
#include <DependencyManager.h>
#include <NumericalConstants.h>
#include <Finally.h>
#include <PathUtils.h>
#include <AbstractUriHandler.h>
#include <AccountManager.h>
#include <NetworkAccessManager.h>

#include "OffscreenGLCanvas.h"
#include "GLEscrow.h"
#include "GLHelpers.h"
#include "GLLogging.h"


QString fixupHifiUrl(const QString& urlString) {
	static const QString ACCESS_TOKEN_PARAMETER = "access_token";
	static const QString ALLOWED_HOST = "metaverse.highfidelity.com";
    QUrl url(urlString);
	QUrlQuery query(url);
	if (url.host() == ALLOWED_HOST && query.allQueryItemValues(ACCESS_TOKEN_PARAMETER).empty()) {
	    auto accountManager = DependencyManager::get<AccountManager>();
	    query.addQueryItem(ACCESS_TOKEN_PARAMETER, accountManager->getAccountInfo().getAccessToken().token);
	    url.setQuery(query.query());
	    return url.toString();
	}
    return urlString;
}

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

    // FIXME hack for authentication, remove when we migrate to Qt 5.6
    Q_INVOKABLE QString fixupUrl(const QString& originalUrl) {
        return fixupHifiUrl(originalUrl);
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

Q_DECLARE_LOGGING_CATEGORY(offscreenFocus)
Q_LOGGING_CATEGORY(offscreenFocus, "hifi.offscreen.focus")

static const QEvent::Type INIT = QEvent::Type(QEvent::User + 1);
static const QEvent::Type RENDER = QEvent::Type(QEvent::User + 2);
static const QEvent::Type RESIZE = QEvent::Type(QEvent::User + 3);
static const QEvent::Type STOP = QEvent::Type(QEvent::User + 4);

class RawTextureRecycler {
public:
    using TexturePtr = GLuint;
    RawTextureRecycler(bool useMipmaps) : _useMipmaps(useMipmaps) {}
    void setSize(const uvec2& size);
    void clear();
    TexturePtr getNextTexture();
    void recycleTexture(GLuint texture);

private:

    struct TexInfo {
        TexturePtr _tex { 0 };
        uvec2 _size;
        bool _active { false };

        TexInfo() {}
        TexInfo(TexturePtr tex, const uvec2& size) : _tex(tex), _size(size) {}
    };

    using Map = std::map<GLuint, TexInfo>;
    using Queue = std::queue<TexturePtr>;

    Map _allTextures;
    Queue _readyTextures;
    uvec2 _size { 1920, 1080 };
    bool _useMipmaps;
};


void RawTextureRecycler::setSize(const uvec2& size) {
    if (size == _size) {
        return;
    }
    _size = size;
    while (!_readyTextures.empty()) {
        _readyTextures.pop();
    }
    std::set<Map::key_type> toDelete;
    std::for_each(_allTextures.begin(), _allTextures.end(), [&](Map::const_reference item) {
        if (!item.second._active && item.second._size != _size) {
            toDelete.insert(item.first);
        }
    });
    std::for_each(toDelete.begin(), toDelete.end(), [&](Map::key_type key) {
        _allTextures.erase(key);
    });
}

void RawTextureRecycler::clear() {
    while (!_readyTextures.empty()) {
        _readyTextures.pop();
    }
    _allTextures.clear();
}

RawTextureRecycler::TexturePtr RawTextureRecycler::getNextTexture() {
    if (_readyTextures.empty()) {
        TexturePtr newTexture;
        glGenTextures(1, &newTexture);

        glBindTexture(GL_TEXTURE_2D, newTexture);
        if (_useMipmaps) {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        } else {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        }
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 8.0f);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, -0.2f);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 8.0f); 
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, _size.x, _size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
        _allTextures[newTexture] = TexInfo { newTexture, _size };
        _readyTextures.push(newTexture);
    }

    TexturePtr result = _readyTextures.front();
    _readyTextures.pop();
    auto& item = _allTextures[result];
    item._active = true;
    return result;
}

void RawTextureRecycler::recycleTexture(GLuint texture) {
    Q_ASSERT(_allTextures.count(texture));
    auto& item = _allTextures[texture];
    Q_ASSERT(item._active);
    item._active = false;
    if (item._size != _size) {
        // Buh-bye
        _allTextures.erase(texture);
        return;
    }

    _readyTextures.push(item._tex);
}


class OffscreenQmlRenderThread : public QThread {
public:
    OffscreenQmlRenderThread(OffscreenQmlSurface* surface, QOpenGLContext* shareContext);
    virtual ~OffscreenQmlRenderThread() = default;

    virtual void run() override;
    virtual bool event(QEvent *e) override;

protected:
    class Queue : private QQueue<QEvent*> {
    public:
        void add(QEvent::Type type);
        QEvent* take();

    private:
        QMutex _mutex;
        QWaitCondition _waitCondition;
        bool _isWaiting{ false };
    };

    friend class OffscreenQmlSurface;

    QJsonObject getGLContextData();

    Queue _queue;
    QMutex _mutex;
    QWaitCondition _waitCondition;
    std::atomic<bool> _rendering { false };

    QJsonObject _glData;
    QMutex _glMutex;
    QWaitCondition _glWait;

private:
    // Event-driven methods
    void init();
    void render();
    void resize();
    void cleanup();

    // Helper methods
    void setupFbo();
    bool allowNewFrame(uint8_t fps);

    // Rendering members
    OffscreenGLCanvas _canvas;
    OffscreenQmlSurface* _surface{ nullptr };
    QQuickWindow* _quickWindow{ nullptr };
    QMyQuickRenderControl* _renderControl{ nullptr };
    GLuint _fbo { 0 };
    GLuint _depthStencil { 0 };
    RawTextureRecycler _textures { true };
    GLTextureEscrow _escrow;

    uint64_t _lastRenderTime{ 0 };
    uvec2 _size{ 1920, 1080 };
    QSize _newSize;
    bool _quit{ false };
};

void OffscreenQmlRenderThread::Queue::add(QEvent::Type type) {
    QMutexLocker locker(&_mutex);
    enqueue(new QEvent(type));
    if (_isWaiting) {
        _waitCondition.wakeOne();
    }
}

QEvent* OffscreenQmlRenderThread::Queue::take() {
    QMutexLocker locker(&_mutex);
    while (isEmpty()) {
        _isWaiting = true;
        _waitCondition.wait(&_mutex);
        _isWaiting = false;
    }
    QEvent* e = dequeue();
    return e;
}

OffscreenQmlRenderThread::OffscreenQmlRenderThread(OffscreenQmlSurface* surface, QOpenGLContext* shareContext) : _surface(surface) {
    _canvas.setObjectName("OffscreenQmlRenderCanvas");
    qCDebug(glLogging) << "Building QML Renderer";
    if (!_canvas.create(shareContext)) {
        qWarning("Failed to create OffscreenGLCanvas");
        _quit = true;
        return;
    };

    _renderControl = new QMyQuickRenderControl();
    QQuickWindow::setDefaultAlphaBuffer(true);
    // Create a QQuickWindow that is associated with our render control.
    // This window never gets created or shown, meaning that it will never get an underlying native (platform) window.
    // NOTE: Must be created on the main thread so that OffscreenQmlSurface can send it events
    // NOTE: Must be created on the rendering thread or it will refuse to render,
    //       so we wait until after its ctor to move object/context to this thread.
    _quickWindow = new QQuickWindow(_renderControl);
    _quickWindow->setColor(QColor(255, 255, 255, 0));
    _quickWindow->setFlags(_quickWindow->flags() | static_cast<Qt::WindowFlags>(Qt::WA_TranslucentBackground));

    // We can prepare, but we must wait to start() the thread until after the ctor
    _renderControl->prepareThread(this);
    _canvas.getContextObject()->moveToThread(this);
    moveToThread(this);

    _queue.add(INIT);
}

void OffscreenQmlRenderThread::run() {
    qCDebug(glLogging) << "Starting QML Renderer thread";

    while (!_quit) {
        QEvent* e = _queue.take();
        event(e);
        delete e;
    }
}

bool OffscreenQmlRenderThread::event(QEvent *e) {
    switch (int(e->type())) {
    case INIT:
        init();
        return true;
    case RENDER:
        render();
        return true;
    case RESIZE:
        resize();
        return true;
    case STOP:
        cleanup();
        return true;
    default:
        return QObject::event(e);
    }
}

void OffscreenQmlRenderThread::setupFbo() {
    _textures.setSize(_size);
    if (_depthStencil) {
        glDeleteRenderbuffers(1, &_depthStencil);
        _depthStencil = 0;
    }
    glGenRenderbuffers(1, &_depthStencil);
    glBindRenderbuffer(GL_RENDERBUFFER, _depthStencil);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, _size.x, _size.y);

    if (_fbo) {
        glDeleteFramebuffers(1, &_fbo);
        _fbo = 0;
    }
    glGenFramebuffers(1, &_fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _fbo);
    glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _depthStencil);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

QJsonObject OffscreenQmlRenderThread::getGLContextData() {
    _glMutex.lock();
    if (_glData.isEmpty()) {
        _glWait.wait(&_glMutex);
    }
    _glMutex.unlock();
    return _glData;
}

void OffscreenQmlRenderThread::init() {
    qCDebug(glLogging) << "Initializing QML Renderer";

    if (!_canvas.makeCurrent()) {
        qWarning("Failed to make context current on QML Renderer Thread");
        _quit = true;
        return;
    }

    _glMutex.lock();
    _glData = ::getGLContextData();
    _glMutex.unlock();
    _glWait.wakeAll();

    connect(_renderControl, &QQuickRenderControl::renderRequested, _surface, &OffscreenQmlSurface::requestRender);
    connect(_renderControl, &QQuickRenderControl::sceneChanged, _surface, &OffscreenQmlSurface::requestUpdate);

    _renderControl->initialize(_canvas.getContext());
    setupFbo();
    _escrow.setRecycler([this](GLuint texture){
        _textures.recycleTexture(texture);
    });
}

void OffscreenQmlRenderThread::cleanup() {
    _renderControl->invalidate();

    if (_depthStencil) {
        glDeleteRenderbuffers(1, &_depthStencil);
        _depthStencil = 0;
    }
    if (_fbo) {
        glDeleteFramebuffers(1, &_fbo);
        _fbo = 0;
    }

    _textures.clear();

    _canvas.doneCurrent();
    _canvas.getContextObject()->moveToThread(QCoreApplication::instance()->thread());

    _quit = true;
}

void OffscreenQmlRenderThread::resize() {
    // Lock _newSize changes
    {
        QMutexLocker locker(&_mutex);

        // Update our members
        if (_quickWindow) {
            _quickWindow->setGeometry(QRect(QPoint(), _newSize));
            _quickWindow->contentItem()->setSize(_newSize);
        }

        // Qt bug in 5.4 forces this check of pixel ratio,
        // even though we're rendering offscreen.
        qreal pixelRatio = 1.0;
        if (_renderControl && _renderControl->_renderWindow) {
            pixelRatio = _renderControl->_renderWindow->devicePixelRatio();
        }

        uvec2 newOffscreenSize = toGlm(_newSize * pixelRatio);
        if (newOffscreenSize == _size) {
            return;
        }

        qCDebug(glLogging) << "Offscreen UI resizing to " << _newSize.width() << "x" << _newSize.height() << " with pixel ratio " << pixelRatio;
        _size = newOffscreenSize;
    }

    _textures.setSize(_size);
    setupFbo();
}

void OffscreenQmlRenderThread::render() {
    // Ensure we always release the main thread
    Finally releaseMainThread([this] {
        _waitCondition.wakeOne();
    });

    if (_surface->_paused) {
        return;
    }

    _rendering = true;
    Finally unmarkRenderingFlag([this] {
        _rendering = false;
    });

    {
        QMutexLocker locker(&_mutex);
        _renderControl->sync();
        releaseMainThread.trigger();
    }

    _quickWindow->setRenderTarget(_fbo, QSize(_size.x, _size.y));

    try {
        GLuint texture = _textures.getNextTexture();
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _fbo);
        glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture, 0);
            PROFILE_RANGE("qml_render->rendercontrol")
            _renderControl->render();

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glGenerateMipmap(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);

        _quickWindow->resetOpenGLState();
        _escrow.submit(texture);
        _lastRenderTime = usecTimestampNow();
    } catch (std::runtime_error& error) {
        qWarning() << "Failed to render QML: " << error.what();
    }
}

bool OffscreenQmlRenderThread::allowNewFrame(uint8_t fps) {
    auto minRenderInterval = USECS_PER_SECOND / fps;
    auto lastInterval = usecTimestampNow() - _lastRenderTime;
    return (lastInterval > minRenderInterval);
}

OffscreenQmlSurface::OffscreenQmlSurface() {
    // moveToThread(qApp->thread());
}

static const uint64_t MAX_SHUTDOWN_WAIT_SECS = 2;
OffscreenQmlSurface::~OffscreenQmlSurface() {
    QObject::disconnect(&_updateTimer);
    QObject::disconnect(qApp);

    qCDebug(glLogging) << "Stopping QML Renderer Thread " << _renderer->currentThreadId();
    _renderer->_queue.add(STOP);
    if (!_renderer->wait(MAX_SHUTDOWN_WAIT_SECS * USECS_PER_SECOND)) {
        qWarning() << "Failed to shut down the QML Renderer Thread";
    }

    delete _rootItem;
    delete _renderer;
    delete _qmlComponent;
    delete _qmlEngine;
}

void OffscreenQmlSurface::onAboutToQuit() {
    QObject::disconnect(&_updateTimer);
}

void OffscreenQmlSurface::create(QOpenGLContext* shareContext) {
    qCDebug(glLogging) << "Building QML surface";

    _renderer = new OffscreenQmlRenderThread(this, shareContext);
    _renderer->moveToThread(_renderer);
    _renderer->setObjectName("QML Renderer Thread");
    _renderer->start();

    _renderer->_renderControl->_renderWindow = _proxyWindow;

    connect(_renderer->_quickWindow, &QQuickWindow::focusObjectChanged, this, &OffscreenQmlSurface::onFocusObjectChanged);

    // Create a QML engine.
    _qmlEngine = new QQmlEngine;

    _qmlEngine->setNetworkAccessManagerFactory(new QmlNetworkAccessManagerFactory);

    auto importList = _qmlEngine->importPathList();
    importList.insert(importList.begin(), PathUtils::resourcesPath());
    _qmlEngine->setImportPathList(importList);
    if (!_qmlEngine->incubationController()) {
        _qmlEngine->setIncubationController(_renderer->_quickWindow->incubationController());
    }

    _qmlEngine->rootContext()->setContextProperty("GL", _renderer->getGLContextData());
    _qmlEngine->rootContext()->setContextProperty("offscreenWindow", QVariant::fromValue(getWindow()));
    _qmlComponent = new QQmlComponent(_qmlEngine);

    // When Quick says there is a need to render, we will not render immediately. Instead,
    // a timer with a small interval is used to get better performance.
    QObject::connect(&_updateTimer, &QTimer::timeout, this, &OffscreenQmlSurface::updateQuick);
    QObject::connect(qApp, &QCoreApplication::aboutToQuit, this, &OffscreenQmlSurface::onAboutToQuit);
    _updateTimer.setInterval(MIN_TIMER_MS);
    _updateTimer.start();

    auto rootContext = getRootContext();
    rootContext->setContextProperty("urlHandler", new UrlHandler());
    rootContext->setContextProperty("resourceDirectoryUrl", QUrl::fromLocalFile(PathUtils::resourcesPath()));
}

void OffscreenQmlSurface::resize(const QSize& newSize_, bool forceResize) {

    if (!_renderer || !_renderer->_quickWindow) {
        return;
    }

    const float MAX_OFFSCREEN_DIMENSION = 4096;
    QSize newSize = newSize_;

    if (newSize.width() > MAX_OFFSCREEN_DIMENSION || newSize.height() > MAX_OFFSCREEN_DIMENSION) {
        float scale = std::min(
                ((float)newSize.width() / MAX_OFFSCREEN_DIMENSION),
                ((float)newSize.height() / MAX_OFFSCREEN_DIMENSION));
        newSize = QSize(
                std::max(static_cast<int>(scale * newSize.width()), 10),
                std::max(static_cast<int>(scale * newSize.height()), 10));
    }

    QSize currentSize = _renderer->_quickWindow->geometry().size();
    if (newSize == currentSize && !forceResize) {
        return;
    }

    _qmlEngine->rootContext()->setContextProperty("surfaceSize", newSize);

    if (_rootItem) {
        _rootItem->setSize(newSize);
    }

    {
        QMutexLocker locker(&(_renderer->_mutex));
        _renderer->_newSize = newSize;
    }

    _renderer->_queue.add(RESIZE);
}

QQuickItem* OffscreenQmlSurface::getRootItem() {
    return _rootItem;
}

void OffscreenQmlSurface::setBaseUrl(const QUrl& baseUrl) {
    _qmlEngine->setBaseUrl(baseUrl);
}

QObject* OffscreenQmlSurface::load(const QUrl& qmlSource, std::function<void(QQmlContext*, QObject*)> f) {
    // Synchronous loading may take a while; restart the deadlock timer
    QMetaObject::invokeMethod(qApp, "updateHeartbeat", Qt::DirectConnection);

    _qmlComponent->loadUrl(qmlSource, QQmlComponent::PreferSynchronous);

    if (_qmlComponent->isLoading()) {
        connect(_qmlComponent, &QQmlComponent::statusChanged, this,
            [this, f](QQmlComponent::Status){
                finishQmlLoad(f);
            });
        return nullptr;
    }

    return finishQmlLoad(f);
}

void OffscreenQmlSurface::clearCache() {
    getRootContext()->engine()->clearComponentCache();
}

void OffscreenQmlSurface::requestUpdate() {
    _polish = true;
    _render = true;
}

void OffscreenQmlSurface::requestRender() {
    _render = true;
}

QObject* OffscreenQmlSurface::finishQmlLoad(std::function<void(QQmlContext*, QObject*)> f) {
    disconnect(_qmlComponent, &QQmlComponent::statusChanged, this, 0);
    if (_qmlComponent->isError()) {
        QList<QQmlError> errorList = _qmlComponent->errors();
        foreach(const QQmlError& error, errorList) {
            qWarning() << error.url() << error.line() << error;
        }
        return nullptr;
    }

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
        qWarning() << "Unable to find qwebchannel.js or createGlobalEventBridge.js";
    }

    QQmlContext* newContext = new QQmlContext(_qmlEngine, qApp);
    QObject* newObject = _qmlComponent->beginCreate(newContext);
    if (_qmlComponent->isError()) {
        QList<QQmlError> errorList = _qmlComponent->errors();
        foreach(const QQmlError& error, errorList)
            qWarning() << error.url() << error.line() << error;
        if (!_rootItem) {
            qFatal("Unable to finish loading QML root");
        }
        return nullptr;
    }

    newObject->setProperty("eventBridge", QVariant::fromValue(this));
    newContext->setContextProperty("eventBridgeJavaScriptToInject", QVariant(javaScriptToInject));

    f(newContext, newObject);
    _qmlComponent->completeCreate();


    // All quick items should be focusable
    QQuickItem* newItem = qobject_cast<QQuickItem*>(newObject);
    if (newItem) {
        // Make sure we make items focusable (critical for
        // supporting keyboard shortcuts)
        newItem->setFlag(QQuickItem::ItemIsFocusScope, true);
    }

    // If we already have a root, just set a couple of flags and the ancestry
    if (_rootItem) {
        // Allow child windows to be destroyed from JS
        QQmlEngine::setObjectOwnership(newObject, QQmlEngine::JavaScriptOwnership);
        newObject->setParent(_rootItem);
        if (newItem) {
            newItem->setParentItem(_rootItem);
        }
        return newObject;
    }

    if (!newItem) {
        qFatal("Could not load object as root item");
        return nullptr;
    }
    // The root item is ready. Associate it with the window.
    _rootItem = newItem;
    _rootItem->setParentItem(_renderer->_quickWindow->contentItem());
    _rootItem->setSize(_renderer->_quickWindow->renderTargetSize());
    return _rootItem;
}

void OffscreenQmlSurface::updateQuick() {
    // If we're
    //   a) not set up
    //   b) already rendering a frame
    //   c) rendering too fast
    // then skip this
    if (!_renderer || _renderer->_rendering || !_renderer->allowNewFrame(_maxFps)) {
        return;
    }

    if (_polish) {
        _renderer->_renderControl->polishItems();
        _polish = false;
    }

    if (_render) {
        PROFILE_RANGE(__FUNCTION__);
        // Lock the GUI size while syncing
        QMutexLocker locker(&(_renderer->_mutex));
        _renderer->_queue.add(RENDER);
        _renderer->_waitCondition.wait(&(_renderer->_mutex));
        _render = false;
    }

    if (_renderer->_escrow.fetchSignaledAndRelease(_currentTexture)) {
        emit textureUpdated(_currentTexture);
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
    offscreenPosition *= vec2(toGlm(_renderer->_quickWindow->size()));
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
    if (_renderer->_quickWindow == originalDestination) {
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
        Q_ASSERT(recurseTest != _rootItem && recurseTest != _renderer->_quickWindow);
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
            if (QCoreApplication::sendEvent(_renderer->_quickWindow, event)) {
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
            if (QCoreApplication::sendEvent(_renderer->_quickWindow, &mappedEvent)) {
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
                _qmlEngine->rootContext()->setContextProperty("lastMousePosition", transformedPos);
            }
            mappedEvent.ignore();
            if (QCoreApplication::sendEvent(_renderer->_quickWindow, &mappedEvent)) {
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
    requestRender();

    getRootItem()->setProperty("eventBridge", QVariant::fromValue(this));
    getRootContext()->setContextProperty("webEntity", this);
}

bool OffscreenQmlSurface::isPaused() const {
    return _paused;
}

void OffscreenQmlSurface::setProxyWindow(QWindow* window) {
    _proxyWindow = window;
    if (_renderer && _renderer->_renderControl) {
        _renderer->_renderControl->_renderWindow = window;
    }
}

QObject* OffscreenQmlSurface::getEventHandler() {
    return getWindow();
}

QQuickWindow* OffscreenQmlSurface::getWindow() {
    return _renderer->_quickWindow;
}

QSize OffscreenQmlSurface::size() const {
    return _renderer->_quickWindow->geometry().size();
}

QQmlContext* OffscreenQmlSurface::getRootContext() {
    return _qmlEngine->rootContext();
}

Q_DECLARE_METATYPE(std::function<void()>);
auto VoidLambdaType = qRegisterMetaType<std::function<void()>>();
Q_DECLARE_METATYPE(std::function<QVariant()>);
auto VariantLambdaType = qRegisterMetaType<std::function<QVariant()>>();


void OffscreenQmlSurface::executeOnUiThread(std::function<void()> function, bool blocking ) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "executeOnUiThread", blocking ? Qt::BlockingQueuedConnection : Qt::QueuedConnection,
            Q_ARG(std::function<void()>, function));
        return;
    }

    function();
}

QVariant OffscreenQmlSurface::returnFromUiThread(std::function<QVariant()> function) {
    if (QThread::currentThread() != thread()) {
        QVariant result;
        QMetaObject::invokeMethod(this, "returnFromUiThread", Qt::BlockingQueuedConnection,
            Q_RETURN_ARG(QVariant, result),
            Q_ARG(std::function<QVariant()>, function));
        return result;
    }

    return function();
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
static const uint8_t UPWARDS_WHITE_ARROW_FROM_BAR[] = { 0xE2, 0x87, 0xAA, 0x00 }; // shift
static const uint8_t LEFT_ARROW[] = { 0xE2, 0x86, 0x90, 0x00 }; // backspace
static const uint8_t LEFTWARD_WHITE_ARROW[] = { 0xE2, 0x87, 0xA6, 0x00 }; // left arrow
static const uint8_t RIGHTWARD_WHITE_ARROW[] = { 0xE2, 0x87, 0xA8, 0x00 }; // right arrow
static const uint8_t ASTERISIM[] = { 0xE2, 0x81, 0x82, 0x00 }; // symbols
static const uint8_t RETURN_SYMBOL[] = { 0xE2, 0x8F, 0x8E, 0x00 }; // return
static const char PUNCTUATION_STRING[] = "&123";
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
    auto utf8Key = key.toUtf8();

    int scanCode = (int)utf8Key[0];
    QString keyString = key;
    if (equals(utf8Key, UPWARDS_WHITE_ARROW_FROM_BAR) || equals(utf8Key, ASTERISIM) ||
        equals(utf8Key, (uint8_t*)PUNCTUATION_STRING) || equals(utf8Key, (uint8_t*)ALPHABET_STRING)) {
        return;  // ignore
    } else if (equals(utf8Key, LEFT_ARROW)) {
        scanCode = Qt::Key_Backspace;
        keyString = "\x08";
    } else if (equals(utf8Key, RETURN_SYMBOL)) {
        scanCode = Qt::Key_Return;
        keyString = "\x0d";
    } else if (equals(utf8Key, LEFTWARD_WHITE_ARROW)) {
        scanCode = Qt::Key_Left;
        keyString = "";
    } else if (equals(utf8Key, RIGHTWARD_WHITE_ARROW)) {
        scanCode = Qt::Key_Right;
        keyString = "";
    }

    QKeyEvent* pressEvent = new QKeyEvent(QEvent::KeyPress, scanCode, Qt::NoModifier, keyString);
    QKeyEvent* releaseEvent = new QKeyEvent(QEvent::KeyRelease, scanCode, Qt::NoModifier, keyString);
    QCoreApplication::postEvent(getEventHandler(), pressEvent);
    QCoreApplication::postEvent(getEventHandler(), releaseEvent);
}

void OffscreenQmlSurface::setKeyboardRaised(QObject* object, bool raised, bool numeric) {
    if (!object) {
        return;
    }

    QQuickItem* item = dynamic_cast<QQuickItem*>(object);
    while (item) {
        // Numeric value may be set in parameter from HTML UI; for QML UI, detect numeric fields here.
        numeric = numeric || QString(item->metaObject()->className()).left(7) == "SpinBox";

        if (item->property("keyboardRaised").isValid()) {
            if (item->property("punctuationMode").isValid()) {
                item->setProperty("punctuationMode", QVariant(numeric));
            }
            item->setProperty("keyboardRaised", QVariant(raised));
            return;
        }
        item = dynamic_cast<QQuickItem*>(item->parentItem());
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

#include "OffscreenQmlSurface.moc"
