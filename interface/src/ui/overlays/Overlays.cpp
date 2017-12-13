//
//  Overlays.cpp
//  interface/src/ui/overlays
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Overlays.h"

#include <limits>

#include <QtScript/QScriptValueIterator>

#include <shared/QtHelpers.h>
#include <OffscreenUi.h>
#include <render/Scene.h>
#include <RegisteredMetaTypes.h>

#include "Application.h"
#include "InterfaceLogging.h"
#include "Image3DOverlay.h"
#include "Circle3DOverlay.h"
#include "Cube3DOverlay.h"
#include "Shape3DOverlay.h"
#include "ImageOverlay.h"
#include "Line3DOverlay.h"
#include "ModelOverlay.h"
#include "Rectangle3DOverlay.h"
#include "Sphere3DOverlay.h"
#include "Grid3DOverlay.h"
#include "TextOverlay.h"
#include "RectangleOverlay.h"
#include "Text3DOverlay.h"
#include "Web3DOverlay.h"
#include <QtQuick/QQuickWindow>

#include <PointerManager.h>

Q_LOGGING_CATEGORY(trace_render_overlays, "trace.render.overlays")

extern void initOverlay3DPipelines(render::ShapePlumber& plumber, bool depthTest = false);

Overlays::Overlays() {
    auto pointerManager = DependencyManager::get<PointerManager>();
    connect(pointerManager.data(), &PointerManager::hoverBeginOverlay, this, &Overlays::hoverEnterPointerEvent);
    connect(pointerManager.data(), &PointerManager::hoverContinueOverlay, this, &Overlays::hoverOverPointerEvent);
    connect(pointerManager.data(), &PointerManager::hoverEndOverlay, this, &Overlays::hoverLeavePointerEvent);
    connect(pointerManager.data(), &PointerManager::triggerBeginOverlay, this, &Overlays::mousePressPointerEvent);
    connect(pointerManager.data(), &PointerManager::triggerContinueOverlay, this, &Overlays::mouseMovePointerEvent);
    connect(pointerManager.data(), &PointerManager::triggerEndOverlay, this, &Overlays::mouseReleasePointerEvent);
}

void Overlays::cleanupAllOverlays() {
    QMap<OverlayID, Overlay::Pointer> overlaysHUD;
    QMap<OverlayID, Overlay::Pointer> overlaysWorld;
    {
        QMutexLocker locker(&_mutex);
        overlaysHUD.swap(_overlaysHUD);
        overlaysWorld.swap(_overlaysWorld);
    }

    foreach(Overlay::Pointer overlay, overlaysHUD) {
        _overlaysToDelete.push_back(overlay);
    }
    foreach(Overlay::Pointer overlay, overlaysWorld) {
        _overlaysToDelete.push_back(overlay);
    }
#if OVERLAY_PANELS
    _panels.clear();
#endif
    cleanupOverlaysToDelete();
}

void Overlays::init() {
#if OVERLAY_PANELS
    _scriptEngine = new QScriptEngine();
#endif
}

void Overlays::update(float deltatime) {
    {
        QMutexLocker locker(&_mutex);
        foreach(const auto& thisOverlay, _overlaysHUD) {
            thisOverlay->update(deltatime);
        }
        foreach(const auto& thisOverlay, _overlaysWorld) {
            thisOverlay->update(deltatime);
        }
    }

    cleanupOverlaysToDelete();
}

void Overlays::cleanupOverlaysToDelete() {
    if (!_overlaysToDelete.isEmpty()) {
        render::ScenePointer scene = qApp->getMain3DScene();
        render::Transaction transaction;

        {
            do {
                Overlay::Pointer overlay = _overlaysToDelete.takeLast();

                auto itemID = overlay->getRenderItemID();
                if (render::Item::isValidID(itemID)) {
                    overlay->removeFromScene(overlay, scene, transaction);
                }
            } while (!_overlaysToDelete.isEmpty());
        }

        if (transaction.hasRemovedItems()) {
            scene->enqueueTransaction(transaction);
        }
    }
}

void Overlays::renderHUD(RenderArgs* renderArgs) {
    PROFILE_RANGE(render_overlays, __FUNCTION__);
    gpu::Batch& batch = *renderArgs->_batch;

    auto geometryCache = DependencyManager::get<GeometryCache>();
    auto textureCache = DependencyManager::get<TextureCache>();

    auto size = qApp->getUiSize();
    int width = size.x;
    int height = size.y;
    mat4 legacyProjection = glm::ortho<float>(0, width, height, 0, -1000, 1000);

    QMutexLocker locker(&_mutex);
    foreach(Overlay::Pointer thisOverlay, _overlaysHUD) {

        // Reset all batch pipeline settings between overlay
        geometryCache->useSimpleDrawPipeline(batch);
        batch.setResourceTexture(0, textureCache->getWhiteTexture()); // FIXME - do we really need to do this??
        batch.setProjectionTransform(legacyProjection);
        batch.setModelTransform(Transform());
        batch.resetViewTransform();

        thisOverlay->render(renderArgs);
    }
}

void Overlays::disable() {
    _enabled = false;
}

void Overlays::enable() {
    _enabled = true;
}

// Note, can't be invoked by scripts, but can be called by the InterfaceParentFinder
// class on packet processing threads
Overlay::Pointer Overlays::getOverlay(OverlayID id) const {
    QMutexLocker locker(&_mutex);
    if (_overlaysHUD.contains(id)) {
        return _overlaysHUD[id];
    } else if (_overlaysWorld.contains(id)) {
        return _overlaysWorld[id];
    }
    return nullptr;
}

