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

#include "ImageOverlay.h"
#include "TextOverlay.h"
#include "RectangleOverlay.h"

#include <raypick/RayPick.h>

#include <RenderableWebEntityItem.h>

#include "ui/Keyboard.h"
#include <QtQuick/QQuickWindow>

#include <PointerManager.h>

Q_LOGGING_CATEGORY(trace_render_overlays, "trace.render.overlays")

std::unordered_map<QString, QString> Overlays::_entityToOverlayTypes;
std::unordered_map<QString, QString> Overlays::_overlayToEntityTypes;

Overlays::Overlays() {
    auto pointerManager = DependencyManager::get<PointerManager>();
    connect(pointerManager.data(), &PointerManager::hoverBeginOverlay, this, &Overlays::hoverEnterPointerEvent);
    connect(pointerManager.data(), &PointerManager::hoverContinueOverlay, this, &Overlays::hoverOverPointerEvent);
    connect(pointerManager.data(), &PointerManager::hoverEndOverlay, this, &Overlays::hoverLeavePointerEvent);
    connect(pointerManager.data(), &PointerManager::triggerBeginOverlay, this, &Overlays::mousePressPointerEvent);
    connect(pointerManager.data(), &PointerManager::triggerContinueOverlay, this, &Overlays::mouseMovePointerEvent);
    connect(pointerManager.data(), &PointerManager::triggerEndOverlay, this, &Overlays::mouseReleasePointerEvent);

    ADD_TYPE_MAP(Box, cube);
    ADD_TYPE_MAP(Sphere, sphere);
    ADD_TYPE_MAP(Shape, shape);
    ADD_TYPE_MAP(Model, model);
    ADD_TYPE_MAP(Text, text3d);
    ADD_TYPE_MAP(Image, image3d);
    ADD_TYPE_MAP(Web, web3d);
    ADD_TYPE_MAP(PolyLine, line3d);
    ADD_TYPE_MAP(Grid, grid);
    ADD_TYPE_MAP(Gizmo, circle3d);
}

void Overlays::cleanupAllOverlays() {
    _shuttingDown = true;
    QMap<QUuid, Overlay::Pointer> overlays;
    {
        QMutexLocker locker(&_mutex);
        overlays.swap(_overlays);
    }

    foreach(Overlay::Pointer overlay, overlays) {
        _overlaysToDelete.push_back(overlay);
    }
    cleanupOverlaysToDelete();
}

void Overlays::init() {
}

