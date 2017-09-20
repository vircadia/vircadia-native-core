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
#include <QtQml/QQmlEngine>

#include <AbstractViewStateInterface.h>
#include <gpu/Batch.h>
#include <DependencyManager.h>
#include <GeometryCache.h>
#include <GeometryUtil.h>
#include <gl/GLHelpers.h>
#include <scripting/HMDScriptingInterface.h>
#include <ui/OffscreenQmlSurface.h>
#include <ui/OffscreenQmlSurfaceCache.h>
#include <ui/TabletScriptingInterface.h>
#include <PathUtils.h>
#include <RegisteredMetaTypes.h>
#include <TextureCache.h>
#include <UsersScriptingInterface.h>
#include <UserActivityLoggerScriptingInterface.h>
#include <AbstractViewStateInterface.h>
#include <AddressManager.h>
#include "scripting/AccountScriptingInterface.h"
#include "scripting/HMDScriptingInterface.h"
#include "scripting/AssetMappingsScriptingInterface.h"
#include "scripting/MenuScriptingInterface.h"
#include <Preferences.h>
#include <ScriptEngines.h>
#include "FileDialogHelper.h"
#include "avatar/AvatarManager.h"
#include "AudioClient.h"
#include "LODManager.h"
#include "ui/OctreeStatsProvider.h"
#include "ui/DomainConnectionModel.h"
#include "ui/AvatarInputs.h"
#include "avatar/AvatarManager.h"
#include "scripting/GlobalServicesScriptingInterface.h"
#include <plugins/InputConfiguration.h>
#include "ui/Snapshot.h"
#include "SoundCache.h"

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
    connect(this, &Web3DOverlay::requestWebSurface, this, &Web3DOverlay::buildWebSurface);
    connect(this, &Web3DOverlay::releaseWebSurface, this, &Web3DOverlay::destroyWebSurface);
    connect(this, &Web3DOverlay::resizeWebSurface, this, &Web3DOverlay::onResizeWebSurface);
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
    disconnect(this, &Web3DOverlay::requestWebSurface, this, nullptr);
    disconnect(this, &Web3DOverlay::releaseWebSurface, this, nullptr);
    disconnect(this, &Web3DOverlay::resizeWebSurface, this, nullptr);

    destroyWebSurface();
    auto geometryCache = DependencyManager::get<GeometryCache>();
    if (geometryCache) {
        geometryCache->releaseID(_geometryId);
    }
}


