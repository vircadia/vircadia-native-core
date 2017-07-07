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

Q_LOGGING_CATEGORY(trace_render_overlays, "trace.render.overlays")

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

        if (transaction._removedItems.size() > 0) {
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
    }
    if (_overlaysWorld.contains(id)) {
        return _overlaysWorld[id];
    }
    return nullptr;
}

OverlayID Overlays::addOverlay(const QString& type, const QVariant& properties) {
    if (QThread::currentThread() != thread()) {
        OverlayID result;
        BLOCKING_INVOKE_METHOD(this, "addOverlay", Q_RETURN_ARG(OverlayID, result), Q_ARG(QString, type), Q_ARG(QVariant, properties));
        return result;
    }

    Overlay::Pointer thisOverlay = nullptr;

    if (type == ImageOverlay::TYPE) {
        thisOverlay = std::make_shared<ImageOverlay>();
    } else if (type == Image3DOverlay::TYPE || type == "billboard") { // "billboard" for backwards compatibility
        thisOverlay = std::make_shared<Image3DOverlay>();
    } else if (type == TextOverlay::TYPE) {
        thisOverlay = std::make_shared<TextOverlay>();
    } else if (type == Text3DOverlay::TYPE) {
        thisOverlay = std::make_shared<Text3DOverlay>();
    } else if (type == Shape3DOverlay::TYPE) {
        thisOverlay = std::make_shared<Shape3DOverlay>();
    } else if (type == Cube3DOverlay::TYPE) {
        thisOverlay = std::make_shared<Cube3DOverlay>();
    } else if (type == Sphere3DOverlay::TYPE) {
        thisOverlay = std::make_shared<Sphere3DOverlay>();
    } else if (type == Circle3DOverlay::TYPE) {
        thisOverlay = std::make_shared<Circle3DOverlay>();
    } else if (type == Rectangle3DOverlay::TYPE) {
        thisOverlay = std::make_shared<Rectangle3DOverlay>();
    } else if (type == Line3DOverlay::TYPE) {
        thisOverlay = std::make_shared<Line3DOverlay>();
    } else if (type == Grid3DOverlay::TYPE) {
        thisOverlay = std::make_shared<Grid3DOverlay>();
    } else if (type == ModelOverlay::TYPE) {
        thisOverlay = std::make_shared<ModelOverlay>();
    } else if (type == Web3DOverlay::TYPE) {
        thisOverlay = std::make_shared<Web3DOverlay>();
    } else if (type == RectangleOverlay::TYPE) {
        thisOverlay = std::make_shared<RectangleOverlay>();
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
        BLOCKING_INVOKE_METHOD(this, "cloneOverlay", Q_RETURN_ARG(OverlayID, result), Q_ARG(OverlayID, id));
        return result;
    }

    Overlay::Pointer thisOverlay = getOverlay(id);

    if (thisOverlay) {
        OverlayID cloneId = addOverlay(Overlay::Pointer(thisOverlay->createClone()));
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
    if (QThread::currentThread() != thread()) {
        // NOTE editOverlay can be called very frequently in scripts and can't afford to 
        // block waiting on the main thread.  Additionally, no script actually 
        // examines the return value and does something useful with it, so use a non-blocking
        // invoke and just always return true
        QMetaObject::invokeMethod(this, "editOverlay", Q_ARG(OverlayID, id), Q_ARG(QVariant, properties));
        return true;
    }

    Overlay::Pointer thisOverlay = getOverlay(id);
    if (thisOverlay) {
        thisOverlay->setProperties(properties.toMap());
        return true;
    }
    return false;
}

bool Overlays::editOverlays(const QVariant& propertiesById) {
    if (QThread::currentThread() != thread()) {
        // NOTE see comment on editOverlay for why this is not a blocking call
        QMetaObject::invokeMethod(this, "editOverlays", Q_ARG(QVariant, propertiesById));
        return true;
    }

    QVariantMap map = propertiesById.toMap();
    bool success = true;
    for (const auto& key : map.keys()) {
        OverlayID id = OverlayID(key);
        Overlay::Pointer thisOverlay = getOverlay(id);
        if (!thisOverlay) {
            success = false;
            continue;
        }
        QVariant properties = map[key];
        thisOverlay->setProperties(properties.toMap());
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
    if (QThread::currentThread() != thread()) {
        OverlayID result;
        BLOCKING_INVOKE_METHOD(this, "getOverlayAtPoint", Q_RETURN_ARG(OverlayID, result), Q_ARG(glm::vec2, point));
        return result;
    }

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
    if (QThread::currentThread() != thread()) {
        OverlayPropertyResult result;
        BLOCKING_INVOKE_METHOD(this, "getProperty", Q_RETURN_ARG(OverlayPropertyResult, result), Q_ARG(OverlayID, id), Q_ARG(QString, property));
        return result;
    }

    OverlayPropertyResult result;
    Overlay::Pointer thisOverlay = getOverlay(id);
    if (thisOverlay && thisOverlay->supportsGetProperty()) {
        result.value = thisOverlay->getProperty(property);
    }
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

    return findRayIntersectionInternal(ray, precisionPicking,
                                       overlaysToInclude, overlaysToDiscard, visibleOnly, collidableOnly);
}


RayToOverlayIntersectionResult Overlays::findRayIntersectionInternal(const PickRay& ray, bool precisionPicking,
                                                                     const QVector<OverlayID>& overlaysToInclude,
                                                                     const QVector<OverlayID>& overlaysToDiscard,
                                                                     bool visibleOnly, bool collidableOnly) {
    if (QThread::currentThread() != thread()) {
        RayToOverlayIntersectionResult result;
        BLOCKING_INVOKE_METHOD(this, "findRayIntersectionInternal", Q_RETURN_ARG(RayToOverlayIntersectionResult, result), 
            Q_ARG(PickRay, ray), 
            Q_ARG(bool, precisionPicking), 
            Q_ARG(QVector<OverlayID>, overlaysToInclude), 
            Q_ARG(QVector<OverlayID>, overlaysToDiscard), 
            Q_ARG(bool, visibleOnly), 
            Q_ARG(bool, collidableOnly));
        return result;
    }

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
        BLOCKING_INVOKE_METHOD(this, "isAddedOverlay", Q_RETURN_ARG(bool, result), Q_ARG(OverlayID, id));
        return result;
    }

    QMutexLocker locker(&_mutex);
    return _overlaysHUD.contains(id) || _overlaysWorld.contains(id);
}

void Overlays::sendMousePressOnOverlay(OverlayID overlayID, const PointerEvent& event) {
    emit mousePressOnOverlay(overlayID, event);
}

void Overlays::sendMouseReleaseOnOverlay(OverlayID overlayID, const PointerEvent& event) {
    emit mouseReleaseOnOverlay(overlayID, event);
}

void Overlays::sendMouseMoveOnOverlay(OverlayID overlayID, const PointerEvent& event) {
    emit mouseMoveOnOverlay(overlayID, event);
}

void Overlays::sendHoverEnterOverlay(OverlayID id, PointerEvent event) {
    emit hoverEnterOverlay(id, event);
}

void Overlays::sendHoverOverOverlay(OverlayID  id, PointerEvent event) {
    emit hoverOverOverlay(id, event);
}

void Overlays::sendHoverLeaveOverlay(OverlayID  id, PointerEvent event) {
    emit hoverLeaveOverlay(id, event);
}

OverlayID Overlays::getKeyboardFocusOverlay() {
    if (QThread::currentThread() != thread()) {
        OverlayID result;
        BLOCKING_INVOKE_METHOD(this, "getKeyboardFocusOverlay", Q_RETURN_ARG(OverlayID, result));
        return result;
    }

    return qApp->getKeyboardFocusOverlay();
}

void Overlays::setKeyboardFocusOverlay(OverlayID id) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "setKeyboardFocusOverlay", Q_ARG(OverlayID, id));
        return;
    }

    qApp->setKeyboardFocusOverlay(id);
}