void Overlays::update(float deltatime) {
    {
        QMutexLocker locker(&_mutex);
        foreach(const auto& thisOverlay, _overlays) {
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

void Overlays::render(RenderArgs* renderArgs) {
    PROFILE_RANGE(render_overlays, __FUNCTION__);
    gpu::Batch& batch = *renderArgs->_batch;

    auto geometryCache = DependencyManager::get<GeometryCache>();
    auto textureCache = DependencyManager::get<TextureCache>();

    auto size = glm::uvec2(qApp->getUiSize());
    int width = size.x;
    int height = size.y;
    mat4 legacyProjection = glm::ortho<float>(0, width, height, 0, -1000, 1000);

    QMutexLocker locker(&_mutex);
    foreach(Overlay::Pointer thisOverlay, _overlays) {

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
Overlay::Pointer Overlays::get2DOverlay(const QUuid& id) const {
    if (_shuttingDown) {
        return nullptr;
    }

    QMutexLocker locker(&_mutex);
    auto overlayIter = _overlays.find(id);
    if (overlayIter != _overlays.end()) {
        return overlayIter.value();
    }
    return nullptr;
}

QString Overlays::entityToOverlayType(const QString& type) {
    auto iter = _entityToOverlayTypes.find(type);
    if (iter != _entityToOverlayTypes.end()) {
        return iter->second;
    }
    return "unknown";
}

QString Overlays::overlayToEntityType(const QString& type) {
    auto iter = _overlayToEntityTypes.find(type);
    if (iter != _overlayToEntityTypes.end()) {
        return iter->second;
    } else if (type == "billboard") {
        return "Image";
    }
    return "Unknown";
}

EntityItemProperties convertOverlayToEntityProperties(const QVariantMap& overlayProps) {
    EntityItemProperties props;

    return props;
}

QVariantMap convertEntityToOverlayProperties(const EntityItemProperties& entityProps) {
    QVariantMap props;

    return props;
}

QUuid Overlays::addOverlay(const QString& type, const QVariant& properties) {
    if (_shuttingDown) {
        return UNKNOWN_ENTITY_ID;
    }

    if (QThread::currentThread() != thread()) {
        QUuid result;
        PROFILE_RANGE(script, __FUNCTION__);
        BLOCKING_INVOKE_METHOD(this, "addOverlay", Q_RETURN_ARG(QUuid, result), Q_ARG(const QString&, type), Q_ARG(const QVariant&, properties));
        return result;
    }

    /**jsdoc
     * <p>An overlay may be one of the following types:</p>
     * <table>
     *   <thead>
     *     <tr><th>Value</th><th>2D/3D</th><th>Description</th></tr>
     *   </thead>
     *   <tbody>
     *     <tr><td><code>image</code></td><td>2D</td><td>An image. Synonym: <code>billboard</code>.</td></tr>
     *     <tr><td><code>rectangle</code></td><td>2D</td><td>A rectangle.</td></tr>
     *     <tr><td><code>text</code></td><td>2D</td><td>Text.</td></tr>
     *     <tr><td><code>circle3d</code></td><td>3D</td><td>A circle.</td></tr>
     *     <tr><td><code>cube</code></td><td>3D</td><td>A cube. Can also use a <code>shape</code> overlay to create a 
     *     cube.</td></tr>
     *     <tr><td><code>grid</code></td><td>3D</td><td>A grid of lines in a plane.</td></tr>
     *     <tr><td><code>image3d</code></td><td>3D</td><td>An image.</td></tr>
     *     <tr><td><code>line3d</code></td><td>3D</td><td>A line.</td></tr>
     *     <tr><td><code>model</code></td><td>3D</td><td>A model.</td></tr>
     *     <tr><td><code>rectangle3d</code></td><td>3D</td><td>A rectangle.</td></tr>
     *     <tr><td><code>shape</code></td><td>3D</td><td>A geometric shape, such as a cube, sphere, or cylinder.</td></tr>
     *     <tr><td><code>sphere</code></td><td>3D</td><td>A sphere. Can also use a <code>shape</code> overlay to create a 
     *     sphere.</td></tr>
     *     <tr><td><code>text3d</code></td><td>3D</td><td>Text.</td></tr>
     *     <tr><td><code>web3d</code></td><td>3D</td><td>Web content.</td></tr>
     *   </tbody>
     * </table>
     * <p>2D overlays are rendered on the display surface in desktop mode and on the HUD surface in HMD mode. 3D overlays are
     * rendered at a position and orientation in-world, but are deprecated (use local entities instead).<p>
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
     *     <tr><td><code>image</code></td><td>{@link Overlays.ImageProperties|ImageProperties}</td></tr>
     *     <tr><td><code>rectangle</code></td><td>{@link Overlays.RectangleProperties|RectangleProperties}</td></tr>
     *     <tr><td><code>text</code></td><td>{@link Overlays.TextProperties|TextProperties}</td></tr>
     *     <tr><td><code>circle3d</code></td><td>{@link Overlays.Circle3DProperties|Circle3DProperties}</td></tr>
     *     <tr><td><code>cube</code></td><td>{@link Overlays.CubeProperties|CubeProperties}</td></tr>
     *     <tr><td><code>grid</code></td><td>{@link Overlays.GridProperties|GridProperties}</td></tr>
     *     <tr><td><code>image3d</code></td><td>{@link Overlays.Image3DProperties|Image3DProperties}</td></tr>
     *     <tr><td><code>line3d</code></td><td>{@link Overlays.Line3DProperties|Line3DProperties}</td></tr>
     *     <tr><td><code>model</code></td><td>{@link Overlays.ModelProperties|ModelProperties}</td></tr>
     *     <tr><td><code>rectangle3d</code></td><td>{@link Overlays.Rectangle3DProperties|Rectangle3DProperties}</td></tr>
     *     <tr><td><code>shape</code></td><td>{@link Overlays.ShapeProperties|ShapeProperties}</td></tr>
     *     <tr><td><code>sphere</code></td><td>{@link Overlays.SphereProperties|SphereProperties}</td></tr>
     *     <tr><td><code>text3d</code></td><td>{@link Overlays.Text3DProperties|Text3DProperties}</td></tr>
     *     <tr><td><code>web3d</code></td><td>{@link Overlays.Web3DProperties|Web3DProperties}</td></tr>
     *   </tbody>
     * </table>
     * @typedef {object} Overlays.OverlayProperties
     */

    Overlay::Pointer overlay;
    if (type == ImageOverlay::TYPE) {
#if !defined(DISABLE_QML)
        overlay = Overlay::Pointer(new ImageOverlay(), [](Overlay* ptr) { ptr->deleteLater(); });
#endif
    } else if (type == TextOverlay::TYPE) {
#if !defined(DISABLE_QML)
        overlay = Overlay::Pointer(new TextOverlay(), [](Overlay* ptr) { ptr->deleteLater(); });
#endif
    } else if (type == RectangleOverlay::TYPE) {
        overlay = Overlay::Pointer(new RectangleOverlay(), [](Overlay* ptr) { ptr->deleteLater(); });
    }

    if (overlay) {
        overlay->setProperties(properties.toMap());
        return add2DOverlay(overlay);
    }

    QString entityType = overlayToEntityType(type);

    if (entityType == "Unknown") {
        return UNKNOWN_ENTITY_ID;
    }

    QVariantMap propertyMap = properties.toMap();
    propertyMap["type"] = entityType;
    return DependencyManager::get<EntityScriptingInterface>()->addEntity(convertOverlayToEntityProperties(propertyMap), "local");
}

QUuid Overlays::add2DOverlay(const Overlay::Pointer& overlay) {
    if (_shuttingDown) {
        return UNKNOWN_ENTITY_ID;
    }

    QUuid thisID = QUuid::createUuid();
    overlay->setID(thisID);
    overlay->setStackOrder(_stackOrder++);
    {
        QMutexLocker locker(&_mutex);
        _overlays[thisID] = overlay;
    }

    return thisID;
}

QUuid Overlays::cloneOverlay(const QUuid& id) {
    if (_shuttingDown) {
        return UNKNOWN_ENTITY_ID;
    }

    if (QThread::currentThread() != thread()) {
        QUuid result;
        PROFILE_RANGE(script, __FUNCTION__);
        BLOCKING_INVOKE_METHOD(this, "cloneOverlay", Q_RETURN_ARG(QUuid, result), Q_ARG(const QUuid&, id));
        return result;
    }

    Overlay::Pointer overlay = get2DOverlay(id);
    if (overlay) {
        return add2DOverlay(Overlay::Pointer(overlay->createClone(), [](Overlay* ptr) { ptr->deleteLater(); }));
    }

    return DependencyManager::get<EntityScriptingInterface>()->cloneEntity(id);
}

bool Overlays::editOverlay(const QUuid& id, const QVariant& properties) {
    if (_shuttingDown) {
        return false;
    }

    auto overlay = get2DOverlay(id);
    if (overlay) {
        if (QThread::currentThread() != thread()) {
            // NOTE editOverlay can be called very frequently in scripts and can't afford to
            // block waiting on the main thread.  Additionally, no script actually
            // examines the return value and does something useful with it, so use a non-blocking
            // invoke and just always return true
            QMetaObject::invokeMethod(this, "editOverlay", Q_ARG(const QUuid&, id), Q_ARG(const QVariant&, properties));
            return true;
        }

        overlay->setProperties(properties.toMap());
        return true;
    }

    EntityItemProperties entityProperties = convertOverlayToEntityProperties(properties.toMap());
    return !DependencyManager::get<EntityScriptingInterface>()->editEntity(id, entityProperties).isNull();
}

bool Overlays::editOverlays(const QVariant& propertiesById) {
    if (_shuttingDown) {
        return false;
    }

    bool deferOverlays = QThread::currentThread() != thread();

    QVariantMap deferred;
    const QVariantMap map = propertiesById.toMap();
    for (const auto& key : map.keys()) {
        QUuid id = QUuid(key);
        const QVariant& properties = map[key];

        Overlay::Pointer overlay = get2DOverlay(id);
        if (overlay) {
            if (deferOverlays) {
                deferred[key] = properties;
                continue;
            }
            overlay->setProperties(properties.toMap());
        } else {
            EntityItemProperties entityProperties = convertOverlayToEntityProperties(properties.toMap());
            DependencyManager::get<EntityScriptingInterface>()->editEntity(id, entityProperties);
        }
    }

    // For 2D/QML overlays, we need to perform the edit on the main thread
    if (!deferred.empty()) {
        // NOTE see comment on editOverlay for why this is not a blocking call
        QMetaObject::invokeMethod(this, "editOverlays", Q_ARG(const QVariant&, deferred));
    }

    return true;
}

void Overlays::deleteOverlay(const QUuid& id) {
    if (_shuttingDown) {
        return;
    }

    Overlay::Pointer overlay = get2DOverlay(id);
    if (overlay) {
        _overlaysToDelete.push_back(overlay);
        emit overlayDeleted(id);
        return;
    }

    DependencyManager::get<EntityScriptingInterface>()->deleteEntity(id);
    emit overlayDeleted(id);
}

QString Overlays::getOverlayType(const QUuid& id) {
    if (_shuttingDown) {
        return "";
    }

    if (QThread::currentThread() != thread()) {
        QString result;
        PROFILE_RANGE(script, __FUNCTION__);
        BLOCKING_INVOKE_METHOD(this, "getOverlayType", Q_RETURN_ARG(QString, result), Q_ARG(const QUuid&, id));
        return result;
    }

    Overlay::Pointer overlay = get2DOverlay(id);
    if (overlay) {
        return overlay->getType();
    }

    return entityToOverlayType(DependencyManager::get<EntityScriptingInterface>()->getEntityType(id));
}

QObject* Overlays::getOverlayObject(const QUuid& id) {
    if (QThread::currentThread() != thread()) {
        QObject* result;
        PROFILE_RANGE(script, __FUNCTION__);
        BLOCKING_INVOKE_METHOD(this, "getOverlayObject", Q_RETURN_ARG(QObject*, result), Q_ARG(const QUuid&, id));
        return result;
    }

    Overlay::Pointer overlay = get2DOverlay(id);
    if (overlay) {
        return qobject_cast<QObject*>(&(*overlay));
    }

    return DependencyManager::get<EntityScriptingInterface>()->getEntityObject(id);
}

QUuid Overlays::getOverlayAtPoint(const glm::vec2& point) {
    if (_shuttingDown || !_enabled) {
        return UNKNOWN_ENTITY_ID;
    }

    QMutexLocker locker(&_mutex);
    QMapIterator<QUuid, Overlay::Pointer> i(_overlays);
    unsigned int bestStackOrder = 0;
    QUuid bestID = UNKNOWN_ENTITY_ID;
    while (i.hasNext()) {
        i.next();
        auto thisOverlay = std::dynamic_pointer_cast<Overlay2D>(i.value());
        if (thisOverlay && thisOverlay->getVisible() && thisOverlay->isLoaded() &&
            thisOverlay->getBoundingRect().contains(point.x, point.y, false)) {
            if (thisOverlay->getStackOrder() > bestStackOrder) {
                bestID = i.key();
                bestStackOrder = thisOverlay->getStackOrder();
            }
        }
    }

    return bestID;
}

QVariant Overlays::getProperty(const QUuid& id, const QString& property) {
    Overlay::Pointer overlay = get2DOverlay(id);
    if (overlay) {
        if (overlay->supportsGetProperty()) {
            return overlay->getProperty(property);
        }
        return QVariant();
    }

    QVariantMap overlayProperties = convertEntityToOverlayProperties(DependencyManager::get<EntityScriptingInterface>()->getEntityProperties(id));
    auto propIter = overlayProperties.find(property);
    if (propIter != overlayProperties.end()) {
        return propIter.value();
    }
    return QVariant();
}

QVariantMap Overlays::getProperties(const QUuid& id, const QStringList& properties) {
    Overlay::Pointer overlay = get2DOverlay(id);
    QVariantMap result;
    if (overlay) {
        if (overlay->supportsGetProperty()) {
            for (const auto& property : properties) {
                result.insert(property, overlay->getProperty(property));
            }
        }
        return result;
    }

    QVariantMap overlayProperties = convertEntityToOverlayProperties(DependencyManager::get<EntityScriptingInterface>()->getEntityProperties(id));
    for (const auto& property : properties) {
        auto propIter = overlayProperties.find(property);
        if (propIter != overlayProperties.end()) {
            result.insert(property, propIter.value());
        }
    }
    return result;
}

QVariantMap Overlays::getOverlaysProperties(const QVariant& propertiesById) {
    QVariantMap map = propertiesById.toMap();
    QVariantMap result;
    for (const auto& key : map.keys()) {
        result[key] = getProperties(QUuid(key), map[key].toStringList());
    }
    return result;
}

RayToOverlayIntersectionResult Overlays::findRayIntersection(const PickRay& ray, bool precisionPicking,
                                                             const QScriptValue& overlayIDsToInclude,
                                                             const QScriptValue& overlayIDsToDiscard,
                                                             bool visibleOnly, bool collidableOnly) {
    const QVector<EntityItemID> include = qVectorEntityItemIDFromScriptValue(overlayIDsToInclude);
    const QVector<EntityItemID> discard = qVectorEntityItemIDFromScriptValue(overlayIDsToDiscard);

    return findRayIntersectionVector(ray, precisionPicking, include, discard, visibleOnly, collidableOnly);
}

RayToOverlayIntersectionResult Overlays::findRayIntersectionVector(const PickRay& ray, bool precisionPicking,
                                                                   const QVector<EntityItemID>& include,
                                                                   const QVector<EntityItemID>& discard,
                                                                   bool visibleOnly, bool collidableOnly) {
    unsigned int searchFilter = PickFilter::getBitMask(PickFilter::FlagBit::LOCAL_ENTITIES);

    if (!precisionPicking) {
        searchFilter = searchFilter | PickFilter::getBitMask(PickFilter::FlagBit::COARSE);
    }

    if (visibleOnly) {
        searchFilter = searchFilter | PickFilter::getBitMask(PickFilter::FlagBit::VISIBLE);
    }

    if (collidableOnly) {
        searchFilter = searchFilter | PickFilter::getBitMask(PickFilter::FlagBit::COLLIDABLE);
    }
    auto result = DependencyManager::get<EntityScriptingInterface>()->evalRayIntersectionVector(ray, PickFilter(searchFilter), include, discard);

    RayToOverlayIntersectionResult overlayResult;
    overlayResult.overlayID = result.entityID;
    overlayResult.intersects = result.intersects;
    overlayResult.intersection = result.intersection;
    overlayResult.distance = result.distance;
    overlayResult.surfaceNormal = result.surfaceNormal;
    overlayResult.face = result.face;
    overlayResult.extraInfo = result.extraInfo;
    return overlayResult;
}

ParabolaToOverlayIntersectionResult Overlays::findParabolaIntersectionVector(const PickParabola& parabola, bool precisionPicking,
                                                                             const QVector<EntityItemID>& include,
                                                                             const QVector<EntityItemID>& discard,
                                                                             bool visibleOnly, bool collidableOnly) {
    unsigned int searchFilter = PickFilter::getBitMask(PickFilter::FlagBit::LOCAL_ENTITIES);

    if (!precisionPicking) {
        searchFilter = searchFilter | PickFilter::getBitMask(PickFilter::FlagBit::COARSE);
    }

    if (visibleOnly) {
        searchFilter = searchFilter | PickFilter::getBitMask(PickFilter::FlagBit::VISIBLE);
    }

    if (collidableOnly) {
        searchFilter = searchFilter | PickFilter::getBitMask(PickFilter::FlagBit::COLLIDABLE);
    }
    auto result = DependencyManager::get<EntityScriptingInterface>()->evalParabolaIntersectionVector(parabola, PickFilter(searchFilter), include, discard);

    ParabolaToOverlayIntersectionResult overlayResult;
    overlayResult.overlayID = result.entityID;
    overlayResult.intersects = result.intersects;
    overlayResult.intersection = result.intersection;
    overlayResult.parabolicDistance = result.parabolicDistance;
    overlayResult.surfaceNormal = result.surfaceNormal;
    overlayResult.face = result.face;
    overlayResult.extraInfo = result.extraInfo;
    return overlayResult;
}

QScriptValue RayToOverlayIntersectionResultToScriptValue(QScriptEngine* engine, const RayToOverlayIntersectionResult& value) {
    QScriptValue obj = engine->newObject();
    obj.setProperty("intersects", value.intersects);
    QScriptValue overlayIDValue = quuidToScriptValue(engine, value.overlayID);
    obj.setProperty("overlayID", overlayIDValue);
    obj.setProperty("distance", value.distance);
    obj.setProperty("face", boxFaceToString(value.face));

    QScriptValue intersection = vec3ToScriptValue(engine, value.intersection);
    obj.setProperty("intersection", intersection);
    QScriptValue surfaceNormal = vec3ToScriptValue(engine, value.surfaceNormal);
    obj.setProperty("surfaceNormal", surfaceNormal);
    obj.setProperty("extraInfo", engine->toScriptValue(value.extraInfo));
    return obj;
}

void RayToOverlayIntersectionResultFromScriptValue(const QScriptValue& object, RayToOverlayIntersectionResult& value) {
    value.intersects = object.property("intersects").toVariant().toBool();
    QScriptValue overlayIDValue = object.property("overlayID");
    quuidFromScriptValue(overlayIDValue, value.overlayID);
    value.distance = object.property("distance").toVariant().toFloat();
    value.face = boxFaceFromString(object.property("face").toVariant().toString());

    QScriptValue intersection = object.property("intersection");
    if (intersection.isValid()) {
        vec3FromScriptValue(intersection, value.intersection);
    }
    QScriptValue surfaceNormal = object.property("surfaceNormal");
    if (surfaceNormal.isValid()) {
        vec3FromScriptValue(surfaceNormal, value.surfaceNormal);
    }
    value.extraInfo = object.property("extraInfo").toVariant().toMap();
}

bool Overlays::isLoaded(const QUuid& id) {
    if (QThread::currentThread() != thread()) {
        bool result;
        PROFILE_RANGE(script, __FUNCTION__);
        BLOCKING_INVOKE_METHOD(this, "isLoaded", Q_RETURN_ARG(bool, result), Q_ARG(const QUuid&, id));
        return result;
    }

    Overlay::Pointer overlay = get2DOverlay(id);
    if (overlay) {
        return overlay->isLoaded();
    }

    return DependencyManager::get<EntityScriptingInterface>()->isLoaded(id);
}

QSizeF Overlays::textSize(const QUuid& id, const QString& text) {
    if (QThread::currentThread() != thread()) {
        QSizeF result;
        PROFILE_RANGE(script, __FUNCTION__);
        BLOCKING_INVOKE_METHOD(this, "textSize", Q_RETURN_ARG(QSizeF, result), Q_ARG(const QUuid&, id), Q_ARG(QString, text));
        return result;
    }

    Overlay::Pointer overlay = get2DOverlay(id);
    if (overlay) {
        if (auto textOverlay = std::dynamic_pointer_cast<TextOverlay>(overlay)) {
            return textOverlay->textSize(text);
        }
        return QSizeF(0.0f, 0.0f);
    } else {
        return DependencyManager::get<EntityScriptingInterface>()->textSize(id, text);
    }
}

bool Overlays::isAddedOverlay(const QUuid& id) {
    Overlay::Pointer overlay = get2DOverlay(id);
    if (overlay) {
        return true;
    }

    return DependencyManager::get<EntityScriptingInterface>()->isAddedEntity(id);
}

void Overlays::sendMousePressOnOverlay(const QUuid& id, const PointerEvent& event) {
    mousePressPointerEvent(id, event);
}

void Overlays::sendMouseReleaseOnOverlay(const QUuid& id, const PointerEvent& event) {
    mouseReleasePointerEvent(id, event);
}

void Overlays::sendMouseMoveOnOverlay(const QUuid& id, const PointerEvent& event) {
    mouseMovePointerEvent(id, event);
}

void Overlays::sendHoverEnterOverlay(const QUuid& id, const PointerEvent& event) {
    hoverEnterPointerEvent(id, event);
}

void Overlays::sendHoverOverOverlay(const QUuid& id, const PointerEvent& event) {
    hoverOverPointerEvent(id, event);
}

void Overlays::sendHoverLeaveOverlay(const QUuid& id, const PointerEvent& event) {
    hoverLeavePointerEvent(id, event);
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

PointerEvent Overlays::calculateOverlayPointerEvent(const QUuid& id, const PickRay& ray,
                                             const RayToOverlayIntersectionResult& rayPickResult, QMouseEvent* event,
                                             PointerEvent::EventType eventType) {
    glm::vec2 pos2D = RayPick::projectOntoEntityXYPlane(id, rayPickResult.intersection);
    PointerEvent pointerEvent(eventType, PointerManager::MOUSE_POINTER_ID, pos2D, rayPickResult.intersection, rayPickResult.surfaceNormal,
                              ray.direction, toPointerButton(*event), toPointerButtons(*event), event->modifiers());

    return pointerEvent;
}

bool Overlays::mousePressEvent(QMouseEvent* event) {
    PerformanceTimer perfTimer("Overlays::mousePressEvent");

    PickRay ray = qApp->computePickRay(event->x(), event->y());
    RayToOverlayIntersectionResult rayPickResult = findRayIntersectionVector(ray, true, QVector<EntityItemID>(), QVector<EntityItemID>());
    if (rayPickResult.intersects) {
        _currentClickingOnOverlayID = rayPickResult.overlayID;

        PointerEvent pointerEvent = calculateOverlayPointerEvent(_currentClickingOnOverlayID, ray, rayPickResult, event, PointerEvent::Press);
        mousePressPointerEvent(_currentClickingOnOverlayID, pointerEvent);
        return true;
    }
    // if we didn't press on an overlay, disable overlay keyboard focus
    setKeyboardFocusOverlay(UNKNOWN_ENTITY_ID);
    // emit to scripts
    emit mousePressOffOverlay();
    return false;
}

void Overlays::mousePressPointerEvent(const QUuid& id, const PointerEvent& event) {
    // TODO: generalize this to allow any object to recieve events
    auto renderable = qApp->getEntities()->renderableForEntityId(id);
    if (renderable) {
        auto web = std::dynamic_pointer_cast<render::entities::WebEntityRenderer>(renderable);
        if (web) {
            if (event.shouldFocus()) {
                // Focus keyboard on web overlays
                DependencyManager::get<EntityScriptingInterface>()->setKeyboardFocusEntity(UNKNOWN_ENTITY_ID);
                setKeyboardFocusOverlay(id);
            }

            web->handlePointerEvent(event);
        }
    }

    auto keyboard = DependencyManager::get<Keyboard>();
    // Do not send keyboard key event to scripts to prevent malignant scripts from gathering what you typed
    if (!keyboard->getKeysID().contains(id)) {
        // emit to scripts
        emit mousePressOnOverlay(id, event);
    }
}

bool Overlays::mouseDoublePressEvent(QMouseEvent* event) {
    PerformanceTimer perfTimer("Overlays::mouseDoublePressEvent");

    PickRay ray = qApp->computePickRay(event->x(), event->y());
    RayToOverlayIntersectionResult rayPickResult = findRayIntersectionVector(ray, true, QVector<EntityItemID>(), QVector<EntityItemID>());
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

void Overlays::hoverEnterPointerEvent(const QUuid& id, const PointerEvent& event) {
    // TODO: generalize this to allow any object to recieve events
    auto renderable = qApp->getEntities()->renderableForEntityId(id);
    if (renderable) {
        auto web = std::dynamic_pointer_cast<render::entities::WebEntityRenderer>(renderable);
        if (web) {
            web->hoverEnterEntity(event);
        }
    }

    auto keyboard = DependencyManager::get<Keyboard>();
    // Do not send keyboard key event to scripts to prevent malignant scripts from gathering what you typed
    if (!keyboard->getKeysID().contains(id)) {
        // emit to scripts
        emit hoverEnterOverlay(id, event);
    }
}

void Overlays::hoverOverPointerEvent(const QUuid& id, const PointerEvent& event) {
    // TODO: generalize this to allow any overlay to recieve events
    auto renderable = qApp->getEntities()->renderableForEntityId(id);
    if (renderable) {
        auto web = std::dynamic_pointer_cast<render::entities::WebEntityRenderer>(renderable);
        if (web) {
            web->handlePointerEvent(event);
        }
    }

    auto keyboard = DependencyManager::get<Keyboard>();
    // Do not send keyboard key event to scripts to prevent malignant scripts from gathering what you typed
    if (!keyboard->getKeysID().contains(id)) {
        // emit to scripts
        emit hoverOverOverlay(id, event);
    }
}

void Overlays::hoverLeavePointerEvent(const QUuid& id, const PointerEvent& event) {
    // TODO: generalize this to allow any overlay to recieve events
    auto renderable = qApp->getEntities()->renderableForEntityId(id);
    if (renderable) {
        auto web = std::dynamic_pointer_cast<render::entities::WebEntityRenderer>(renderable);
        if (web) {
            web->hoverLeaveEntity(event);
        }
    }

    auto keyboard = DependencyManager::get<Keyboard>();
    // Do not send keyboard key event to scripts to prevent malignant scripts from gathering what you typed
    if (!keyboard->getKeysID().contains(id)) {
        // emit to scripts
        emit hoverLeaveOverlay(id, event);
    }
}

bool Overlays::mouseReleaseEvent(QMouseEvent* event) {
    PerformanceTimer perfTimer("Overlays::mouseReleaseEvent");

    PickRay ray = qApp->computePickRay(event->x(), event->y());
    RayToOverlayIntersectionResult rayPickResult = findRayIntersectionVector(ray, true, QVector<EntityItemID>(), QVector<EntityItemID>());
    if (rayPickResult.intersects) {
        auto pointerEvent = calculateOverlayPointerEvent(rayPickResult.overlayID, ray, rayPickResult, event, PointerEvent::Release);
        mouseReleasePointerEvent(rayPickResult.overlayID, pointerEvent);
    }

    _currentClickingOnOverlayID = UNKNOWN_ENTITY_ID;
    return false;
}

void Overlays::mouseReleasePointerEvent(const QUuid& id, const PointerEvent& event) {
    // TODO: generalize this to allow any overlay to recieve events
    auto renderable = qApp->getEntities()->renderableForEntityId(id);
    if (renderable) {
        auto web = std::dynamic_pointer_cast<render::entities::WebEntityRenderer>(renderable);
        if (web) {
            web->handlePointerEvent(event);
        }
    }

    auto keyboard = DependencyManager::get<Keyboard>();
    // Do not send keyboard key event to scripts to prevent malignant scripts from gathering what you typed
    if (!keyboard->getKeysID().contains(id)) {
        // emit to scripts
        emit mouseReleaseOnOverlay(id, event);
    }
}

bool Overlays::mouseMoveEvent(QMouseEvent* event) {
    PerformanceTimer perfTimer("Overlays::mouseMoveEvent");

    PickRay ray = qApp->computePickRay(event->x(), event->y());
    RayToOverlayIntersectionResult rayPickResult = findRayIntersectionVector(ray, true, QVector<EntityItemID>(), QVector<EntityItemID>());
    if (rayPickResult.intersects) {
        auto pointerEvent = calculateOverlayPointerEvent(rayPickResult.overlayID, ray, rayPickResult, event, PointerEvent::Move);
        mouseMovePointerEvent(rayPickResult.overlayID, pointerEvent);

        // If previously hovering over a different overlay then leave hover on that overlay.
        if (_currentHoverOverOverlayID != UNKNOWN_ENTITY_ID && rayPickResult.overlayID != _currentHoverOverOverlayID) {
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
        if (_currentHoverOverOverlayID != UNKNOWN_ENTITY_ID) {
            auto pointerEvent = calculateOverlayPointerEvent(_currentHoverOverOverlayID, ray, rayPickResult, event, PointerEvent::Move);
            hoverLeavePointerEvent(_currentHoverOverOverlayID, pointerEvent);

            _currentHoverOverOverlayID = UNKNOWN_ENTITY_ID;
        }
    }
    return false;
}

void Overlays::mouseMovePointerEvent(const QUuid& id, const PointerEvent& event) {
    // TODO: generalize this to allow any overlay to recieve events
    auto renderable = qApp->getEntities()->renderableForEntityId(id);
    if (renderable) {
        auto web = std::dynamic_pointer_cast<render::entities::WebEntityRenderer>(renderable);
        if (web) {
            web->handlePointerEvent(event);
        }
    }

    auto keyboard = DependencyManager::get<Keyboard>();
    // Do not send keyboard key event to scripts to prevent malignant scripts from gathering what you typed
    if (!keyboard->getKeysID().contains(id)) {
        // emit to scripts
        emit mouseMoveOnOverlay(id, event);
    }
}

QVector<QUuid> Overlays::findOverlays(const glm::vec3& center, float radius) {
    PROFILE_RANGE(script_entities, __FUNCTION__);

    QVector<QUuid> result;
    auto entityTree = DependencyManager::get<EntityScriptingInterface>()->getEntityTree();
    if (entityTree) {
        unsigned int searchFilter = PickFilter::getBitMask(PickFilter::FlagBit::LOCAL_ENTITIES);
        entityTree->withReadLock([&] {
            entityTree->evalEntitiesInSphere(center, radius, PickFilter(searchFilter), result);
        });
    }
    return result;
}
