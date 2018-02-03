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
#include "scripting/HMDScriptingInterface.h"
#include "scripting/AssetMappingsScriptingInterface.h"
#include "scripting/MenuScriptingInterface.h"
#include "scripting/SettingsScriptingInterface.h"
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
#include "scripting/AccountServicesScriptingInterface.h"
#include <plugins/InputConfiguration.h>
#include "ui/Snapshot.h"
#include "SoundCache.h"
#include "raypick/PointerScriptingInterface.h"

static int MAX_WINDOW_SIZE = 4096;
static const float METERS_TO_INCHES = 39.3701f;
static const float OPAQUE_ALPHA_THRESHOLD = 0.99f;

const QString Web3DOverlay::TYPE = "web3d";
const QString Web3DOverlay::QML = "Web3DOverlay.qml";
Web3DOverlay::Web3DOverlay() {
    _touchDevice.setCapabilities(QTouchDevice::Position);
    _touchDevice.setType(QTouchDevice::TouchScreen);
    _touchDevice.setName("Web3DOverlayTouchDevice");
    _touchDevice.setMaximumTouchPoints(4);

    _geometryId = DependencyManager::get<GeometryCache>()->allocateID();
    connect(this, &Web3DOverlay::requestWebSurface, this, &Web3DOverlay::buildWebSurface);
    connect(this, &Web3DOverlay::releaseWebSurface, this, &Web3DOverlay::destroyWebSurface);
    connect(this, &Web3DOverlay::resizeWebSurface, this, &Web3DOverlay::onResizeWebSurface);

    //need to be intialized before Tablet 1st open
    _webSurface = DependencyManager::get<OffscreenQmlSurfaceCache>()->acquire(_url);
    _webSurface->getSurfaceContext()->setContextProperty("HMD", DependencyManager::get<HMDScriptingInterface>().data());
    _webSurface->getSurfaceContext()->setContextProperty("Account", AccountServicesScriptingInterface::getInstance()); // DEPRECATED - TO BE REMOVED
    _webSurface->getSurfaceContext()->setContextProperty("GlobalServices", AccountServicesScriptingInterface::getInstance()); // DEPRECATED - TO BE REMOVED
    _webSurface->getSurfaceContext()->setContextProperty("AccountServices", AccountServicesScriptingInterface::getInstance());
    _webSurface->getSurfaceContext()->setContextProperty("AddressManager", DependencyManager::get<AddressManager>().data());

}

Web3DOverlay::Web3DOverlay(const Web3DOverlay* Web3DOverlay) :
    Billboard3DOverlay(Web3DOverlay),
    _url(Web3DOverlay->_url),
    _scriptURL(Web3DOverlay->_scriptURL),
    _dpi(Web3DOverlay->_dpi),
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

void Web3DOverlay::rebuildWebSurface() {
    destroyWebSurface();
    buildWebSurface();
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
        // FIXME, the max FPS could be better managed by being dynamic (based on the number of current surfaces
        // and the current rendering load)
        if (_currentMaxFPS != _desiredMaxFPS) {
            setMaxFPS(_desiredMaxFPS);
        }

        if (isWebContent()) {
            _webSurface = DependencyManager::get<OffscreenQmlSurfaceCache>()->acquire(QML);
            _webSurface->getRootItem()->setProperty("url", _url);
            _webSurface->getRootItem()->setProperty("scriptURL", _scriptURL);
        } else {
            _webSurface = DependencyManager::get<OffscreenQmlSurfaceCache>()->acquire(_url);
            setupQmlSurface();
        }
        _webSurface->getSurfaceContext()->setContextProperty("globalPosition", vec3toVariant(getWorldPosition()));
        onResizeWebSurface();
        _webSurface->resume();
    });

    QObject::connect(this, &Web3DOverlay::scriptEventReceived, _webSurface.data(), &OffscreenQmlSurface::emitScriptEvent);
    QObject::connect(_webSurface.data(), &OffscreenQmlSurface::webEventReceived, this, &Web3DOverlay::webEventReceived);
}

void Web3DOverlay::update(float deltatime) {
    if (_webSurface) {
        // update globalPosition
        _webSurface->getSurfaceContext()->setContextProperty("globalPosition", vec3toVariant(getWorldPosition()));
    }
    Parent::update(deltatime);
}

bool Web3DOverlay::isWebContent() const {
    QUrl sourceUrl(_url);
    if (sourceUrl.scheme() == "http" || sourceUrl.scheme() == "https" ||
        _url.toLower().endsWith(".htm") || _url.toLower().endsWith(".html")) {
        return true;
    }
    return false;
}

