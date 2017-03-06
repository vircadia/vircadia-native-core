//
//  Web3DOverlay.cpp
//
//  Created by Clement on 7/1/14.
//  Modified and renamed by Zander Otavka on 8/4/15
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Web3DOverlay.h"

#include <Application.h>

#include <QQuickWindow>
#include <QtGui/QOpenGLContext>
#include <QtQuick/QQuickItem>
#include <QtQml/QQmlContext>

#include <AbstractViewStateInterface.h>
#include <gpu/Batch.h>
#include <DependencyManager.h>
#include <GeometryCache.h>
#include <GeometryUtil.h>
#include <scripting/HMDScriptingInterface.h>
#include <gl/OffscreenQmlSurface.h>
#include <PathUtils.h>
#include <RegisteredMetaTypes.h>
#include <TabletScriptingInterface.h>
#include <TextureCache.h>
#include <UsersScriptingInterface.h>
#include <UserActivityLoggerScriptingInterface.h>
#include <AbstractViewStateInterface.h>
#include <gl/OffscreenQmlSurface.h>
#include <gl/OffscreenQmlSurfaceCache.h>
#include <AddressManager.h>
#include "scripting/AccountScriptingInterface.h"
#include "scripting/HMDScriptingInterface.h"
#include "scripting/AssetMappingsScriptingInterface.h"
#include <Preferences.h>
#include <ScriptEngines.h>
#include "FileDialogHelper.h"
#include "avatar/AvatarManager.h"


static const float DPI = 30.47f;
static const float INCHES_TO_METERS = 1.0f / 39.3701f;
static const float METERS_TO_INCHES = 39.3701f;
static const float OPAQUE_ALPHA_THRESHOLD = 0.99f;

const QString Web3DOverlay::TYPE = "web3d";
const QString Web3DOverlay::QML = "Web3DOverlay.qml";
Web3DOverlay::Web3DOverlay() : _dpi(DPI) {
    _touchDevice.setCapabilities(QTouchDevice::Position);
    _touchDevice.setType(QTouchDevice::TouchScreen);
    _touchDevice.setName("RenderableWebEntityItemTouchDevice");
    _touchDevice.setMaximumTouchPoints(4);

    _geometryId = DependencyManager::get<GeometryCache>()->allocateID();
}

Web3DOverlay::Web3DOverlay(const Web3DOverlay* Web3DOverlay) :
    Billboard3DOverlay(Web3DOverlay),
    _url(Web3DOverlay->_url),
    _scriptURL(Web3DOverlay->_scriptURL),
    _dpi(Web3DOverlay->_dpi),
    _resolution(Web3DOverlay->_resolution),
    _showKeyboardFocusHighlight(Web3DOverlay->_showKeyboardFocusHighlight)
{
    _geometryId = DependencyManager::get<GeometryCache>()->allocateID();
}

Web3DOverlay::~Web3DOverlay() {
    if (_webSurface) {
        QQuickItem* rootItem = _webSurface->getRootItem();

        if (rootItem && rootItem->objectName() == "tabletRoot") {
            auto tabletScriptingInterface = DependencyManager::get<TabletScriptingInterface>();
            tabletScriptingInterface->setQmlTabletRoot("com.highfidelity.interface.tablet.system", nullptr, nullptr);
        }

        // Fix for crash in QtWebEngineCore when rapidly switching domains
        // Call stop on the QWebEngineView before destroying OffscreenQMLSurface.
        if (rootItem) {
            QObject* obj = rootItem->findChild<QObject*>("webEngineView");
            if (obj) {
                // stop loading
                QMetaObject::invokeMethod(obj, "stop");
            }
        }

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

        QObject::disconnect(_emitScriptEventConnection);
        _emitScriptEventConnection = QMetaObject::Connection();
        QObject::disconnect(_webEventReceivedConnection);
        _webEventReceivedConnection = QMetaObject::Connection();

        // The lifetime of the QML surface MUST be managed by the main thread
        // Additionally, we MUST use local variables copied by value, rather than
        // member variables, since they would implicitly refer to a this that
        // is no longer valid
        auto webSurface = _webSurface;
        AbstractViewStateInterface::instance()->postLambdaEvent([webSurface] {
            DependencyManager::get<OffscreenQmlSurfaceCache>()->release(QML, webSurface);
        });
        _webSurface.reset();
    }
    auto geometryCache = DependencyManager::get<GeometryCache>();
    if (geometryCache) {
        geometryCache->releaseID(_geometryId);
    }
}

void Web3DOverlay::update(float deltatime) {
    if (_webSurface) {
        // update globalPosition
        _webSurface->getRootContext()->setContextProperty("globalPosition", vec3toVariant(getPosition()));
    }
}

