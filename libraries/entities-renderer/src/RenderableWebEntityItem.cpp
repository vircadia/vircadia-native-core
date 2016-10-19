//
//  Created by Bradley Austin Davis on 2015/05/12
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "RenderableWebEntityItem.h"

#include <QMouseEvent>
#include <QQuickItem>
#include <QQuickWindow>
#include <QQmlContext>
#include <QOpenGLContext>

#include <glm/gtx/quaternion.hpp>

#include <GeometryCache.h>
#include <PerfStat.h>
#include <gl/OffscreenQmlSurface.h>
#include <AbstractViewStateInterface.h>
#include <GLMHelpers.h>
#include <PathUtils.h>
#include <TextureCache.h>
#include <gpu/Context.h>

#include "EntityTreeRenderer.h"
#include "EntitiesRendererLogging.h"

const float METERS_TO_INCHES = 39.3701f;
static uint32_t _currentWebCount { 0 };
// Don't allow more than 100 concurrent web views
static const uint32_t MAX_CONCURRENT_WEB_VIEWS = 100;
// If a web-view hasn't been rendered for 30 seconds, de-allocate the framebuffer
static uint64_t MAX_NO_RENDER_INTERVAL = 30 * USECS_PER_SECOND;

static int MAX_WINDOW_SIZE = 4096;
static float OPAQUE_ALPHA_THRESHOLD = 0.99f;

EntityItemPointer RenderableWebEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    EntityItemPointer entity{ new RenderableWebEntityItem(entityID) };
    entity->setProperties(properties);
    return entity;
}

RenderableWebEntityItem::RenderableWebEntityItem(const EntityItemID& entityItemID) :
    WebEntityItem(entityItemID) {
    qDebug() << "Created web entity " << getID();

    _touchDevice.setCapabilities(QTouchDevice::Position);
    _touchDevice.setType(QTouchDevice::TouchScreen);
    _touchDevice.setName("RenderableWebEntityItemTouchDevice");
    _touchDevice.setMaximumTouchPoints(4);
    _geometryId = DependencyManager::get<GeometryCache>()->allocateID();
}

RenderableWebEntityItem::~RenderableWebEntityItem() {
    destroyWebSurface();
    qDebug() << "Destroyed web entity " << getID();
    auto geometryCache = DependencyManager::get<GeometryCache>();
    if (geometryCache) {
        geometryCache->releaseID(_geometryId);
    }
}

bool RenderableWebEntityItem::buildWebSurface(EntityTreeRenderer* renderer) {
    if (_currentWebCount >= MAX_CONCURRENT_WEB_VIEWS) {
        qWarning() << "Too many concurrent web views to create new view";
        return false;
    }
    qDebug() << "Building web surface";

    QString javaScriptToInject;
    QFile webChannelFile(":qtwebchannel/qwebchannel.js");
    QFile createGlobalEventBridgeFile(PathUtils::resourcesPath() + "/html/createGlobalEventBridge.js");
    if (webChannelFile.open(QFile::ReadOnly | QFile::Text) &&
        createGlobalEventBridgeFile.open(QFile::ReadOnly | QFile::Text)) {
        QString webChannelStr = QTextStream(&webChannelFile).readAll();
        QString createGlobalEventBridgeStr = QTextStream(&createGlobalEventBridgeFile).readAll();

        // concatenate these js files
        javaScriptToInject = webChannelStr + createGlobalEventBridgeStr;
    } else {
        qCWarning(entitiesrenderer) << "unable to find qwebchannel.js or createGlobalEventBridge.js";
    }

    ++_currentWebCount;
    // Save the original GL context, because creating a QML surface will create a new context
    QOpenGLContext * currentContext = QOpenGLContext::currentContext();
    if (!currentContext) {
        return false;
    }
    QSurface * currentSurface = currentContext->surface();

    auto deleter = [](OffscreenQmlSurface* webSurface) {
        AbstractViewStateInterface::instance()->postLambdaEvent([webSurface] {
            webSurface->deleteLater();
        });
    };
    _webSurface = QSharedPointer<OffscreenQmlSurface>(new OffscreenQmlSurface(), deleter);

    // The lifetime of the QML surface MUST be managed by the main thread
    // Additionally, we MUST use local variables copied by value, rather than
    // member variables, since they would implicitly refer to a this that 
    // is no longer valid

    _webSurface->create(currentContext);
    _webSurface->setBaseUrl(QUrl::fromLocalFile(PathUtils::resourcesPath() + "/qml/controls/"));
    _webSurface->load("WebView.qml", [&](QQmlContext* context, QObject* obj) {
        context->setContextProperty("eventBridgeJavaScriptToInject", QVariant(javaScriptToInject));
    });
    _webSurface->resume();
    _webSurface->getRootItem()->setProperty("url", _sourceUrl);
    _webSurface->getRootContext()->setContextProperty("desktop", QVariant());
    // FIXME - Keyboard HMD only: Possibly add "HMDinfo" object to context for WebView.qml.

    // forward web events to EntityScriptingInterface
    auto entities = DependencyManager::get<EntityScriptingInterface>();
    const EntityItemID entityItemID = getID();
    QObject::connect(_webSurface.data(), &OffscreenQmlSurface::webEventReceived, [=](const QVariant& message) {
        emit entities->webEventReceived(entityItemID, message);
    });

    // Restore the original GL context
    currentContext->makeCurrent(currentSurface);

    auto forwardPointerEvent = [=](const EntityItemID& entityItemID, const PointerEvent& event) {
        if (entityItemID == getID()) {
            handlePointerEvent(event);
        }
    };
    _mousePressConnection = QObject::connect(renderer, &EntityTreeRenderer::mousePressOnEntity, forwardPointerEvent);
    _mouseReleaseConnection = QObject::connect(renderer, &EntityTreeRenderer::mouseReleaseOnEntity, forwardPointerEvent);
    _mouseMoveConnection = QObject::connect(renderer, &EntityTreeRenderer::mouseMoveOnEntity, forwardPointerEvent);
    _hoverLeaveConnection = QObject::connect(renderer, &EntityTreeRenderer::hoverLeaveEntity, [=](const EntityItemID& entityItemID, const PointerEvent& event) {
        if (this->_pressed && this->getID() == entityItemID) {
            // If the user mouses off the entity while the button is down, simulate a touch end.
            QTouchEvent::TouchPoint point;
            point.setId(event.getID());
            point.setState(Qt::TouchPointReleased);
            glm::vec2 windowPos = event.getPos2D() * (METERS_TO_INCHES * _dpi);
            QPointF windowPoint(windowPos.x, windowPos.y);
            point.setScenePos(windowPoint);
            point.setPos(windowPoint);
            QList<QTouchEvent::TouchPoint> touchPoints;
            touchPoints.push_back(point);
            QTouchEvent* touchEvent = new QTouchEvent(QEvent::TouchEnd, nullptr, Qt::NoModifier, Qt::TouchPointReleased, touchPoints);
            touchEvent->setWindow(_webSurface->getWindow());
            touchEvent->setDevice(&_touchDevice);
            touchEvent->setTarget(_webSurface->getRootItem());
            QCoreApplication::postEvent(_webSurface->getWindow(), touchEvent);
        }
    });
    return true;
}