OverlayID Overlays::addOverlay(const QString& type, const QVariant& properties) {
    if (QThread::currentThread() != thread()) {
        OverlayID result;
        PROFILE_RANGE(script, __FUNCTION__);
        BLOCKING_INVOKE_METHOD(this, "addOverlay", Q_RETURN_ARG(OverlayID, result), Q_ARG(QString, type), Q_ARG(QVariant, properties));
        return result;
    }

    Overlay::Pointer thisOverlay = nullptr;

    /**jsdoc
     * <p>An overlay may be one of the following types:</p>
     * <table>
     *   <thead>
     *     <tr><th>Value</th><th>2D/3D</th><th>Description</th></tr>
     *   </thead>
     *   <tbody>
     *     <tr><td><code>circle3d</code></td><td>3D</td><td>A circle.</td></tr>
     *     <tr><td><code>cube</code></td><td>3D</td><td>A cube. Can also use a <code>shape</code> overlay to create a 
     *     cube.</td></tr>
     *     <tr><td><code>grid</code></td><td>3D</td><td>A grid of lines in a plane.</td></tr>
     *     <tr><td><code>image</code></td><td>2D</td><td>An image. Synonym: <code>billboard</code>.</td></tr>
     *     <tr><td><code>image3d</code></td><td>3D</td><td>An image.</td></tr>
     *     <tr><td><code>line3d</code></td><td>3D</td><td>A line.</td></tr>
     *     <tr><td><code>model</code></td><td>3D</td><td>A model.</td></tr>
     *     <tr><td><code>rectangle</code></td><td>2D</td><td>A rectangle.</td></tr>
     *     <tr><td><code>rectangle3d</code></td><td>3D</td><td>A rectangle.</td></tr>
     *     <tr><td><code>shape</code></td><td>3D</td><td>A geometric shape, such as a cube, sphere, or cylinder.</td></tr>
     *     <tr><td><code>sphere</code></td><td>3D</td><td>A sphere. Can also use a <code>shape</code> overlay to create a 
     *     sphere.</td></tr>
     *     <tr><td><code>text</code></td><td>2D</td><td>Text.</td></tr>
     *     <tr><td><code>text3d</code></td><td>3D</td><td>Text.</td></tr>
     *     <tr><td><code>web3d</code></td><td>3D</td><td>Web content.</td></tr>
     *   </tbody>
     * </table>
     * <p>2D overlays are rendered on the display surface in desktop mode and on the HUD surface in HMD mode. 3D overlays are
     * rendered at a position and orientation in-world.<p>
     * <p>Each overlay type has different {@link Overlays.OverlayProperties|OverlayProperties}.</p>
     * @typedef {string} Overlays.OverlayType
     */

     /**jsdoc
     * <p>Different overlay types have different properties:</p>
     * <table>
     *   <thead>
     *     <tr><th>{@link Overlays.OverlayType|OverlayType}</th><th>Overlay Properties</th></tr>
     *   </thead>
     *   <tbody>
     *     <tr><td><code>circle3d</code></td><td>{@link Overlays.Circle3DProperties|Circle3DProperties}</td></tr>
     *     <tr><td><code>cube</code></td><td>{@link Overlays.CubeProperties|CubeProperties}</td></tr>
     *     <tr><td><code>grid</code></td><td>{@link Overlays.GridProperties|GridProperties}</td></tr>
     *     <tr><td><code>image</code></td><td>{@link Overlays.ImageProperties|ImageProperties}</td></tr>
     *     <tr><td><code>image3d</code></td><td>{@link Overlays.Image3DProperties|Image3DProperties}</td></tr>
     *     <tr><td><code>line3d</code></td><td>{@link Overlays.Line3DProperties|Line3DProperties}</td></tr>
     *     <tr><td><code>model</code></td><td>{@link Overlays.ModelProperties|ModelProperties}</td></tr>
     *     <tr><td><code>rectangle</code></td><td>{@link Overlays.RectangleProperties|RectangleProperties}</td></tr>
     *     <tr><td><code>rectangle3d</code></td><td>{@link Overlays.Rectangle3DProperties|Rectangle3DProperties}</td></tr>
     *     <tr><td><code>shape</code></td><td>{@link Overlays.ShapeProperties|ShapeProperties}</td></tr>
     *     <tr><td><code>sphere</code></td><td>{@link Overlays.SphereProperties|SphereProperties}</td></tr>
     *     <tr><td><code>text</code></td><td>{@link Overlays.TextProperties|TextProperties}</td></tr>
     *     <tr><td><code>text3d</code></td><td>{@link Overlays.Text3DProperties|Text3DProperties}</td></tr>
     *     <tr><td><code>web3d</code></td><td>{@link Overlays.Web3DProperties|Web3DProperties}</td></tr>
     *   </tbody>
     * </table>
     * @typedef {object} Overlays.OverlayProperties
     */

    if (type == ImageOverlay::TYPE) {
        thisOverlay = Overlay::Pointer(new ImageOverlay(), [](Overlay* ptr) { ptr->deleteLater(); });
    } else if (type == Image3DOverlay::TYPE || type == "billboard") { // "billboard" for backwards compatibility
        thisOverlay = Overlay::Pointer(new Image3DOverlay(), [](Overlay* ptr) { ptr->deleteLater(); });
    } else if (type == TextOverlay::TYPE) {
        thisOverlay = Overlay::Pointer(new TextOverlay(), [](Overlay* ptr) { ptr->deleteLater(); });
    } else if (type == Text3DOverlay::TYPE) {
        thisOverlay = Overlay::Pointer(new Text3DOverlay(), [](Overlay* ptr) { ptr->deleteLater(); });
    } else if (type == Shape3DOverlay::TYPE) {
        thisOverlay = Overlay::Pointer(new Shape3DOverlay(), [](Overlay* ptr) { ptr->deleteLater(); });
    } else if (type == Cube3DOverlay::TYPE) {
        thisOverlay = Overlay::Pointer(new Cube3DOverlay(), [](Overlay* ptr) { ptr->deleteLater(); });
    } else if (type == Sphere3DOverlay::TYPE) {
        thisOverlay = Overlay::Pointer(new Sphere3DOverlay(), [](Overlay* ptr) { ptr->deleteLater(); });
    } else if (type == Circle3DOverlay::TYPE) {
        thisOverlay = Overlay::Pointer(new Circle3DOverlay(), [](Overlay* ptr) { ptr->deleteLater(); });
    } else if (type == Rectangle3DOverlay::TYPE) {
        thisOverlay = Overlay::Pointer(new Rectangle3DOverlay(), [](Overlay* ptr) { ptr->deleteLater(); });
    } else if (type == Line3DOverlay::TYPE) {
        thisOverlay = Overlay::Pointer(new Line3DOverlay(), [](Overlay* ptr) { ptr->deleteLater(); });
    } else if (type == Grid3DOverlay::TYPE) {
        thisOverlay = Overlay::Pointer(new Grid3DOverlay(), [](Overlay* ptr) { ptr->deleteLater(); });
    } else if (type == ModelOverlay::TYPE) {
        thisOverlay = Overlay::Pointer(new ModelOverlay(), [](Overlay* ptr) { ptr->deleteLater(); });
    } else if (type == Web3DOverlay::TYPE) {
        thisOverlay = Overlay::Pointer(new Web3DOverlay(), [](Overlay* ptr) { ptr->deleteLater(); });
    } else if (type == RectangleOverlay::TYPE) {
        thisOverlay = Overlay::Pointer(new RectangleOverlay(), [](Overlay* ptr) { ptr->deleteLater(); });
    }

    if (thisOverlay) {
        thisOverlay->setProperties(properties.toMap());
        return addOverlay(thisOverlay);
    }
    return UNKNOWN_OVERLAY_ID;
}