void Web3DOverlay::setupQmlSurface() {
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
        // in Qt 5.10.0 there is already an "Audio" object in the QML context
        // though I failed to find it (from QtMultimedia??). So..  let it be "AudioScriptingInterface"
        _webSurface->getSurfaceContext()->setContextProperty("AudioScriptingInterface", DependencyManager::get<AudioScriptingInterface>().data());

        _webSurface->getSurfaceContext()->setContextProperty("AudioStats", DependencyManager::get<AudioClient>()->getStats().data());
        _webSurface->getSurfaceContext()->setContextProperty("HMD", DependencyManager::get<HMDScriptingInterface>().data());
        _webSurface->getSurfaceContext()->setContextProperty("fileDialogHelper", new FileDialogHelper());
        _webSurface->getSurfaceContext()->setContextProperty("MyAvatar", DependencyManager::get<AvatarManager>()->getMyAvatar().get());
        _webSurface->getSurfaceContext()->setContextProperty("ScriptDiscoveryService", DependencyManager::get<ScriptEngines>().data());
        _webSurface->getSurfaceContext()->setContextProperty("Assets", DependencyManager::get<AssetMappingsScriptingInterface>().data());
        _webSurface->getSurfaceContext()->setContextProperty("LODManager", DependencyManager::get<LODManager>().data());
        _webSurface->getSurfaceContext()->setContextProperty("OctreeStats", DependencyManager::get<OctreeStatsProvider>().data());
        _webSurface->getSurfaceContext()->setContextProperty("DCModel", DependencyManager::get<DomainConnectionModel>().data());
        _webSurface->getSurfaceContext()->setContextProperty("AvatarInputs", AvatarInputs::getInstance());
        _webSurface->getSurfaceContext()->setContextProperty("AvatarList", DependencyManager::get<AvatarManager>().data());
        _webSurface->getSurfaceContext()->setContextProperty("DialogsManager", DialogsManagerScriptingInterface::getInstance());
        _webSurface->getSurfaceContext()->setContextProperty("InputConfiguration", DependencyManager::get<InputConfiguration>().data());
        _webSurface->getSurfaceContext()->setContextProperty("SoundCache", DependencyManager::get<SoundCache>().data());
        _webSurface->getSurfaceContext()->setContextProperty("MenuInterface", MenuScriptingInterface::getInstance());
        _webSurface->getSurfaceContext()->setContextProperty("Settings", SettingsScriptingInterface::getInstance());
        _webSurface->getSurfaceContext()->setContextProperty("Render", AbstractViewStateInterface::instance()->getRenderEngine()->getConfiguration().get());
        _webSurface->getSurfaceContext()->setContextProperty("Controller", DependencyManager::get<controller::ScriptingInterface>().data());
        _webSurface->getSurfaceContext()->setContextProperty("Pointers", DependencyManager::get<PointerScriptingInterface>().data());
        _webSurface->getSurfaceContext()->setContextProperty("Web3DOverlay", this);

        _webSurface->getSurfaceContext()->setContextProperty("pathToFonts", "../../");

        // Override min fps for tablet UI, for silky smooth scrolling
        setMaxFPS(90);
    }
}

void Web3DOverlay::setMaxFPS(uint8_t maxFPS) {
    _desiredMaxFPS = maxFPS;
    if (_webSurface) {
        _webSurface->setMaxFps(_desiredMaxFPS);
        _currentMaxFPS = _desiredMaxFPS;
    }
}

void Web3DOverlay::onResizeWebSurface() {
    glm::vec2 dims = glm::vec2(getDimensions());
    dims *= METERS_TO_INCHES * _dpi;

    // ensure no side is never larger then MAX_WINDOW_SIZE
    float max = (dims.x > dims.y) ? dims.x : dims.y;
    if (max > MAX_WINDOW_SIZE) {
        dims *= MAX_WINDOW_SIZE / max;
    }

    _webSurface->resize(QSize(dims.x, dims.y));
}

unsigned int Web3DOverlay::deviceIdByTouchPoint(qreal x, qreal y) {
    if (_webSurface) {
        return _webSurface->deviceIdByTouchPoint(x, y);
    } else {
        return PointerEvent::INVALID_POINTER_ID;
    }
}

void Web3DOverlay::render(RenderArgs* args) {
    if (!_renderVisible || !getParentVisible()) {
        return;
    }

    if (!_webSurface) {
        emit requestWebSurface();
        return;
    }

    if (_mayNeedResize) {
        emit resizeWebSurface();
    }

    if (_currentMaxFPS != _desiredMaxFPS) {
        setMaxFPS(_desiredMaxFPS);
    }

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
    auto size = renderTransform.getScale();
    renderTransform.setScale(1.0f);
    batch.setModelTransform(renderTransform);

    auto geometryCache = DependencyManager::get<GeometryCache>();
    if (color.a < OPAQUE_ALPHA_THRESHOLD) {
        geometryCache->bindWebBrowserProgram(batch, true);
    } else {
        geometryCache->bindWebBrowserProgram(batch);
    }

    vec2 halfSize = vec2(size.x, size.y) / 2.0f;
    geometryCache->renderQuad(batch, halfSize * -1.0f, halfSize, vec2(0), vec2(1), color, _geometryId);
    batch.setResourceTexture(0, nullptr); // restore default white color after me
}