QString Web3DOverlay::pickURL() {
    QUrl sourceUrl(_url);
    if (sourceUrl.scheme() == "http" || sourceUrl.scheme() == "https" ||
        _url.toLower().endsWith(".htm") || _url.toLower().endsWith(".html")) {

        _webSurface->setBaseUrl(QUrl::fromLocalFile(PathUtils::resourcesPath() + "/qml/"));
        return "Web3DOverlay.qml";
    } else {
        return QUrl::fromLocalFile(PathUtils::resourcesPath()).toString() + "/" + _url;
    }
}


void Web3DOverlay::loadSourceURL() {

    QUrl sourceUrl(_url);
    if (sourceUrl.scheme() == "http" || sourceUrl.scheme() == "https" ||
        _url.toLower().endsWith(".htm") || _url.toLower().endsWith(".html")) {

        _webSurface->setBaseUrl(QUrl::fromLocalFile(PathUtils::resourcesPath() + "/qml/"));
        _webSurface->load("Web3DOverlay.qml");
        _webSurface->resume();
        _webSurface->getRootItem()->setProperty("url", _url);
        _webSurface->getRootItem()->setProperty("scriptURL", _scriptURL);
        _webSurface->getRootContext()->setContextProperty("ApplicationInterface", qApp);

    } else {
        _webSurface->setBaseUrl(QUrl::fromLocalFile(PathUtils::resourcesPath()));
        _webSurface->load(_url, [&](QQmlContext* context, QObject* obj) {});
        _webSurface->resume();

        _webSurface->getRootContext()->setContextProperty("Users", DependencyManager::get<UsersScriptingInterface>().data());
        _webSurface->getRootContext()->setContextProperty("HMD", DependencyManager::get<HMDScriptingInterface>().data());
        _webSurface->getRootContext()->setContextProperty("UserActivityLogger", DependencyManager::get<UserActivityLoggerScriptingInterface>().data());
        _webSurface->getRootContext()->setContextProperty("Preferences", DependencyManager::get<Preferences>().data());
        _webSurface->getRootContext()->setContextProperty("Vec3", new Vec3());
        _webSurface->getRootContext()->setContextProperty("Quat", new Quat());
        _webSurface->getRootContext()->setContextProperty("MyAvatar", DependencyManager::get<AvatarManager>()->getMyAvatar().get());
        _webSurface->getRootContext()->setContextProperty("Entities", DependencyManager::get<EntityScriptingInterface>().data());

        if (_webSurface->getRootItem() && _webSurface->getRootItem()->objectName() == "tabletRoot") {
            auto tabletScriptingInterface = DependencyManager::get<TabletScriptingInterface>();
            auto flags = tabletScriptingInterface->getFlags();
            _webSurface->getRootContext()->setContextProperty("offscreenFlags", flags);
            _webSurface->getRootContext()->setContextProperty("AddressManager", DependencyManager::get<AddressManager>().data());
            _webSurface->getRootContext()->setContextProperty("Account", AccountScriptingInterface::getInstance());
            _webSurface->getRootContext()->setContextProperty("HMD", DependencyManager::get<HMDScriptingInterface>().data());
            _webSurface->getRootContext()->setContextProperty("fileDialogHelper", new FileDialogHelper());
            _webSurface->getRootContext()->setContextProperty("ScriptDiscoveryService", DependencyManager::get<ScriptEngines>().data());
            _webSurface->getRootContext()->setContextProperty("Assets", DependencyManager::get<AssetMappingsScriptingInterface>().data());
            _webSurface->getRootContext()->setContextProperty("pathToFonts", "../../../");
            tabletScriptingInterface->setQmlTabletRoot("com.highfidelity.interface.tablet.system", _webSurface->getRootItem(), _webSurface.data());

            // Override min fps for tablet UI, for silky smooth scrolling
            _webSurface->setMaxFps(90);
        }
    }
    _webSurface->getRootContext()->setContextProperty("globalPosition", vec3toVariant(getPosition()));
}