OverlayID Overlays::addOverlay(const Overlay::Pointer& overlay) {
    OverlayID thisID = OverlayID(QUuid::createUuid());
    overlay->setOverlayID(thisID);
    overlay->setStackOrder(_stackOrder++);
    if (overlay->is3D()) {
        {
            QMutexLocker locker(&_mutex);
            _overlaysWorld[thisID] = overlay;
        }

        render::ScenePointer scene = qApp->getMain3DScene();
        render::Transaction transaction;
        overlay->addToScene(overlay, scene, transaction);
        scene->enqueueTransaction(transaction);
    } else {
        QMutexLocker locker(&_mutex);
        _overlaysHUD[thisID] = overlay;
    }

    return thisID;
}

OverlayID Overlays::cloneOverlay(OverlayID id) {
    if (QThread::currentThread() != thread()) {
        OverlayID result;
        PROFILE_RANGE(script, __FUNCTION__);
        BLOCKING_INVOKE_METHOD(this, "cloneOverlay", Q_RETURN_ARG(OverlayID, result), Q_ARG(OverlayID, id));
        return result;
    }

    Overlay::Pointer thisOverlay = getOverlay(id);

    if (thisOverlay) {
        OverlayID cloneId = addOverlay(Overlay::Pointer(thisOverlay->createClone(), [](Overlay* ptr) { ptr->deleteLater(); }));
#if OVERLAY_PANELS
        auto attachable = std::dynamic_pointer_cast<PanelAttachable>(thisOverlay);
        if (attachable && attachable->getParentPanel()) {
            attachable->getParentPanel()->addChild(cloneId);
        }
#endif
        return cloneId;
    }

    return UNKNOWN_OVERLAY_ID;  // Not found
}

bool Overlays::editOverlay(OverlayID id, const QVariant& properties) {
    auto thisOverlay = getOverlay(id);
    if (!thisOverlay) {
        return false;
    }

    if (!thisOverlay->is3D() && QThread::currentThread() != thread()) {
        // NOTE editOverlay can be called very frequently in scripts and can't afford to
        // block waiting on the main thread.  Additionally, no script actually
        // examines the return value and does something useful with it, so use a non-blocking
        // invoke and just always return true
        QMetaObject::invokeMethod(this, "editOverlay", Q_ARG(OverlayID, id), Q_ARG(QVariant, properties));
        return true;
    }

    thisOverlay->setProperties(properties.toMap());
    return true;
}

bool Overlays::editOverlays(const QVariant& propertiesById) {
    bool defer2DOverlays = QThread::currentThread() != thread();

    QVariantMap deferrred;
    const QVariantMap map = propertiesById.toMap();
    bool success = true;
    for (const auto& key : map.keys()) {
        OverlayID id = OverlayID(key);
        Overlay::Pointer thisOverlay = getOverlay(id);
        if (!thisOverlay) {
            success = false;
            continue;
        }

        const QVariant& properties = map[key];
        if (defer2DOverlays && !thisOverlay->is3D()) {
            deferrred[key] = properties;
            continue;
        }
        thisOverlay->setProperties(properties.toMap());
    }

    // For 2D/QML overlays, we need to perform the edit on the main thread
    if (defer2DOverlays && !deferrred.empty()) {
        // NOTE see comment on editOverlay for why this is not a blocking call
        QMetaObject::invokeMethod(this, "editOverlays", Q_ARG(QVariant, deferrred));
    }

    return success;
}

void Overlays::deleteOverlay(OverlayID id) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "deleteOverlay", Q_ARG(OverlayID, id));
        return;
    }

    Overlay::Pointer overlayToDelete;

    {
        QMutexLocker locker(&_mutex);
        if (_overlaysHUD.contains(id)) {
            overlayToDelete = _overlaysHUD.take(id);
        } else if (_overlaysWorld.contains(id)) {
            overlayToDelete = _overlaysWorld.take(id);
        } else {
            return;
        }
    }

#if OVERLAY_PANELS
    auto attachable = std::dynamic_pointer_cast<PanelAttachable>(overlayToDelete);
    if (attachable && attachable->getParentPanel()) {
        attachable->getParentPanel()->removeChild(id);
        attachable->setParentPanel(nullptr);
    }
#endif


    _overlaysToDelete.push_back(overlayToDelete);
    emit overlayDeleted(id);
}

QString Overlays::getOverlayType(OverlayID overlayId) {
    if (QThread::currentThread() != thread()) {
        QString result;
        PROFILE_RANGE(script, __FUNCTION__);
        BLOCKING_INVOKE_METHOD(this, "getOverlayType", Q_RETURN_ARG(QString, result), Q_ARG(OverlayID, overlayId));
        return result;
    }

    Overlay::Pointer overlay = getOverlay(overlayId);
    if (overlay) {
        return overlay->getType();
    }
    return "";
}

QObject* Overlays::getOverlayObject(OverlayID id) {
    if (QThread::currentThread() != thread()) {
        QObject* result;
        PROFILE_RANGE(script, __FUNCTION__);
        BLOCKING_INVOKE_METHOD(this, "getOverlayObject", Q_RETURN_ARG(QObject*, result), Q_ARG(OverlayID, id));
        return result;
    }

    Overlay::Pointer thisOverlay = getOverlay(id);
    if (thisOverlay) {
        return qobject_cast<QObject*>(&(*thisOverlay));
    }
    return nullptr;
}