float Overlays::width() {
    if (QThread::currentThread() != thread()) {
        float result;
        BLOCKING_INVOKE_METHOD(this, "width", Q_RETURN_ARG(float, result));
        return result;
    }

    auto offscreenUi = DependencyManager::get<OffscreenUi>();
    return offscreenUi->getWindow()->size().width();
}

float Overlays::height() {
    if (QThread::currentThread() != thread()) {
        float result;
        BLOCKING_INVOKE_METHOD(this, "height", Q_RETURN_ARG(float, result));
        return result;
    }

    auto offscreenUi = DependencyManager::get<OffscreenUi>();
    return offscreenUi->getWindow()->size().height();
}

static const uint32_t MOUSE_POINTER_ID = 0;

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

PointerEvent Overlays::calculatePointerEvent(Overlay::Pointer overlay, PickRay ray,
                                             RayToOverlayIntersectionResult rayPickResult, QMouseEvent* event,
                                             PointerEvent::EventType eventType) {

    auto thisOverlay = std::dynamic_pointer_cast<Web3DOverlay>(overlay);

    auto position = thisOverlay->getPosition();
    auto rotation = thisOverlay->getRotation();
    auto dimensions = thisOverlay->getSize();

    glm::vec2 pos2D = projectOntoOverlayXYPlane(position, rotation, dimensions, ray, rayPickResult);

    PointerEvent pointerEvent(eventType, MOUSE_POINTER_ID, pos2D, rayPickResult.intersection, rayPickResult.surfaceNormal,
                              ray.direction, toPointerButton(*event), toPointerButtons(*event), event->modifiers());

    return pointerEvent;
}