void Web3DOverlay::render(RenderArgs* args) {
    if (!_visible || !getParentVisible()) {
        return;
    }

    QOpenGLContext * currentContext = QOpenGLContext::currentContext();
    QSurface * currentSurface = currentContext->surface();
    if (!_webSurface) {
        _webSurface = DependencyManager::get<OffscreenQmlSurfaceCache>()->acquire(pickURL());
        _webSurface->setMaxFps(10);
        // FIXME, the max FPS could be better managed by being dynamic (based on the number of current surfaces
        // and the current rendering load)
        loadSourceURL();
        _webSurface->resume();
        _webSurface->resize(QSize(_resolution.x, _resolution.y));
        _webSurface->getRootItem()->setProperty("url", _url);
        _webSurface->getRootItem()->setProperty("scriptURL", _scriptURL);
        currentContext->makeCurrent(currentSurface);

        auto selfOverlayID = getOverlayID();
        std::weak_ptr<Web3DOverlay> weakSelf = std::dynamic_pointer_cast<Web3DOverlay>(qApp->getOverlays().getOverlay(selfOverlayID));
        auto forwardPointerEvent = [=](OverlayID overlayID, const PointerEvent& event) {
            auto self = weakSelf.lock();
            if (!self) {
                return;
            }
            if (overlayID == selfOverlayID) {
                self->handlePointerEvent(event);
            }
        };

        _mousePressConnection = connect(&(qApp->getOverlays()), &Overlays::mousePressOnOverlay, this, forwardPointerEvent, Qt::DirectConnection);
        _mouseReleaseConnection = connect(&(qApp->getOverlays()), &Overlays::mouseReleaseOnOverlay, this, forwardPointerEvent, Qt::DirectConnection);
        _mouseMoveConnection = connect(&(qApp->getOverlays()), &Overlays::mouseMoveOnOverlay, this, forwardPointerEvent, Qt::DirectConnection);
        _hoverLeaveConnection = connect(&(qApp->getOverlays()), &Overlays::hoverLeaveOverlay, this, [=](OverlayID overlayID, const PointerEvent& event) {
            auto self = weakSelf.lock();
            if (!self) {
                return;
            }
            if (self->_pressed && overlayID == selfOverlayID) {
                // If the user mouses off the overlay while the button is down, simulate a touch end.
                QTouchEvent::TouchPoint point;
                point.setId(event.getID());
                point.setState(Qt::TouchPointReleased);
                glm::vec2 windowPos = event.getPos2D() * (METERS_TO_INCHES * _dpi);
                QPointF windowPoint(windowPos.x, windowPos.y);
                point.setScenePos(windowPoint);
                point.setPos(windowPoint);
                QList<QTouchEvent::TouchPoint> touchPoints;
                touchPoints.push_back(point);
                QTouchEvent* touchEvent = new QTouchEvent(QEvent::TouchEnd, nullptr, Qt::NoModifier, Qt::TouchPointReleased,
                    touchPoints);
                touchEvent->setWindow(self->_webSurface->getWindow());
                touchEvent->setDevice(&_touchDevice);
                touchEvent->setTarget(self->_webSurface->getRootItem());
                QCoreApplication::postEvent(self->_webSurface->getWindow(), touchEvent);
            }
        }, Qt::DirectConnection);

        _emitScriptEventConnection = connect(this, &Web3DOverlay::scriptEventReceived, _webSurface.data(), &OffscreenQmlSurface::emitScriptEvent);
        _webEventReceivedConnection = connect(_webSurface.data(), &OffscreenQmlSurface::webEventReceived, this, &Web3DOverlay::webEventReceived);
    }

    vec2 halfSize = getSize() / 2.0f;
    vec4 color(toGlm(getColor()), getAlpha());

    Transform transform = getTransform();

    // FIXME: applyTransformTo causes tablet overlay to detach from tablet entity.
    // Perhaps rather than deleting the following code it should be run only if isFacingAvatar() is true?
    /*
    applyTransformTo(transform, true);
    setTransform(transform);
    */

    if (glm::length2(getDimensions()) != 1.0f) {
        transform.postScale(vec3(getDimensions(), 1.0f));
    }

    if (!_texture) {
        auto webSurface = _webSurface;
        _texture = gpu::TexturePointer(gpu::Texture::createExternal2D(OffscreenQmlSurface::getDiscardLambda()));
        _texture->setSource(__FUNCTION__);
    }
    OffscreenQmlSurface::TextureAndFence newTextureAndFence;
    bool newTextureAvailable = _webSurface->fetchTexture(newTextureAndFence);
    if (newTextureAvailable) {
        _texture->setExternalTexture(newTextureAndFence.first, newTextureAndFence.second);
    }

    Q_ASSERT(args->_batch);
    gpu::Batch& batch = *args->_batch;
    batch.setResourceTexture(0, _texture);
    batch.setModelTransform(transform);
    auto geometryCache = DependencyManager::get<GeometryCache>();
    if (color.a < OPAQUE_ALPHA_THRESHOLD) {
        geometryCache->bindTransparentWebBrowserProgram(batch, _isAA);
    } else {
        geometryCache->bindOpaqueWebBrowserProgram(batch, _isAA);
    }
    geometryCache->renderQuad(batch, halfSize * -1.0f, halfSize, vec2(0), vec2(1), color, _geometryId);
    batch.setResourceTexture(0, args->_whiteTexture); // restore default white color after me
}

const render::ShapeKey Web3DOverlay::getShapeKey() {
    auto builder = render::ShapeKey::Builder().withoutCullFace().withDepthBias();
    if (getAlpha() != 1.0f) {
        builder.withTranslucent();
    }
    return builder.build();
}