#if OVERLAY_PANELS
OverlayID Overlays::getParentPanel(OverlayID childId) const {
    Overlay::Pointer overlay = getOverlay(childId);
    auto attachable = std::dynamic_pointer_cast<PanelAttachable>(overlay);
    if (attachable) {
        return _panels.key(attachable->getParentPanel());
    } else if (_panels.contains(childId)) {
        return _panels.key(getPanel(childId)->getParentPanel());
    }
    return UNKNOWN_OVERLAY_ID;
}

void Overlays::setParentPanel(OverlayID childId, OverlayID panelId) {
    auto attachable = std::dynamic_pointer_cast<PanelAttachable>(getOverlay(childId));
    if (attachable) {
        if (_panels.contains(panelId)) {
            auto panel = getPanel(panelId);
            panel->addChild(childId);
            attachable->setParentPanel(panel);
        } else {
            auto panel = attachable->getParentPanel();
            if (panel) {
                panel->removeChild(childId);
                attachable->setParentPanel(nullptr);
            }
        }
    } else if (_panels.contains(childId)) {
        OverlayPanel::Pointer child = getPanel(childId);
        if (_panels.contains(panelId)) {
            auto panel = getPanel(panelId);
            panel->addChild(childId);
            child->setParentPanel(panel);
        } else {
            auto panel = child->getParentPanel();
            if (panel) {
                panel->removeChild(childId);
                child->setParentPanel(0);
            }
        }
    }
}
#endif

OverlayID Overlays::getOverlayAtPoint(const glm::vec2& point) {
    if (!_enabled) {
        return UNKNOWN_OVERLAY_ID;
    }

    QMutexLocker locker(&_mutex);
    QMapIterator<OverlayID, Overlay::Pointer> i(_overlaysHUD);
    unsigned int bestStackOrder = 0;
    OverlayID bestOverlayID = UNKNOWN_OVERLAY_ID;
    while (i.hasNext()) {
        i.next();
        auto thisOverlay = std::dynamic_pointer_cast<Overlay2D>(i.value());
        if (thisOverlay && thisOverlay->getVisible() && thisOverlay->isLoaded() &&
            thisOverlay->getBoundingRect().contains(point.x, point.y, false)) {
            if (thisOverlay->getStackOrder() > bestStackOrder) {
                bestOverlayID = i.key();
                bestStackOrder = thisOverlay->getStackOrder();
            }
        }
    }

    return bestOverlayID;
}

OverlayPropertyResult Overlays::getProperty(OverlayID id, const QString& property) {
    Overlay::Pointer thisOverlay = getOverlay(id);
    OverlayPropertyResult result;
    if (thisOverlay && thisOverlay->supportsGetProperty()) {
        result.value = thisOverlay->getProperty(property);
    }
    return result;
}

OverlayPropertyResult Overlays::getProperties(const OverlayID& id, const QStringList& properties) {
    Overlay::Pointer thisOverlay = getOverlay(id);
    OverlayPropertyResult result;
    if (thisOverlay && thisOverlay->supportsGetProperty()) {
        QVariantMap mapResult;
        for (const auto& property : properties) {
            mapResult.insert(property, thisOverlay->getProperty(property));
        }
        result.value = mapResult;
    }
    return result;
}

OverlayPropertyResult Overlays::getOverlaysProperties(const QVariant& propertiesById) {
    QVariantMap map = propertiesById.toMap();
    OverlayPropertyResult result;
    QVariantMap resultMap;
    for (const auto& key : map.keys()) {
        OverlayID id = OverlayID(key);
        QVariantMap overlayResult;
        Overlay::Pointer thisOverlay = getOverlay(id);
        if (thisOverlay && thisOverlay->supportsGetProperty()) {
            QStringList propertiesToFetch = map[key].toStringList();
            for (const auto& property : propertiesToFetch) {
                overlayResult[property] = thisOverlay->getProperty(property);
            }
        }
        resultMap[key] = overlayResult;
    }
    result.value = resultMap;
    return result;
}

OverlayPropertyResult::OverlayPropertyResult() {
}

QScriptValue OverlayPropertyResultToScriptValue(QScriptEngine* engine, const OverlayPropertyResult& value) {
    if (!value.value.isValid()) {
        return QScriptValue::UndefinedValue;
    }
    return engine->toScriptValue(value.value);
}

void OverlayPropertyResultFromScriptValue(const QScriptValue& object, OverlayPropertyResult& value) {
    value.value = object.toVariant();
}


RayToOverlayIntersectionResult Overlays::findRayIntersection(const PickRay& ray, bool precisionPicking,
                                                             const QScriptValue& overlayIDsToInclude,
                                                             const QScriptValue& overlayIDsToDiscard,
                                                             bool visibleOnly, bool collidableOnly) {
    const QVector<OverlayID> overlaysToInclude = qVectorOverlayIDFromScriptValue(overlayIDsToInclude);
    const QVector<OverlayID> overlaysToDiscard = qVectorOverlayIDFromScriptValue(overlayIDsToDiscard);

    return findRayIntersectionVector(ray, precisionPicking,
                                     overlaysToInclude, overlaysToDiscard, visibleOnly, collidableOnly);
}