RayToOverlayIntersectionResult Overlays::findRayIntersectionForMouseEvent(PickRay ray) {
    QVector<OverlayID> overlaysToInclude;
    QVector<OverlayID> overlaysToDiscard;
    RayToOverlayIntersectionResult rayPickResult;

    // first priority is tablet screen
    overlaysToInclude << qApp->getTabletScreenID();
    rayPickResult = findRayIntersectionInternal(ray, true, overlaysToInclude, overlaysToDiscard);
    if (rayPickResult.intersects) {
        return rayPickResult;
    }
    // then tablet home button
    overlaysToInclude.clear();
    overlaysToInclude << qApp->getTabletHomeButtonID();
    rayPickResult = findRayIntersectionInternal(ray, true, overlaysToInclude, overlaysToDiscard);
    if (rayPickResult.intersects) {
        return rayPickResult;
    }
    // then tablet frame
    overlaysToInclude.clear();
    overlaysToInclude << OverlayID(qApp->getTabletFrameID());
    rayPickResult = findRayIntersectionInternal(ray, true, overlaysToInclude, overlaysToDiscard);
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

        // Only Web overlays can have focus.
        auto thisOverlay = std::dynamic_pointer_cast<Web3DOverlay>(getOverlay(_currentClickingOnOverlayID));
        if (thisOverlay) {
            auto pointerEvent = calculatePointerEvent(thisOverlay, ray, rayPickResult, event, PointerEvent::Press);
            emit mousePressOnOverlay(_currentClickingOnOverlayID, pointerEvent);
            return true;
        }
    }
    emit mousePressOffOverlay();
    return false;
}

bool Overlays::mouseDoublePressEvent(QMouseEvent* event) {
    PerformanceTimer perfTimer("Overlays::mouseDoublePressEvent");

    PickRay ray = qApp->computePickRay(event->x(), event->y());
    RayToOverlayIntersectionResult rayPickResult = findRayIntersectionForMouseEvent(ray);
    if (rayPickResult.intersects) {
        _currentClickingOnOverlayID = rayPickResult.overlayID;

        // Only Web overlays can have focus.
        auto thisOverlay = std::dynamic_pointer_cast<Web3DOverlay>(getOverlay(_currentClickingOnOverlayID));
        if (thisOverlay) {
            auto pointerEvent = calculatePointerEvent(thisOverlay, ray, rayPickResult, event, PointerEvent::Press);
            emit mouseDoublePressOnOverlay(_currentClickingOnOverlayID, pointerEvent);
            return true;
        }
    }
    emit mouseDoublePressOffOverlay();
    return false;
}