glm::vec2 RenderableWebEntityItem::getWindowSize() const {
    glm::vec2 dims = glm::vec2(getDimensions());
    dims *= METERS_TO_INCHES * _dpi;

    // ensure no side is never larger then MAX_WINDOW_SIZE
    float max = (dims.x > dims.y) ? dims.x : dims.y;
    if (max > MAX_WINDOW_SIZE) {
        dims *= MAX_WINDOW_SIZE / max;
    }

    return dims;
}

void RenderableWebEntityItem::render(RenderArgs* args) {
    checkFading();

    #ifdef WANT_EXTRA_DEBUGGING
    {
        gpu::Batch& batch = *args->_batch;
        batch.setModelTransform(getTransformToCenter()); // we want to include the scale as well
        glm::vec4 cubeColor{ 1.0f, 0.0f, 0.0f, 1.0f};
        DependencyManager::get<GeometryCache>()->renderWireCube(batch, 1.0f, cubeColor);
    }
    #endif

    if (!_webSurface) {
        if (!buildWebSurface(static_cast<EntityTreeRenderer*>(args->_renderer))) {
            return;
        }
        _fadeStartTime = usecTimestampNow();
    }

    _lastRenderTime = usecTimestampNow();

    glm::vec2 windowSize = getWindowSize();

    // The offscreen surface is idempotent for resizes (bails early
    // if it's a no-op), so it's safe to just call resize every frame
    // without worrying about excessive overhead.
    _webSurface->resize(QSize(windowSize.x, windowSize.y));

    if (!_texture) {
        auto webSurface = _webSurface;
        auto recycler = [webSurface] (uint32_t recycleTexture, void* recycleFence) {
            webSurface->releaseTexture({ recycleTexture, recycleFence });
        };
        _texture = gpu::TexturePointer(gpu::Texture::createExternal2D(recycler));
        _texture->setSource(__FUNCTION__);
    }
    OffscreenQmlSurface::TextureAndFence newTextureAndFence;
    bool newTextureAvailable = _webSurface->fetchTexture(newTextureAndFence);
    if (newTextureAvailable) {
        _texture->setExternalTexture(newTextureAndFence.first, newTextureAndFence.second);
    }

    PerformanceTimer perfTimer("RenderableWebEntityItem::render");
    Q_ASSERT(getType() == EntityTypes::Web);
    static const glm::vec2 texMin(0.0f), texMax(1.0f), topLeft(-0.5f), bottomRight(0.5f);

    Q_ASSERT(args->_batch);
    gpu::Batch& batch = *args->_batch;

    bool success;
    batch.setModelTransform(getTransformToCenter(success));
    if (!success) {
        return;
    }
    batch.setResourceTexture(0, _texture);

    float fadeRatio = _isFading ? Interpolate::calculateFadeRatio(_fadeStartTime) : 1.0f;

    batch._glColor4f(1.0f, 1.0f, 1.0f, fadeRatio);

    if (fadeRatio < OPAQUE_ALPHA_THRESHOLD) {
        DependencyManager::get<GeometryCache>()->bindTransparentWebBrowserProgram(batch);
    } else {
        DependencyManager::get<GeometryCache>()->bindOpaqueWebBrowserProgram(batch);
    }
    DependencyManager::get<GeometryCache>()->renderQuad(batch, topLeft, bottomRight, texMin, texMax, glm::vec4(1.0f, 1.0f, 1.0f, fadeRatio), _geometryId);
}