RayToOverlayIntersectionResult Overlays::findRayIntersectionVector(const PickRay& ray, bool precisionPicking,
                                                                   const QVector<OverlayID>& overlaysToInclude,
                                                                   const QVector<OverlayID>& overlaysToDiscard,
                                                                   bool visibleOnly, bool collidableOnly) {
    float bestDistance = std::numeric_limits<float>::max();
    bool bestIsFront = false;

    QMutexLocker locker(&_mutex);
    RayToOverlayIntersectionResult result;
    QMapIterator<OverlayID, Overlay::Pointer> i(_overlaysWorld);
    while (i.hasNext()) {
        i.next();
        OverlayID thisID = i.key();
        auto thisOverlay = std::dynamic_pointer_cast<Base3DOverlay>(i.value());

        if ((overlaysToDiscard.size() > 0 && overlaysToDiscard.contains(thisID)) ||
            (overlaysToInclude.size() > 0 && !overlaysToInclude.contains(thisID))) {
            continue;
        }

        if (thisOverlay && thisOverlay->getVisible() && !thisOverlay->getIgnoreRayIntersection() && thisOverlay->isLoaded()) {
            float thisDistance;
            BoxFace thisFace;
            glm::vec3 thisSurfaceNormal;
            QString thisExtraInfo;
            if (thisOverlay->findRayIntersectionExtraInfo(ray.origin, ray.direction, thisDistance,
                                                          thisFace, thisSurfaceNormal, thisExtraInfo)) {
                bool isDrawInFront = thisOverlay->getDrawInFront();
                if ((bestIsFront && isDrawInFront && thisDistance < bestDistance)
                    || (!bestIsFront && (isDrawInFront || thisDistance < bestDistance))) {

                    bestIsFront = isDrawInFront;
                    bestDistance = thisDistance;
                    result.intersects = true;
                    result.distance = thisDistance;
                    result.face = thisFace;
                    result.surfaceNormal = thisSurfaceNormal;
                    result.overlayID = thisID;
                    result.intersection = ray.origin + (ray.direction * thisDistance);
                    result.extraInfo = thisExtraInfo;
                }
            }
        }
    }
    return result;
}

QScriptValue RayToOverlayIntersectionResultToScriptValue(QScriptEngine* engine, const RayToOverlayIntersectionResult& value) {
    auto obj = engine->newObject();
    obj.setProperty("intersects", value.intersects);
    obj.setProperty("overlayID", OverlayIDtoScriptValue(engine, value.overlayID));
    obj.setProperty("distance", value.distance);

    QString faceName = "";
    // handle BoxFace
    switch (value.face) {
        case MIN_X_FACE:
            faceName = "MIN_X_FACE";
            break;
        case MAX_X_FACE:
            faceName = "MAX_X_FACE";
            break;
        case MIN_Y_FACE:
            faceName = "MIN_Y_FACE";
            break;
        case MAX_Y_FACE:
            faceName = "MAX_Y_FACE";
            break;
        case MIN_Z_FACE:
            faceName = "MIN_Z_FACE";
            break;
        case MAX_Z_FACE:
            faceName = "MAX_Z_FACE";
            break;
        default:
        case UNKNOWN_FACE:
            faceName = "UNKNOWN_FACE";
            break;
    }
    obj.setProperty("face", faceName);
    auto intersection = vec3toScriptValue(engine, value.intersection);
    obj.setProperty("intersection", intersection);
    obj.setProperty("extraInfo", value.extraInfo);
    return obj;
}

void RayToOverlayIntersectionResultFromScriptValue(const QScriptValue& objectVar, RayToOverlayIntersectionResult& value) {
    QVariantMap object = objectVar.toVariant().toMap();
    value.intersects = object["intersects"].toBool();
    value.overlayID = OverlayID(QUuid(object["overlayID"].toString()));
    value.distance = object["distance"].toFloat();

    QString faceName = object["face"].toString();
    if (faceName == "MIN_X_FACE") {
        value.face = MIN_X_FACE;
    } else if (faceName == "MAX_X_FACE") {
        value.face = MAX_X_FACE;
    } else if (faceName == "MIN_Y_FACE") {
        value.face = MIN_Y_FACE;
    } else if (faceName == "MAX_Y_FACE") {
        value.face = MAX_Y_FACE;
    } else if (faceName == "MIN_Z_FACE") {
        value.face = MIN_Z_FACE;
    } else if (faceName == "MAX_Z_FACE") {
        value.face = MAX_Z_FACE;
    } else {
        value.face = UNKNOWN_FACE;
    };
    auto intersection = object["intersection"];
    if (intersection.isValid()) {
        bool valid;
        auto newIntersection = vec3FromVariant(intersection, valid);
        if (valid) {
            value.intersection = newIntersection;
        }
    }
    value.extraInfo = object["extraInfo"].toString();
}

bool Overlays::isLoaded(OverlayID id) {
    if (QThread::currentThread() != thread()) {
        bool result;
        PROFILE_RANGE(script, __FUNCTION__);
        BLOCKING_INVOKE_METHOD(this, "isLoaded", Q_RETURN_ARG(bool, result), Q_ARG(OverlayID, id));
        return result;
    }

    Overlay::Pointer thisOverlay = getOverlay(id);
    if (!thisOverlay) {
        return false; // not found
    }
    return thisOverlay->isLoaded();
}

QSizeF Overlays::textSize(OverlayID id, const QString& text) {
    if (QThread::currentThread() != thread()) {
        QSizeF result;
        PROFILE_RANGE(script, __FUNCTION__);
        BLOCKING_INVOKE_METHOD(this, "textSize", Q_RETURN_ARG(QSizeF, result), Q_ARG(OverlayID, id), Q_ARG(QString, text));
        return result;
    }

    Overlay::Pointer thisOverlay = getOverlay(id);
    if (thisOverlay) {
        if (thisOverlay->is3D()) {
            if (auto text3dOverlay = std::dynamic_pointer_cast<Text3DOverlay>(thisOverlay)) {
                return text3dOverlay->textSize(text);
            }
        } else {
            if (auto textOverlay = std::dynamic_pointer_cast<TextOverlay>(thisOverlay)) {
                return textOverlay->textSize(text);
            }
        }
    }
    return QSizeF(0.0f, 0.0f);
}

#if OVERLAY_PANELS
OverlayID Overlays::addPanel(OverlayPanel::Pointer panel) {
    QWriteLocker lock(&_lock);

    OverlayID thisID = QUuid::createUuid();
    _panels[thisID] = panel;

    return thisID;
}

OverlayID Overlays::addPanel(const QVariant& properties) {
    OverlayPanel::Pointer panel = std::make_shared<OverlayPanel>();
    panel->init(_scriptEngine);
    panel->setProperties(properties.toMap());
    return addPanel(panel);
}

void Overlays::editPanel(OverlayID panelId, const QVariant& properties) {
    if (_panels.contains(panelId)) {
        _panels[panelId]->setProperties(properties.toMap());
    }
}