bool Overlays::mouseReleaseEvent(QMouseEvent* event) {
    PerformanceTimer perfTimer("Overlays::mouseReleaseEvent");

    PickRay ray = qApp->computePickRay(event->x(), event->y());
    RayToOverlayIntersectionResult rayPickResult = findRayIntersectionForMouseEvent(ray);
    if (rayPickResult.intersects) {

        // Only Web overlays can have focus.
        auto thisOverlay = std::dynamic_pointer_cast<Web3DOverlay>(getOverlay(rayPickResult.overlayID));
        if (thisOverlay) {
            auto pointerEvent = calculatePointerEvent(thisOverlay, ray, rayPickResult, event, PointerEvent::Release);
            emit mouseReleaseOnOverlay(rayPickResult.overlayID, pointerEvent);
        }
    }

    _currentClickingOnOverlayID = UNKNOWN_OVERLAY_ID;
    return false;
}

bool Overlays::mouseMoveEvent(QMouseEvent* event) {
    PerformanceTimer perfTimer("Overlays::mouseMoveEvent");

    PickRay ray = qApp->computePickRay(event->x(), event->y());
    RayToOverlayIntersectionResult rayPickResult = findRayIntersectionForMouseEvent(ray);
    if (rayPickResult.intersects) {

        // Only Web overlays can have focus.
        auto thisOverlay = std::dynamic_pointer_cast<Web3DOverlay>(getOverlay(rayPickResult.overlayID));
        if (thisOverlay) {
            auto pointerEvent = calculatePointerEvent(thisOverlay, ray, rayPickResult, event, PointerEvent::Move);
            emit mouseMoveOnOverlay(rayPickResult.overlayID, pointerEvent);

            // If previously hovering over a different overlay then leave hover on that overlay.
            if (_currentHoverOverOverlayID != UNKNOWN_OVERLAY_ID && rayPickResult.overlayID != _currentHoverOverOverlayID) {
                auto thisOverlay = std::dynamic_pointer_cast<Web3DOverlay>(getOverlay(_currentHoverOverOverlayID));
                if (thisOverlay) {
                    auto pointerEvent = calculatePointerEvent(thisOverlay, ray, rayPickResult, event, PointerEvent::Move);
                    emit hoverLeaveOverlay(_currentHoverOverOverlayID, pointerEvent);
                }
            }

            // If hovering over a new overlay then enter hover on that overlay.
            if (rayPickResult.overlayID != _currentHoverOverOverlayID) {
                emit hoverEnterOverlay(rayPickResult.overlayID, pointerEvent);
            }

            // Hover over current overlay.
            emit hoverOverOverlay(rayPickResult.overlayID, pointerEvent);

            _currentHoverOverOverlayID = rayPickResult.overlayID;
        }
    } else {
        // If previously hovering an overlay then leave hover.
        if (_currentHoverOverOverlayID != UNKNOWN_OVERLAY_ID) {
            auto thisOverlay = std::dynamic_pointer_cast<Web3DOverlay>(getOverlay(_currentHoverOverOverlayID));
            if (thisOverlay) {
                auto pointerEvent = calculatePointerEvent(thisOverlay, ray, rayPickResult, event, PointerEvent::Move);
                emit hoverLeaveOverlay(_currentHoverOverOverlayID, pointerEvent);
            }

            _currentHoverOverOverlayID = UNKNOWN_OVERLAY_ID;
        }
    }
    return false;
}

QVector<QUuid> Overlays::findOverlays(const glm::vec3& center, float radius) {
    QVector<QUuid> result;
    if (QThread::currentThread() != thread()) {
        BLOCKING_INVOKE_METHOD(this, "findOverlays", Q_RETURN_ARG(QVector<QUuid>, result), Q_ARG(glm::vec3, center), Q_ARG(float, radius));
        return result;
    }

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