QObject* Web3DOverlay::getEventHandler() {
    if (!_webSurface) {
        return nullptr;
    }
    return _webSurface->getEventHandler();
}

void Web3DOverlay::setProxyWindow(QWindow* proxyWindow) {
    if (!_webSurface) {
        return;
    }

    _webSurface->setProxyWindow(proxyWindow);
}

void Web3DOverlay::handlePointerEvent(const PointerEvent& event) {
    if (!_webSurface) {
        return;
    }

    glm::vec2 windowPos = event.getPos2D() * (METERS_TO_INCHES * _dpi);
    QPointF windowPoint(windowPos.x, windowPos.y);

    if (event.getType() == PointerEvent::Move) {
        // Forward a mouse move event to the Web surface.
        QMouseEvent* mouseEvent = new QMouseEvent(QEvent::MouseMove, windowPoint, windowPoint, windowPoint, Qt::NoButton,
            Qt::NoButton, Qt::NoModifier);
        QCoreApplication::postEvent(_webSurface->getWindow(), mouseEvent);
    }

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

    QTouchEvent* touchEvent = new QTouchEvent(type, &_touchDevice, event.getKeyboardModifiers());
    touchEvent->setWindow(_webSurface->getWindow());
    touchEvent->setTarget(_webSurface->getRootItem());
    touchEvent->setTouchPoints(touchPoints);
    touchEvent->setTouchPointStates(touchPointState);

    QCoreApplication::postEvent(_webSurface->getWindow(), touchEvent);
}

void Web3DOverlay::setProperties(const QVariantMap& properties) {
    Billboard3DOverlay::setProperties(properties);

    auto urlValue = properties["url"];
    if (urlValue.isValid()) {
        QString newURL = urlValue.toString();
        if (newURL != _url) {
            setURL(newURL);
        }
    }

    auto scriptURLValue = properties["scriptURL"];
    if (scriptURLValue.isValid()) {
        QString newScriptURL = scriptURLValue.toString();
        if (newScriptURL != _scriptURL) {
            setScriptURL(newScriptURL);
        }
    }

    auto resolution = properties["resolution"];
    if (resolution.isValid()) {
        bool valid;
        auto res = vec2FromVariant(resolution, valid);
        if (valid) {
            _resolution = res;
        }
    }

    auto dpi = properties["dpi"];
    if (dpi.isValid()) {
        _dpi = dpi.toFloat();
    }

    auto showKeyboardFocusHighlight = properties["showKeyboardFocusHighlight"];
    if (showKeyboardFocusHighlight.isValid()) {
        _showKeyboardFocusHighlight = showKeyboardFocusHighlight.toBool();
    }
}

QVariant Web3DOverlay::getProperty(const QString& property) {
    if (property == "url") {
        return _url;
    }
    if (property == "scriptURL") {
        return _scriptURL;
    }
    if (property == "resolution") {
        return vec2toVariant(_resolution);
    }
    if (property == "dpi") {
        return _dpi;
    }
    if (property == "showKeyboardFocusHighlight") {
        return _showKeyboardFocusHighlight;
    }
    return Billboard3DOverlay::getProperty(property);
}

void Web3DOverlay::setURL(const QString& url) {
    _url = url;
    if (_webSurface) {
        AbstractViewStateInterface::instance()->postLambdaEvent([this, url] {
            loadSourceURL();
        });
    }
}

void Web3DOverlay::setScriptURL(const QString& scriptURL) {
    _scriptURL = scriptURL;
    if (_webSurface) {
        AbstractViewStateInterface::instance()->postLambdaEvent([this, scriptURL] {
            _webSurface->getRootItem()->setProperty("scriptURL", scriptURL);
        });
    }
}

glm::vec2 Web3DOverlay::getSize() {
    return _resolution / _dpi * INCHES_TO_METERS * getDimensions();
};

bool Web3DOverlay::findRayIntersection(const glm::vec3& origin, const glm::vec3& direction, float& distance, BoxFace& face, glm::vec3& surfaceNormal) {
    // FIXME - face and surfaceNormal not being returned

    // Don't call applyTransformTo() or setTransform() here because this code runs too frequently.

    // Produce the dimensions of the overlay based on the image's aspect ratio and the overlay's scale.
    return findRayRectangleIntersection(origin, direction, getRotation(), getPosition(), getSize(), distance);
}

Web3DOverlay* Web3DOverlay::createClone() const {
    return new Web3DOverlay(this);
}

void Web3DOverlay::emitScriptEvent(const QVariant& message) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "emitScriptEvent", Qt::QueuedConnection, Q_ARG(QVariant, message));
    } else {
        emit scriptEventReceived(message);
    }
}