OverlayPropertyResult Overlays::getPanelProperty(OverlayID panelId, const QString& property) {
    OverlayPropertyResult result;
    if (_panels.contains(panelId)) {
        OverlayPanel::Pointer thisPanel = getPanel(panelId);
        QReadLocker lock(&_lock);
        result.value = thisPanel->getProperty(property);
    }
    return result;
}


void Overlays::deletePanel(OverlayID panelId) {
    OverlayPanel::Pointer panelToDelete;

    {
        QWriteLocker lock(&_lock);
        if (_panels.contains(panelId)) {
            panelToDelete = _panels.take(panelId);
        } else {
            return;
        }
    }

    while (!panelToDelete->getChildren().isEmpty()) {
        OverlayID childId = panelToDelete->popLastChild();
        deleteOverlay(childId);
        deletePanel(childId);
    }

    emit panelDeleted(panelId);
}
#endif

bool Overlays::isAddedOverlay(OverlayID id) {
    if (QThread::currentThread() != thread()) {
        bool result;
        PROFILE_RANGE(script, __FUNCTION__);
        BLOCKING_INVOKE_METHOD(this, "isAddedOverlay", Q_RETURN_ARG(bool, result), Q_ARG(OverlayID, id));
        return result;
    }

    QMutexLocker locker(&_mutex);
    return _overlaysHUD.contains(id) || _overlaysWorld.contains(id);
}

void Overlays::sendMousePressOnOverlay(const OverlayID& overlayID, const PointerEvent& event) {
    mousePressPointerEvent(overlayID, event);
}

void Overlays::sendMouseReleaseOnOverlay(const OverlayID& overlayID, const PointerEvent& event) {
    mouseReleasePointerEvent(overlayID, event);
}

void Overlays::sendMouseMoveOnOverlay(const OverlayID& overlayID, const PointerEvent& event) {
    mouseMovePointerEvent(overlayID, event);
}

void Overlays::sendHoverEnterOverlay(const OverlayID& overlayID, const PointerEvent& event) {
    hoverEnterPointerEvent(overlayID, event);
}

void Overlays::sendHoverOverOverlay(const OverlayID& overlayID, const PointerEvent& event) {
    hoverOverPointerEvent(overlayID, event);
}

void Overlays::sendHoverLeaveOverlay(const OverlayID& overlayID, const PointerEvent& event) {
    hoverLeavePointerEvent(overlayID, event);
}

OverlayID Overlays::getKeyboardFocusOverlay() {
    return qApp->getKeyboardFocusOverlay();
}

void Overlays::setKeyboardFocusOverlay(const OverlayID& id) {
    qApp->setKeyboardFocusOverlay(id);
}

float Overlays::width() {
    if (QThread::currentThread() != thread()) {
        float result;
        PROFILE_RANGE(script, __FUNCTION__);
        BLOCKING_INVOKE_METHOD(this, "width", Q_RETURN_ARG(float, result));
        return result;
    }

    auto offscreenUi = DependencyManager::get<OffscreenUi>();
    return offscreenUi->getWindow()->size().width();
}

float Overlays::height() {
    if (QThread::currentThread() != thread()) {
        float result;
        PROFILE_RANGE(script, __FUNCTION__);
        BLOCKING_INVOKE_METHOD(this, "height", Q_RETURN_ARG(float, result));
        return result;
    }

    auto offscreenUi = DependencyManager::get<OffscreenUi>();
    return offscreenUi->getWindow()->size().height();
}

static glm::vec2 projectOntoOverlayXYPlane(glm::vec3 position, glm::quat rotation, glm::vec2 dimensions, const PickRay& pickRay,
    const RayToOverlayIntersectionResult& rayPickResult) {

    // Project the intersection point onto the local xy plane of the overlay.
    float distance;
    glm::vec3 planePosition = position;
    glm::vec3 planeNormal = rotation * Vectors::UNIT_Z;
    glm::vec3 overlayDimensions = glm::vec3(dimensions.x, dimensions.y, 0.0f);
    glm::vec3 rayDirection = pickRay.direction;
    glm::vec3 rayStart = pickRay.origin;
    glm::vec3 p;
    if (rayPlaneIntersection(planePosition, planeNormal, rayStart, rayDirection, distance)) {
        p = rayStart + rayDirection * distance;
    } else {
        p = rayPickResult.intersection;
    }
    glm::vec3 localP = glm::inverse(rotation) * (p - position);
    glm::vec3 normalizedP = (localP / overlayDimensions) + glm::vec3(0.5f);
    return glm::vec2(normalizedP.x * overlayDimensions.x,
        (1.0f - normalizedP.y) * overlayDimensions.y);  // flip y-axis
}

static uint32_t toPointerButtons(const QMouseEvent& event) {
    uint32_t buttons = 0;
    buttons |= event.buttons().testFlag(Qt::LeftButton) ? PointerEvent::PrimaryButton : 0;
    buttons |= event.buttons().testFlag(Qt::RightButton) ? PointerEvent::SecondaryButton : 0;
    buttons |= event.buttons().testFlag(Qt::MiddleButton) ? PointerEvent::TertiaryButton : 0;
    return buttons;
}

static PointerEvent::Button toPointerButton(const QMouseEvent& event) {
    switch (event.button()) {
        case Qt::LeftButton:
            return PointerEvent::PrimaryButton;
        case Qt::RightButton:
            return PointerEvent::SecondaryButton;
        case Qt::MiddleButton:
            return PointerEvent::TertiaryButton;
        default:
            return PointerEvent::NoButtons;
    }
}

PointerEvent Overlays::calculateOverlayPointerEvent(OverlayID overlayID, PickRay ray,
                                             RayToOverlayIntersectionResult rayPickResult, QMouseEvent* event,
                                             PointerEvent::EventType eventType) {
    auto overlay = std::dynamic_pointer_cast<Planar3DOverlay>(getOverlay(overlayID));
    if (getOverlayType(overlayID) == "web3d") {
        overlay = std::dynamic_pointer_cast<Web3DOverlay>(getOverlay(overlayID));
    }
    if (!overlay) {
        return PointerEvent();
    }
    glm::vec3 position = overlay->getWorldPosition();
    glm::quat rotation = overlay->getWorldOrientation();
    glm::vec2 dimensions = overlay->getSize();


    glm::vec2 pos2D = projectOntoOverlayXYPlane(position, rotation, dimensions, ray, rayPickResult);

    PointerEvent pointerEvent(eventType, PointerManager::MOUSE_POINTER_ID, pos2D, rayPickResult.intersection, rayPickResult.surfaceNormal,
                              ray.direction, toPointerButton(*event), toPointerButtons(*event), event->modifiers());

    return pointerEvent;
}