void Web3DOverlay::destroyWebSurface() {
    if (!_webSurface) {
        return;
    }
    QQuickItem* rootItem = _webSurface->getRootItem();

    if (rootItem && rootItem->objectName() == "tabletRoot") {
        auto tabletScriptingInterface = DependencyManager::get<TabletScriptingInterface>();
        tabletScriptingInterface->setQmlTabletRoot("com.highfidelity.interface.tablet.system", nullptr);
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
    auto overlays = &(qApp->getOverlays());
    QObject::disconnect(overlays, &Overlays::mousePressOnOverlay, this, nullptr);
    QObject::disconnect(overlays, &Overlays::mouseReleaseOnOverlay, this, nullptr);
    QObject::disconnect(overlays, &Overlays::mouseMoveOnOverlay, this, nullptr);
    QObject::disconnect(overlays, &Overlays::hoverLeaveOverlay, this, nullptr);
    QObject::disconnect(this, &Web3DOverlay::scriptEventReceived, _webSurface.data(), &OffscreenQmlSurface::emitScriptEvent);
    QObject::disconnect(_webSurface.data(), &OffscreenQmlSurface::webEventReceived, this, &Web3DOverlay::webEventReceived);
    DependencyManager::get<OffscreenQmlSurfaceCache>()->release(QML, _webSurface);
    _webSurface.reset();
}

void Web3DOverlay::buildWebSurface() {
    if (_webSurface) {
        return;
    }
    gl::withSavedContext([&] {
        _webSurface = DependencyManager::get<OffscreenQmlSurfaceCache>()->acquire(pickURL());
        // FIXME, the max FPS could be better managed by being dynamic (based on the number of current surfaces
        // and the current rendering load)
        if (_currentMaxFPS != _desiredMaxFPS) {
            setMaxFPS(_desiredMaxFPS);
        }
        loadSourceURL();
        _webSurface->resume();
        _webSurface->resize(QSize(_resolution.x, _resolution.y));
        _webSurface->getRootItem()->setProperty("url", _url);
        _webSurface->getRootItem()->setProperty("scriptURL", _scriptURL);
    });

    auto selfOverlayID = getOverlayID();
    std::weak_ptr<Web3DOverlay> weakSelf = std::dynamic_pointer_cast<Web3DOverlay>(qApp->getOverlays().getOverlay(selfOverlayID));
    auto forwardPointerEvent = [=](OverlayID overlayID, const PointerEvent& event) {
        auto self = weakSelf.lock();
        if (self && overlayID == selfOverlayID) {
            self->handlePointerEvent(event);
        }
    };

    auto overlays = &(qApp->getOverlays());
    QObject::connect(overlays, &Overlays::mousePressOnOverlay, this, forwardPointerEvent);
    QObject::connect(overlays, &Overlays::mouseReleaseOnOverlay, this, forwardPointerEvent);
    QObject::connect(overlays, &Overlays::mouseMoveOnOverlay, this, forwardPointerEvent);
    QObject::connect(overlays, &Overlays::hoverLeaveOverlay, this, [=](OverlayID overlayID, const PointerEvent& event) {
        auto self = weakSelf.lock();
        if (!self) {
            return;
        }
        if (overlayID == selfOverlayID && (self->_pressed || (!self->_activeTouchPoints.empty() && self->_touchBeginAccepted))) {
            PointerEvent endEvent(PointerEvent::Release, event.getID(), event.getPos2D(), event.getPos3D(), event.getNormal(), event.getDirection(),
                event.getButton(), event.getButtons(), event.getKeyboardModifiers());
            forwardPointerEvent(overlayID, endEvent);
        }
    });

    QObject::connect(this, &Web3DOverlay::scriptEventReceived, _webSurface.data(), &OffscreenQmlSurface::emitScriptEvent);
    QObject::connect(_webSurface.data(), &OffscreenQmlSurface::webEventReceived, this, &Web3DOverlay::webEventReceived);
}


void Web3DOverlay::update(float deltatime) {
    if (_webSurface) {
        // update globalPosition
        _webSurface->getSurfaceContext()->setContextProperty("globalPosition", vec3toVariant(getPosition()));
    }
    Parent::update(deltatime);
}

QString Web3DOverlay::pickURL() {
    QUrl sourceUrl(_url);
    if (sourceUrl.scheme() == "http" || sourceUrl.scheme() == "https" ||
        _url.toLower().endsWith(".htm") || _url.toLower().endsWith(".html")) {
        if (_webSurface) {
            _webSurface->setBaseUrl(QUrl::fromLocalFile(PathUtils::resourcesPath() + "/qml/"));
        }
        return "Web3DOverlay.qml";
    } else {
        return QUrl::fromLocalFile(PathUtils::resourcesPath()).toString() + "/" + _url;
    }
}

void Web3DOverlay::loadSourceURL() {
    if (!_webSurface) {
        return;
    }

    QUrl sourceUrl(_url);
    if (sourceUrl.scheme() == "http" || sourceUrl.scheme() == "https" ||
        _url.toLower().endsWith(".htm") || _url.toLower().endsWith(".html")) {

        _webSurface->setBaseUrl(QUrl::fromLocalFile(PathUtils::resourcesPath() + "/qml/"));
        _webSurface->load("Web3DOverlay.qml");
        _webSurface->resume();
        _webSurface->getRootItem()->setProperty("url", _url);
        _webSurface->getRootItem()->setProperty("scriptURL", _scriptURL);

    } else {
        _webSurface->setBaseUrl(QUrl::fromLocalFile(PathUtils::resourcesPath()));
        _webSurface->load(_url, [&](QQmlContext* context, QObject* obj) {});
        _webSurface->resume();

        _webSurface->getSurfaceContext()->setContextProperty("Users", DependencyManager::get<UsersScriptingInterface>().data());
        _webSurface->getSurfaceContext()->setContextProperty("HMD", DependencyManager::get<HMDScriptingInterface>().data());
        _webSurface->getSurfaceContext()->setContextProperty("UserActivityLogger", DependencyManager::get<UserActivityLoggerScriptingInterface>().data());
        _webSurface->getSurfaceContext()->setContextProperty("Preferences", DependencyManager::get<Preferences>().data());
        _webSurface->getSurfaceContext()->setContextProperty("Vec3", new Vec3());
        _webSurface->getSurfaceContext()->setContextProperty("Quat", new Quat());
        _webSurface->getSurfaceContext()->setContextProperty("MyAvatar", DependencyManager::get<AvatarManager>()->getMyAvatar().get());
        _webSurface->getSurfaceContext()->setContextProperty("Entities", DependencyManager::get<EntityScriptingInterface>().data());
        _webSurface->getSurfaceContext()->setContextProperty("Snapshot", DependencyManager::get<Snapshot>().data());

        if (_webSurface->getRootItem() && _webSurface->getRootItem()->objectName() == "tabletRoot") {
            auto tabletScriptingInterface = DependencyManager::get<TabletScriptingInterface>();
            auto flags = tabletScriptingInterface->getFlags();

            _webSurface->getSurfaceContext()->setContextProperty("offscreenFlags", flags);
            _webSurface->getSurfaceContext()->setContextProperty("AddressManager", DependencyManager::get<AddressManager>().data());
            _webSurface->getSurfaceContext()->setContextProperty("Account", AccountScriptingInterface::getInstance());
            _webSurface->getSurfaceContext()->setContextProperty("Audio", DependencyManager::get<AudioScriptingInterface>().data());
            _webSurface->getSurfaceContext()->setContextProperty("AudioStats", DependencyManager::get<AudioClient>()->getStats().data());
            _webSurface->getSurfaceContext()->setContextProperty("HMD", DependencyManager::get<HMDScriptingInterface>().data());
            _webSurface->getSurfaceContext()->setContextProperty("fileDialogHelper", new FileDialogHelper());
            _webSurface->getSurfaceContext()->setContextProperty("MyAvatar", DependencyManager::get<AvatarManager>()->getMyAvatar().get());
            _webSurface->getSurfaceContext()->setContextProperty("ScriptDiscoveryService", DependencyManager::get<ScriptEngines>().data());
            _webSurface->getSurfaceContext()->setContextProperty("Tablet", DependencyManager::get<TabletScriptingInterface>().data());
            _webSurface->getSurfaceContext()->setContextProperty("Assets", DependencyManager::get<AssetMappingsScriptingInterface>().data());
            _webSurface->getSurfaceContext()->setContextProperty("LODManager", DependencyManager::get<LODManager>().data());
            _webSurface->getSurfaceContext()->setContextProperty("OctreeStats", DependencyManager::get<OctreeStatsProvider>().data());
            _webSurface->getSurfaceContext()->setContextProperty("DCModel", DependencyManager::get<DomainConnectionModel>().data());
            _webSurface->getSurfaceContext()->setContextProperty("AvatarInputs", AvatarInputs::getInstance());
            _webSurface->getSurfaceContext()->setContextProperty("GlobalServices", GlobalServicesScriptingInterface::getInstance());
            _webSurface->getSurfaceContext()->setContextProperty("AvatarList", DependencyManager::get<AvatarManager>().data());
            _webSurface->getSurfaceContext()->setContextProperty("DialogsManager", DialogsManagerScriptingInterface::getInstance());
            _webSurface->getSurfaceContext()->setContextProperty("InputConfiguration", DependencyManager::get<InputConfiguration>().data());
            _webSurface->getSurfaceContext()->setContextProperty("SoundCache", DependencyManager::get<SoundCache>().data());
            _webSurface->getSurfaceContext()->setContextProperty("MenuInterface", MenuScriptingInterface::getInstance());

            _webSurface->getSurfaceContext()->setContextProperty("pathToFonts", "../../");

            tabletScriptingInterface->setQmlTabletRoot("com.highfidelity.interface.tablet.system", _webSurface.data());

            // mark the TabletProxy object as cpp ownership.
            QObject* tablet = tabletScriptingInterface->getTablet("com.highfidelity.interface.tablet.system");
            _webSurface->getSurfaceContext()->engine()->setObjectOwnership(tablet, QQmlEngine::CppOwnership);

            // Override min fps for tablet UI, for silky smooth scrolling
            setMaxFPS(90);
        }
    }
    _webSurface->getSurfaceContext()->setContextProperty("globalPosition", vec3toVariant(getPosition()));
}

void Web3DOverlay::setMaxFPS(uint8_t maxFPS) {
    _desiredMaxFPS = maxFPS;
    if (_webSurface) {
        _webSurface->setMaxFps(_desiredMaxFPS);
        _currentMaxFPS = _desiredMaxFPS;
    }
}

void Web3DOverlay::onResizeWebSurface() {
    _mayNeedResize = false;
    _webSurface->resize(QSize(_resolution.x, _resolution.y));
}

void Web3DOverlay::render(RenderArgs* args) {
    if (!_visible || !getParentVisible()) {
        return;
    }

    if (!_webSurface) {
        emit requestWebSurface();
        return;
    }

    if (_currentMaxFPS != _desiredMaxFPS) {
        setMaxFPS(_desiredMaxFPS);
    }

    if (_mayNeedResize) {
        emit resizeWebSurface();
    }

    vec2 halfSize = getSize() / 2.0f;
    vec4 color(toGlm(getColor()), getAlpha());

    if (!_texture) {
        _texture = gpu::Texture::createExternal(OffscreenQmlSurface::getDiscardLambda());
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
    auto renderTransform = getRenderTransform();
    batch.setModelTransform(getRenderTransform());

    auto geometryCache = DependencyManager::get<GeometryCache>();
    if (color.a < OPAQUE_ALPHA_THRESHOLD) {
        geometryCache->bindWebBrowserProgram(batch, true);
    } else {
        geometryCache->bindWebBrowserProgram(batch);
    }
    geometryCache->renderQuad(batch, halfSize * -1.0f, halfSize, vec2(0), vec2(1), color, _geometryId);
    batch.setResourceTexture(0, nullptr); // restore default white color after me
}

const render::ShapeKey Web3DOverlay::getShapeKey() {
    auto builder = render::ShapeKey::Builder().withoutCullFace().withDepthBias().withOwnPipeline();
    if (isTransparent()) {
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
    if (_inputMode == Touch) {
        handlePointerEventAsTouch(event);
    } else {
        handlePointerEventAsMouse(event);
    }
}

void Web3DOverlay::handlePointerEventAsTouch(const PointerEvent& event) {
    if (!_webSurface) {
        return;
    }

    //do not send secondary button events to tablet
    if (event.getButton() == PointerEvent::SecondaryButton ||
        //do not block composed events
        event.getButtons() == PointerEvent::SecondaryButton) {
        return;
    }


    QPointF windowPoint;
    {
        glm::vec2 windowPos = event.getPos2D() * (METERS_TO_INCHES * _dpi);
        windowPoint = QPointF(windowPos.x, windowPos.y);
    }

    Qt::TouchPointState state = Qt::TouchPointStationary;
    if (event.getType() == PointerEvent::Press && event.getButton() == PointerEvent::PrimaryButton) {
        state = Qt::TouchPointPressed;
    } else if (event.getType() == PointerEvent::Release) {
        state = Qt::TouchPointReleased;
    } else if (_activeTouchPoints.count(event.getID()) && windowPoint != _activeTouchPoints[event.getID()].pos()) {
        state = Qt::TouchPointMoved;
    }

    QEvent::Type touchType = QEvent::TouchUpdate;
    if (_activeTouchPoints.empty()) {
        // If the first active touch point is being created, send a begin
        touchType = QEvent::TouchBegin;
    } if (state == Qt::TouchPointReleased && _activeTouchPoints.size() == 1 && _activeTouchPoints.count(event.getID())) {
        // If the last active touch point is being released, send an end
        touchType = QEvent::TouchEnd;
    } 

    {
        QTouchEvent::TouchPoint point;
        point.setId(event.getID());
        point.setState(state);
        point.setPos(windowPoint);
        point.setScreenPos(windowPoint);
        _activeTouchPoints[event.getID()] = point;
    }

    QTouchEvent touchEvent(touchType, &_touchDevice, event.getKeyboardModifiers());
    {
        QList<QTouchEvent::TouchPoint> touchPoints;
        Qt::TouchPointStates touchPointStates;
        for (const auto& entry : _activeTouchPoints) {
            touchPointStates |= entry.second.state();
            touchPoints.push_back(entry.second);
        }

        touchEvent.setWindow(_webSurface->getWindow());
        touchEvent.setTarget(_webSurface->getRootItem());
        touchEvent.setTouchPoints(touchPoints);
        touchEvent.setTouchPointStates(touchPointStates);
    }

    // Send mouse events to the Web surface so that HTML dialog elements work with mouse press and hover.
    // FIXME: Scroll bar dragging is a bit unstable in the tablet (content can jump up and down at times).
    // This may be improved in Qt 5.8. Release notes: "Cleaned up touch and mouse event delivery".
    //
    // In Qt 5.9 mouse events must be sent before touch events to make sure some QtQuick components will
    // receive mouse events
    Qt::MouseButton button = Qt::NoButton;
    Qt::MouseButtons buttons = Qt::NoButton;
    if (event.getButton() == PointerEvent::PrimaryButton) {
        button = Qt::LeftButton;
    }
    if (event.getButtons() & PointerEvent::PrimaryButton) {
        buttons |= Qt::LeftButton;
    }

#if QT_VERSION >= QT_VERSION_CHECK(5, 9, 0)
    if (event.getType() == PointerEvent::Move) {
        QMouseEvent mouseEvent(QEvent::MouseMove, windowPoint, windowPoint, windowPoint, button, buttons, Qt::NoModifier);
        QCoreApplication::sendEvent(_webSurface->getWindow(), &mouseEvent);
    }
#endif

    if (touchType == QEvent::TouchBegin) {
        _touchBeginAccepted = QCoreApplication::sendEvent(_webSurface->getWindow(), &touchEvent);
    } else if (_touchBeginAccepted) {
        QCoreApplication::sendEvent(_webSurface->getWindow(), &touchEvent);
    }

    // If this was a release event, remove the point from the active touch points
    if (state == Qt::TouchPointReleased) {
        _activeTouchPoints.erase(event.getID());
    }

#if QT_VERSION < QT_VERSION_CHECK(5, 9, 0)
    if (event.getType() == PointerEvent::Move) {
        QMouseEvent mouseEvent(QEvent::MouseMove, windowPoint, windowPoint, windowPoint, button, buttons, Qt::NoModifier);
        QCoreApplication::sendEvent(_webSurface->getWindow(), &mouseEvent);
    }
#endif
}

void Web3DOverlay::handlePointerEventAsMouse(const PointerEvent& event) {
    if (!_webSurface) {
        return;
    }

    glm::vec2 windowPos = event.getPos2D() * (METERS_TO_INCHES * _dpi);
    QPointF windowPoint(windowPos.x, windowPos.y);

    if (event.getType() == PointerEvent::Press) {
        this->_pressed = true;
    } else if (event.getType() == PointerEvent::Release) {
        this->_pressed = false;
    }

    Qt::MouseButtons buttons = Qt::NoButton;
    if (event.getButtons() & PointerEvent::PrimaryButton) {
        buttons |= Qt::LeftButton;
    }

    Qt::MouseButton button = Qt::NoButton;
    if (event.getButton() == PointerEvent::PrimaryButton) {
        button = Qt::LeftButton;
    }

    QEvent::Type type;
    switch (event.getType()) {
        case PointerEvent::Press:
            type = QEvent::MouseButtonPress;
            break;
        case PointerEvent::Release:
            type = QEvent::MouseButtonRelease;
            break;
        case PointerEvent::Move:
            type = QEvent::MouseMove;
            break;
        default:
            return;
    }

    QMouseEvent mouseEvent(type, windowPoint, windowPoint, windowPoint, button, buttons, Qt::NoModifier);
    QCoreApplication::sendEvent(_webSurface->getWindow(), &mouseEvent);
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

    auto maxFPS = properties["maxFPS"];
    if (maxFPS.isValid()) {
        _desiredMaxFPS = maxFPS.toInt();
    }

    auto showKeyboardFocusHighlight = properties["showKeyboardFocusHighlight"];
    if (showKeyboardFocusHighlight.isValid()) {
        _showKeyboardFocusHighlight = showKeyboardFocusHighlight.toBool();
    }

    auto inputModeValue = properties["inputMode"];
    if (inputModeValue.isValid()) {
        QString inputModeStr = inputModeValue.toString();
        if (inputModeStr == "Mouse") {
            _inputMode = Mouse;
        } else {
            _inputMode = Touch;
        }
    }

    _mayNeedResize = true;
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
    if (property == "maxFPS") {
        return _desiredMaxFPS;
    }
    if (property == "showKeyboardFocusHighlight") {
        return _showKeyboardFocusHighlight;
    }

    if (property == "inputMode") {
        if (_inputMode == Mouse) {
            return QVariant("Mouse");
        } else {
            return QVariant("Touch");
        }
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
            if (!_webSurface) {
                return;
            }
            _webSurface->getRootItem()->setProperty("scriptURL", scriptURL);
        });
    }
}

glm::vec2 Web3DOverlay::getSize() const {
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
    QMetaObject::invokeMethod(this, "scriptEventReceived", Q_ARG(QVariant, message));
}