void RenderableWebEntityItem::setSourceUrl(const QString& value) {
    if (_sourceUrl != value) {
        qDebug() << "Setting web entity source URL to " << value;
        _sourceUrl = value;
        if (_webSurface) {
            AbstractViewStateInterface::instance()->postLambdaEvent([this] {
                _webSurface->getRootItem()->setProperty("url", _sourceUrl);
            });
        }
    }
}

void RenderableWebEntityItem::setProxyWindow(QWindow* proxyWindow) {
    if (_webSurface) {
        _webSurface->setProxyWindow(proxyWindow);
    }
}

QObject* RenderableWebEntityItem::getEventHandler() {
    if (!_webSurface) {
        return nullptr;
    }
    return _webSurface->getEventHandler();
}

void RenderableWebEntityItem::handlePointerEvent(const PointerEvent& event) {

    // Ignore mouse interaction if we're locked
    if (getLocked() || !_webSurface) {
        return;
    }

    glm::vec2 windowPos = event.getPos2D() * (METERS_TO_INCHES * _dpi);
    QPointF windowPoint(windowPos.x, windowPos.y);
    if (event.getType() == PointerEvent::Move) {
        // Forward a mouse move event to webSurface
        QMouseEvent* mouseEvent = new QMouseEvent(QEvent::MouseMove, windowPoint, windowPoint, windowPoint, Qt::NoButton, Qt::NoButton, Qt::NoModifier);
        QCoreApplication::postEvent(_webSurface->getWindow(), mouseEvent);
    }

    {
        // Forward a touch update event to webSurface
        if (event.getType() == PointerEvent::Press) {
            this->_pressed = true;
        } else if (event.getType() == PointerEvent::Release) {
            this->_pressed = false;
        }

        QEvent::Type type;
        Qt::TouchPointState touchPointState;
        switch (event.getType()) {
        case PointerEvent::Press:
            type = QEvent::TouchBegin;
            touchPointState = Qt::TouchPointPressed;
            break;
        case PointerEvent::Release:
            type = QEvent::TouchEnd;
            touchPointState = Qt::TouchPointReleased;
            break;
        case PointerEvent::Move:
        default:
            type = QEvent::TouchUpdate;
            touchPointState = Qt::TouchPointMoved;
            break;
        }

        QTouchEvent::TouchPoint point;
        point.setId(event.getID());
        point.setState(touchPointState);
        point.setPos(windowPoint);
        point.setScreenPos(windowPoint);
        QList<QTouchEvent::TouchPoint> touchPoints;
        touchPoints.push_back(point);

        QTouchEvent* touchEvent = new QTouchEvent(type);
        touchEvent->setWindow(_webSurface->getWindow());
        touchEvent->setDevice(&_touchDevice);
        touchEvent->setTarget(_webSurface->getRootItem());
        touchEvent->setTouchPoints(touchPoints);
        touchEvent->setTouchPointStates(touchPointState);

        _lastTouchEvent = *touchEvent;

        QCoreApplication::postEvent(_webSurface->getWindow(), touchEvent);
    }
}

void RenderableWebEntityItem::destroyWebSurface() {
    if (_webSurface) {
        --_currentWebCount;
        _webSurface->pause();
        _webSurface->disconnect(_connection);
        QObject::disconnect(_mousePressConnection);
        _mousePressConnection = QMetaObject::Connection();
        QObject::disconnect(_mouseReleaseConnection);
        _mouseReleaseConnection = QMetaObject::Connection();
        QObject::disconnect(_mouseMoveConnection);
        _mouseMoveConnection = QMetaObject::Connection();
        QObject::disconnect(_hoverLeaveConnection);
        _hoverLeaveConnection = QMetaObject::Connection();
        _webSurface.reset();
    }
}

void RenderableWebEntityItem::update(const quint64& now) {
    auto interval = now - _lastRenderTime;
    if (interval > MAX_NO_RENDER_INTERVAL) {
        destroyWebSurface();
    }
}

bool RenderableWebEntityItem::isTransparent() {
    float fadeRatio = _isFading ? Interpolate::calculateFadeRatio(_fadeStartTime) : 1.0f;
    return fadeRatio < OPAQUE_ALPHA_THRESHOLD;
}

void RenderableWebEntityItem::emitScriptEvent(const QVariant& message) {
    if (_webSurface) {
        _webSurface->emitScriptEvent(message);
    }
}