RayToOverlayIntersectionResult Overlays::findRayIntersectionForMouseEvent(PickRay ray) {
    QVector<OverlayID> overlaysToInclude;
    QVector<OverlayID> overlaysToDiscard;
    RayToOverlayIntersectionResult rayPickResult;

    // first priority is tablet screen
    overlaysToInclude << qApp->getTabletScreenID();
    rayPickResult = findRayIntersectionVector(ray, true, overlaysToInclude, overlaysToDiscard);
    if (rayPickResult.intersects) {
        return rayPickResult;
    }
    // then tablet home button
    overlaysToInclude.clear();
    overlaysToInclude << qApp->getTabletHomeButtonID();
    rayPickResult = findRayIntersectionVector(ray, true, overlaysToInclude, overlaysToDiscard);
    if (rayPickResult.intersects) {
        return rayPickResult;
    }
    // then tablet frame
    overlaysToInclude.clear();
    overlaysToInclude << OverlayID(qApp->getTabletFrameID());
    rayPickResult = findRayIntersectionVector(ray, true, overlaysToInclude, overlaysToDiscard);
    if (rayPickResult.intersects) {
        return rayPickResult;
    }
    // then whatever
    return findRayIntersection(ray);
}

bool Overlays::mousePressEvent(QMouseEvent* event) {
    PerformanceTimer perfTimer("Overlays::mousePressEvent");

    PickRay ray = qApp->computePickRay(event->x(), event->y());
    RayToOverlayIntersectionResult rayPickResult = findRayIntersectionForMouseEvent(ray);
    if (rayPickResult.intersects) {
        _currentClickingOnOverlayID = rayPickResult.overlayID;

        PointerEvent pointerEvent = calculateOverlayPointerEvent(_currentClickingOnOverlayID, ray, rayPickResult, event, PointerEvent::Press);
        mousePressPointerEvent(_currentClickingOnOverlayID, pointerEvent);
        return true;
    }
    // if we didn't press on an overlay, disable overlay keyboard focus
    setKeyboardFocusOverlay(UNKNOWN_OVERLAY_ID);
    // emit to scripts
    emit mousePressOffOverlay();
    return false;
}

void Overlays::mousePressPointerEvent(const OverlayID& overlayID, const PointerEvent& event) {
    // TODO: generalize this to allow any overlay to recieve events
    std::shared_ptr<Web3DOverlay> thisOverlay;
    if (getOverlayType(overlayID) == "web3d") {
        thisOverlay = std::static_pointer_cast<Web3DOverlay>(getOverlay(overlayID));
    }
    if (thisOverlay) {
        if (event.shouldFocus()) {
            // Focus keyboard on web overlays
            DependencyManager::get<EntityScriptingInterface>()->setKeyboardFocusEntity(UNKNOWN_ENTITY_ID);
            setKeyboardFocusOverlay(overlayID);
        }

        // Send to web overlay
        QMetaObject::invokeMethod(thisOverlay.get(), "handlePointerEvent", Q_ARG(PointerEvent, event));
    }

    // emit to scripts
    emit mousePressOnOverlay(overlayID, event);
}

bool Overlays::mouseDoublePressEvent(QMouseEvent* event) {
    PerformanceTimer perfTimer("Overlays::mouseDoublePressEvent");

    PickRay ray = qApp->computePickRay(event->x(), event->y());
    RayToOverlayIntersectionResult rayPickResult = findRayIntersectionForMouseEvent(ray);
    if (rayPickResult.intersects) {
        _currentClickingOnOverlayID = rayPickResult.overlayID;

        auto pointerEvent = calculateOverlayPointerEvent(_currentClickingOnOverlayID, ray, rayPickResult, event, PointerEvent::Press);
        // emit to scripts
        emit mouseDoublePressOnOverlay(_currentClickingOnOverlayID, pointerEvent);
        return true;
    }
    // emit to scripts
    emit mouseDoublePressOffOverlay();
    return false;
}

void Overlays::hoverEnterPointerEvent(const OverlayID& overlayID, const PointerEvent& event) {
    // TODO: generalize this to allow any overlay to recieve events
    std::shared_ptr<Web3DOverlay> thisOverlay;
    if (getOverlayType(overlayID) == "web3d") {
        thisOverlay = std::static_pointer_cast<Web3DOverlay>(getOverlay(overlayID));
    }
    if (thisOverlay) {
        // Send to web overlay
        QMetaObject::invokeMethod(thisOverlay.get(), "hoverEnterOverlay", Q_ARG(PointerEvent, event));
    }

    // emit to scripts
    emit hoverEnterOverlay(overlayID, event);
}

void Overlays::hoverOverPointerEvent(const OverlayID& overlayID, const PointerEvent& event) {
    // TODO: generalize this to allow any overlay to recieve events
    std::shared_ptr<Web3DOverlay> thisOverlay;
    if (getOverlayType(overlayID) == "web3d") {
        thisOverlay = std::static_pointer_cast<Web3DOverlay>(getOverlay(overlayID));
    }
    if (thisOverlay) {
        // Send to web overlay
        QMetaObject::invokeMethod(thisOverlay.get(), "handlePointerEvent", Q_ARG(PointerEvent, event));
    }

    // emit to scripts
    emit hoverOverOverlay(overlayID, event);
}