Transform Web3DOverlay::evalRenderTransform() {
    Transform transform = Parent::evalRenderTransform();
    transform.setScale(1.0f);
    transform.postScale(glm::vec3(getDimensions(), 1.0f));
    return transform;
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

void Web3DOverlay::hoverEnterOverlay(const PointerEvent& event) {
    if (_inputMode == Mouse) {
        handlePointerEvent(event);
    } else if (_webSurface) {
        PointerEvent webEvent = event;
        webEvent.setPos2D(event.getPos2D() * (METERS_TO_INCHES * _dpi));
        _webSurface->hoverBeginEvent(webEvent, _touchDevice);
    }
}

void Web3DOverlay::hoverLeaveOverlay(const PointerEvent& event) {
    if (_inputMode == Mouse) {
        PointerEvent endEvent(PointerEvent::Release, event.getID(), event.getPos2D(), event.getPos3D(), event.getNormal(), event.getDirection(),
            event.getButton(), event.getButtons(), event.getKeyboardModifiers());
        handlePointerEvent(endEvent);
        // QML onReleased is only triggered if a click has happened first.  We need to send this "fake" mouse move event to properly trigger an onExited.
        PointerEvent endMoveEvent(PointerEvent::Move, event.getID());
        handlePointerEvent(endMoveEvent);
    } else if (_webSurface) {
        PointerEvent webEvent = event;
        webEvent.setPos2D(event.getPos2D() * (METERS_TO_INCHES * _dpi));
        _webSurface->hoverEndEvent(webEvent, _touchDevice);
    }
}

void Web3DOverlay::handlePointerEvent(const PointerEvent& event) {
    if (_inputMode == Touch) {
        handlePointerEventAsTouch(event);
    } else {
        handlePointerEventAsMouse(event);
    }
}

void Web3DOverlay::handlePointerEventAsTouch(const PointerEvent& event) {
    if (_webSurface) {
        PointerEvent webEvent = event;
        webEvent.setPos2D(event.getPos2D() * (METERS_TO_INCHES * _dpi));
        _webSurface->handlePointerEvent(webEvent, _touchDevice);
    }
}

void Web3DOverlay::handlePointerEventAsMouse(const PointerEvent& event) {
    if (!_webSurface) {
        return;
    }

    glm::vec2 windowPos = event.getPos2D() * (METERS_TO_INCHES * _dpi);
    QPointF windowPoint(windowPos.x, windowPos.y);

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

    QMouseEvent mouseEvent(type, windowPoint, windowPoint, windowPoint, button, buttons, event.getKeyboardModifiers());
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

    auto dpi = properties["dpi"];
    if (dpi.isValid()) {
        _dpi = dpi.toFloat();
        _mayNeedResize = true;
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
}

// Web3DOverlay overrides the meaning of Planar3DOverlay's dimensions property.
/**jsdoc
 * These are the properties of a <code>web3d</code> {@link Overlays.OverlayType|OverlayType}.
 * @typedef {object} Overlays.Web3DProperties
 *
 * @property {string} type=web3d - Has the value <code>"web3d"</code>. <em>Read-only.</em>
 * @property {Color} color=255,255,255 - The color of the overlay.
 * @property {number} alpha=0.7 - The opacity of the overlay, <code>0.0</code> - <code>1.0</code>.
 * @property {number} pulseMax=0 - The maximum value of the pulse multiplier.
 * @property {number} pulseMin=0 - The minimum value of the pulse multiplier.
 * @property {number} pulsePeriod=1 - The duration of the color and alpha pulse, in seconds. A pulse multiplier value goes from
 *     <code>pulseMin</code> to <code>pulseMax</code>, then <code>pulseMax</code> to <code>pulseMin</code> in one period.
 * @property {number} alphaPulse=0 - If non-zero, the alpha of the overlay is pulsed: the alpha value is multiplied by the
 *     current pulse multiplier value each frame. If > 0 the pulse multiplier is applied in phase with the pulse period; if < 0
 *     the pulse multiplier is applied out of phase with the pulse period. (The magnitude of the property isn't otherwise
 *     used.)
 * @property {number} colorPulse=0 - If non-zero, the color of the overlay is pulsed: the color value is multiplied by the
 *     current pulse multiplier value each frame. If > 0 the pulse multiplier is applied in phase with the pulse period; if < 0
 *     the pulse multiplier is applied out of phase with the pulse period. (The magnitude of the property isn't otherwise
 *     used.)
 * @property {boolean} visible=true - If <code>true</code>, the overlay is rendered, otherwise it is not rendered.
 *
 * @property {string} name="" - A friendly name for the overlay.
 * @property {Vec3} position - The position of the overlay center. Synonyms: <code>p1</code>, <code>point</code>, and 
 *     <code>start</code>.
 * @property {Vec3} localPosition - The local position of the overlay relative to its parent if the overlay has a
 *     <code>parentID</code> set, otherwise the same value as <code>position</code>.
 * @property {Quat} rotation - The orientation of the overlay. Synonym: <code>orientation</code>.
 * @property {Quat} localRotation - The orientation of the overlay relative to its parent if the overlay has a
 *     <code>parentID</code> set, otherwise the same value as <code>rotation</code>.
 * @property {boolean} isSolid=false - Synonyms: <ode>solid</code>, <code>isFilled</code>, and <code>filled</code>.
 *     Antonyms: <code>isWire</code> and <code>wire</code>.
 * @property {boolean} isDashedLine=false - If <code>true</code>, a dashed line is drawn on the overlay's edges. Synonym:
 *     <code>dashed</code>.
 * @property {boolean} ignoreRayIntersection=false - If <code>true</code>, 
 *     {@link Overlays.findRayIntersection|findRayIntersection} ignores the overlay.
 * @property {boolean} drawInFront=false - If <code>true</code>, the overlay is rendered in front of other overlays that don't
 *     have <code>drawInFront</code> set to <code>true</code>, and in front of entities.
 * @property {boolean} grabbable=false - Signal to grabbing scripts whether or not this overlay can be grabbed.
 * @property {Uuid} parentID=null - The avatar, entity, or overlay that the overlay is parented to.
 * @property {number} parentJointIndex=65535 - Integer value specifying the skeleton joint that the overlay is attached to if
 *     <code>parentID</code> is an avatar skeleton. A value of <code>65535</code> means "no joint".
 *
 * @property {boolean} isFacingAvatar - If <code>true</code>, the overlay is rotated to face the user's camera about an axis
 *     parallel to the user's avatar's "up" direction.
 *
 * @property {string} url - The URL of the Web page to display.
 * @property {string} scriptURL="" - The URL of a JavaScript file to inject into the Web page.
 * @property {number} dpi=30 - The dots per inch to display the Web page at, on the overlay.
 * @property {Vec2} dimensions=1,1 - The size of the overlay to display the Web page on, in meters. Synonyms: 
 *     <code>scale</code>, <code>size</code>.
 * @property {number} maxFPS=10 - The maximum update rate for the Web overlay content, in frames/second.
 * @property {boolean} showKeyboardFocusHighlight=true - If <code>true</code>, the Web overlay is highlighted when it has
 *     keyboard focus.
 * @property {string} inputMode=Touch - The user input mode to use - either <code>"Touch"</code> or <code>"Mouse"</code>.
 */
QVariant Web3DOverlay::getProperty(const QString& property) {
    if (property == "url") {
        return _url;
    }
    if (property == "scriptURL") {
        return _scriptURL;
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
    if (url != _url) {
        bool wasWebContent = isWebContent();
        _url = url;
        if (_webSurface) {
            if (wasWebContent && isWebContent()) {
                // If we're just targeting a new web URL, then switch to that without messing around
                // with the underlying QML
                AbstractViewStateInterface::instance()->postLambdaEvent([this, url] {
                    _webSurface->getRootItem()->setProperty("url", _url);
                    _webSurface->getRootItem()->setProperty("scriptURL", _scriptURL);
                });
            } else {
                // If we're switching to or from web content, or between different QML content
                // we need to destroy and rebuild the entire QML surface
                AbstractViewStateInterface::instance()->postLambdaEvent([this, url] {
                    rebuildWebSurface();
                });
            }
        }
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

bool Web3DOverlay::findRayIntersection(const glm::vec3& origin, const glm::vec3& direction, float& distance, BoxFace& face, glm::vec3& surfaceNormal) {
    glm::vec2 dimensions = getDimensions();
    glm::quat rotation = getWorldOrientation();
    glm::vec3 position = getWorldPosition();

    if (findRayRectangleIntersection(origin, direction, rotation, position, dimensions, distance)) {
        surfaceNormal = rotation * Vectors::UNIT_Z;
        face = glm::dot(surfaceNormal, direction) > 0 ? MIN_Z_FACE : MAX_Z_FACE;
        return true;
    } else {
        return false;
    }
}

Web3DOverlay* Web3DOverlay::createClone() const {
    return new Web3DOverlay(this);
}

void Web3DOverlay::emitScriptEvent(const QVariant& message) {
    QMetaObject::invokeMethod(this, "scriptEventReceived", Q_ARG(QVariant, message));
}