void Overlays::hoverLeavePointerEvent(const OverlayID& overlayID, const PointerEvent& event) {
    // TODO: generalize this to allow any overlay to recieve events
    std::shared_ptr<Web3DOverlay> thisOverlay;
    if (getOverlayType(overlayID) == "web3d") {
        thisOverlay = std::static_pointer_cast<Web3DOverlay>(getOverlay(overlayID));
    }
    if (thisOverlay) {
        // Send to web overlay
        QMetaObject::invokeMethod(thisOverlay.get(), "hoverLeaveOverlay", Q_ARG(PointerEvent, event));
    }

    // emit to scripts
    emit hoverLeaveOverlay(overlayID, event);
}

bool Overlays::mouseReleaseEvent(QMouseEvent* event) {
    PerformanceTimer perfTimer("Overlays::mouseReleaseEvent");

    PickRay ray = qApp->computePickRay(event->x(), event->y());
    RayToOverlayIntersectionResult rayPickResult = findRayIntersectionForMouseEvent(ray);
    if (rayPickResult.intersects) {
        auto pointerEvent = calculateOverlayPointerEvent(rayPickResult.overlayID, ray, rayPickResult, event, PointerEvent::Release);
        mouseReleasePointerEvent(rayPickResult.overlayID, pointerEvent);
    }

    _currentClickingOnOverlayID = UNKNOWN_OVERLAY_ID;
    return false;
}

void Overlays::mouseReleasePointerEvent(const OverlayID& overlayID, const PointerEvent& event) {
    // TODO: generalize this to allow any overlay to recieve events
    std::shared_ptr<Web3DOverlay> thisOverlay;
    if (getOverlayType(overlayID) == "web3d") {
        thisOverlay = std::static_pointer_cast<Web3DOverlay>(getOverlay(overlayID));
    }
    if (thisOverlay) {
        // Send to web overlay
        QMetaObject::invokeMethod(thisOverlay.get(), "handlePointerEvent", Q_ARG(PointerEvent, event));
    }

    // emit to scripts
    emit mouseReleaseOnOverlay(overlayID, event);
}

bool Overlays::mouseMoveEvent(QMouseEvent* event) {
    PerformanceTimer perfTimer("Overlays::mouseMoveEvent");

    PickRay ray = qApp->computePickRay(event->x(), event->y());
    RayToOverlayIntersectionResult rayPickResult = findRayIntersectionForMouseEvent(ray);
    if (rayPickResult.intersects) {
        auto pointerEvent = calculateOverlayPointerEvent(rayPickResult.overlayID, ray, rayPickResult, event, PointerEvent::Move);
        mouseMovePointerEvent(rayPickResult.overlayID, pointerEvent);

        // If previously hovering over a different overlay then leave hover on that overlay.
        if (_currentHoverOverOverlayID != UNKNOWN_OVERLAY_ID && rayPickResult.overlayID != _currentHoverOverOverlayID) {
            auto pointerEvent = calculateOverlayPointerEvent(_currentHoverOverOverlayID, ray, rayPickResult, event, PointerEvent::Move);
            hoverLeavePointerEvent(_currentHoverOverOverlayID, pointerEvent);
        }

        // If hovering over a new overlay then enter hover on that overlay.
        if (rayPickResult.overlayID != _currentHoverOverOverlayID) {
            hoverEnterPointerEvent(rayPickResult.overlayID, pointerEvent);
        }

        // Hover over current overlay.
        hoverOverPointerEvent(rayPickResult.overlayID, pointerEvent);

        _currentHoverOverOverlayID = rayPickResult.overlayID;
    } else {
        // If previously hovering an overlay then leave hover.
        if (_currentHoverOverOverlayID != UNKNOWN_OVERLAY_ID) {
            auto pointerEvent = calculateOverlayPointerEvent(_currentHoverOverOverlayID, ray, rayPickResult, event, PointerEvent::Move);
            hoverLeavePointerEvent(_currentHoverOverOverlayID, pointerEvent);

            _currentHoverOverOverlayID = UNKNOWN_OVERLAY_ID;
        }
    }
    return false;
}

void Overlays::mouseMovePointerEvent(const OverlayID& overlayID, const PointerEvent& event) {
    // TODO: generalize this to allow any overlay to recieve events
    std::shared_ptr<Web3DOverlay> thisOverlay;
    if (getOverlayType(overlayID) == "web3d") {
        thisOverlay = std::static_pointer_cast<Web3DOverlay>(getOverlay(overlayID));
    }
    if (thisOverlay) {
        // Send to web overlay
        QMetaObject::invokeMethod(thisOverlay.get(), "handlePointerEvent", Q_ARG(PointerEvent, event));
    }

    // emit to scripts
    emit mouseMoveOnOverlay(overlayID, event);
}

QVector<QUuid> Overlays::findOverlays(const glm::vec3& center, float radius) {
    QVector<QUuid> result;
    //if (QThread::currentThread() != thread()) {
    //    PROFILE_RANGE(script, __FUNCTION__);
    //    BLOCKING_INVOKE_METHOD(this, "findOverlays", Q_RETURN_ARG(QVector<QUuid>, result), Q_ARG(glm::vec3, center), Q_ARG(float, radius));
    //    return result;
    //}

    QMutexLocker locker(&_mutex);
    QMapIterator<OverlayID, Overlay::Pointer> i(_overlaysWorld);
    int checked = 0;
    while (i.hasNext()) {
        checked++;
        i.next();
        OverlayID thisID = i.key();
        auto overlay = std::dynamic_pointer_cast<Volume3DOverlay>(i.value());
        if (overlay && overlay->getVisible() && !overlay->getIgnoreRayIntersection() && overlay->isLoaded()) {
            // get AABox in frame of overlay
            glm::vec3 dimensions = overlay->getDimensions();
            glm::vec3 low = dimensions * -0.5f;
            AABox overlayFrameBox(low, dimensions);

            Transform overlayToWorldMatrix = overlay->getTransform();
            overlayToWorldMatrix.setScale(1.0f);  // ignore inherited scale factor from parents
            glm::mat4 worldToOverlayMatrix = glm::inverse(overlayToWorldMatrix.getMatrix());
            glm::vec3 overlayFrameSearchPosition = glm::vec3(worldToOverlayMatrix * glm::vec4(center, 1.0f));
            glm::vec3 penetration;
            if (overlayFrameBox.findSpherePenetration(overlayFrameSearchPosition, radius, penetration)) {
                result.append(thisID);
            }
        }
    }

    return result;
}
