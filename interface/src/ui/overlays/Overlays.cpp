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

#include <OffscreenUi.h>
#include <render/Scene.h>
#include <RegisteredMetaTypes.h>

#include "Application.h"
#include "Image3DOverlay.h"
#include "Circle3DOverlay.h"
#include "Cube3DOverlay.h"
#include "Shape3DOverlay.h"
#include "ImageOverlay.h"
#include "Line3DOverlay.h"
#include "LocalModelsOverlay.h"
#include "ModelOverlay.h"
#include "Rectangle3DOverlay.h"
#include "Sphere3DOverlay.h"
#include "Grid3DOverlay.h"
#include "TextOverlay.h"
#include "RectangleOverlay.h"
#include "Text3DOverlay.h"
#include "Web3DOverlay.h"
#include <QtQuick/QQuickWindow>


Overlays::Overlays() :
    _nextOverlayID(1) {}

void Overlays::cleanupAllOverlays() {
    {
        QWriteLocker lock(&_lock);
        QWriteLocker deleteLock(&_deleteLock);
        foreach(Overlay::Pointer overlay, _overlaysHUD) {
            _overlaysToDelete.push_back(overlay);
        }
        foreach(Overlay::Pointer overlay, _overlaysWorld) {
            _overlaysToDelete.push_back(overlay);
        }
        _overlaysHUD.clear();
        _overlaysWorld.clear();
        _panels.clear();
    }
    cleanupOverlaysToDelete();
}

void Overlays::init() {
    _scriptEngine = new QScriptEngine();
}

void Overlays::update(float deltatime) {

    {
        QWriteLocker lock(&_lock);
        foreach(Overlay::Pointer thisOverlay, _overlaysHUD) {
            thisOverlay->update(deltatime);
        }
        foreach(Overlay::Pointer thisOverlay, _overlaysWorld) {
            thisOverlay->update(deltatime);
        }
    }

    cleanupOverlaysToDelete();
}

void Overlays::cleanupOverlaysToDelete() {
    if (!_overlaysToDelete.isEmpty()) {
        render::ScenePointer scene = qApp->getMain3DScene();
        render::PendingChanges pendingChanges;

        {
            QWriteLocker lock(&_deleteLock);

            do {
                Overlay::Pointer overlay = _overlaysToDelete.takeLast();

                auto itemID = overlay->getRenderItemID();
                if (render::Item::isValidID(itemID)) {
                    overlay->removeFromScene(overlay, scene, pendingChanges);
                }
            } while (!_overlaysToDelete.isEmpty());
        }

        if (pendingChanges._removedItems.size() > 0) {
            scene->enqueuePendingChanges(pendingChanges);
        }
    }
}

void Overlays::renderHUD(RenderArgs* renderArgs) {
    PROFILE_RANGE(__FUNCTION__);
    QReadLocker lock(&_lock);
    gpu::Batch& batch = *renderArgs->_batch;

    auto geometryCache = DependencyManager::get<GeometryCache>();
    auto textureCache = DependencyManager::get<TextureCache>();

    auto size = qApp->getUiSize();
    int width = size.x;
    int height = size.y;
    mat4 legacyProjection = glm::ortho<float>(0, width, height, 0, -1000, 1000);


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
    QWriteLocker lock(&_lock);
    _enabled = false;
}

void Overlays::enable() {
    QWriteLocker lock(&_lock);
    _enabled = true;
}

Overlay::Pointer Overlays::getOverlay(unsigned int id) const {
    if (_overlaysHUD.contains(id)) {
        return _overlaysHUD[id];
    }
    if (_overlaysWorld.contains(id)) {
        return _overlaysWorld[id];
    }
    return nullptr;
}

unsigned int Overlays::addOverlay(const QString& type, const QVariant& properties) {
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
    } else if (type == LocalModelsOverlay::TYPE) {
        thisOverlay = std::make_shared<LocalModelsOverlay>(qApp->getEntityClipboardRenderer());
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
    return 0;
}

unsigned int Overlays::addOverlay(Overlay::Pointer overlay) {
    QWriteLocker lock(&_lock);
    unsigned int thisID = _nextOverlayID;
    overlay->setOverlayID(thisID);
    _nextOverlayID++;
    if (overlay->is3D()) {
        _overlaysWorld[thisID] = overlay;

        render::ScenePointer scene = qApp->getMain3DScene();
        render::PendingChanges pendingChanges;
        overlay->addToScene(overlay, scene, pendingChanges);
        scene->enqueuePendingChanges(pendingChanges);
    } else {
        _overlaysHUD[thisID] = overlay;
    }

    return thisID;
}

unsigned int Overlays::cloneOverlay(unsigned int id) {
    Overlay::Pointer thisOverlay = getOverlay(id);

    if (thisOverlay) {
        unsigned int cloneId = addOverlay(Overlay::Pointer(thisOverlay->createClone()));
        auto attachable = std::dynamic_pointer_cast<PanelAttachable>(thisOverlay);
        if (attachable && attachable->getParentPanel()) {
            attachable->getParentPanel()->addChild(cloneId);
        }
        return cloneId;
    } 
    
    return 0;  // Not found
}

bool Overlays::editOverlay(unsigned int id, const QVariant& properties) {
    QWriteLocker lock(&_lock);

    Overlay::Pointer thisOverlay = getOverlay(id);
    if (thisOverlay) {
        thisOverlay->setProperties(properties.toMap());

        return true;
    }
    return false;
}

bool Overlays::editOverlays(const QVariant& propertiesById) {
    QVariantMap map = propertiesById.toMap();
    bool success = true;
    QWriteLocker lock(&_lock);
    for (const auto& key : map.keys()) {
        bool convertSuccess;
        unsigned int id = key.toUInt(&convertSuccess);
        if (!convertSuccess) {
            success = false;
            continue;
        }

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

void Overlays::deleteOverlay(unsigned int id) {
    Overlay::Pointer overlayToDelete;

    {
        QWriteLocker lock(&_lock);
        if (_overlaysHUD.contains(id)) {
            overlayToDelete = _overlaysHUD.take(id);
        } else if (_overlaysWorld.contains(id)) {
            overlayToDelete = _overlaysWorld.take(id);
        } else {
            return;
        }
    }

    auto attachable = std::dynamic_pointer_cast<PanelAttachable>(overlayToDelete);
    if (attachable && attachable->getParentPanel()) {
        attachable->getParentPanel()->removeChild(id);
        attachable->setParentPanel(nullptr);
    }

    QWriteLocker lock(&_deleteLock);
    _overlaysToDelete.push_back(overlayToDelete);

    emit overlayDeleted(id);
}

QString Overlays::getOverlayType(unsigned int overlayId) const {
    Overlay::Pointer overlay = getOverlay(overlayId);
    if (overlay) {
        return overlay->getType();
    }
    return "";
}

QObject* Overlays::getOverlayObject(unsigned int id) {
    Overlay::Pointer thisOverlay = getOverlay(id);
    if (thisOverlay) {
        return qobject_cast<QObject*>(&(*thisOverlay));
    }
    return nullptr;
}

unsigned int Overlays::getParentPanel(unsigned int childId) const {
    Overlay::Pointer overlay = getOverlay(childId);
    auto attachable = std::dynamic_pointer_cast<PanelAttachable>(overlay);
    if (attachable) {
        return _panels.key(attachable->getParentPanel());
    } else if (_panels.contains(childId)) {
        return _panels.key(getPanel(childId)->getParentPanel());
    }
    return 0;
}

void Overlays::setParentPanel(unsigned int childId, unsigned int panelId) {
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

unsigned int Overlays::getOverlayAtPoint(const glm::vec2& point) {
    glm::vec2 pointCopy = point;
    QReadLocker lock(&_lock);
    if (!_enabled) {
        return 0;
    }
    QMapIterator<unsigned int, Overlay::Pointer> i(_overlaysHUD);
    i.toBack();

    const float LARGE_NEGATIVE_FLOAT = -9999999;
    glm::vec3 origin(pointCopy.x, pointCopy.y, LARGE_NEGATIVE_FLOAT);
    glm::vec3 direction(0, 0, 1);
    BoxFace thisFace;
    glm::vec3 thisSurfaceNormal;
    float distance;

    while (i.hasPrevious()) {
        i.previous();
        unsigned int thisID = i.key();
        if (i.value()->is3D()) {
            auto thisOverlay = std::dynamic_pointer_cast<Base3DOverlay>(i.value());
            if (thisOverlay && !thisOverlay->getIgnoreRayIntersection()) {
                if (thisOverlay->findRayIntersection(origin, direction, distance, thisFace, thisSurfaceNormal)) {
                    return thisID;
                }
            }
        } else {
            auto thisOverlay = std::dynamic_pointer_cast<Overlay2D>(i.value());
            if (thisOverlay && thisOverlay->getVisible() && thisOverlay->isLoaded() &&
                thisOverlay->getBoundingRect().contains(pointCopy.x, pointCopy.y, false)) {
                return thisID;
            }
        }
    }

    return 0; // not found
}

OverlayPropertyResult Overlays::getProperty(unsigned int id, const QString& property) {
    OverlayPropertyResult result;
    Overlay::Pointer thisOverlay = getOverlay(id);
    QReadLocker lock(&_lock);
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
    return engine->newVariant(value.value);
}

void OverlayPropertyResultFromScriptValue(const QScriptValue& object, OverlayPropertyResult& value) {
    value.value = object.toVariant();
}


RayToOverlayIntersectionResult Overlays::findRayIntersection(const PickRay& ray) {
    float bestDistance = std::numeric_limits<float>::max();
    bool bestIsFront = false;
    RayToOverlayIntersectionResult result;
    QMapIterator<unsigned int, Overlay::Pointer> i(_overlaysWorld);
    i.toBack();
    while (i.hasPrevious()) {
        i.previous();
        unsigned int thisID = i.key();
        auto thisOverlay = std::dynamic_pointer_cast<Base3DOverlay>(i.value());
        if (thisOverlay && thisOverlay->getVisible() && !thisOverlay->getIgnoreRayIntersection() && thisOverlay->isLoaded()) {
            float thisDistance;
            BoxFace thisFace;
            glm::vec3 thisSurfaceNormal;
            QString thisExtraInfo;
            if (thisOverlay->findRayIntersectionExtraInfo(ray.origin, ray.direction, thisDistance, 
                                                            thisFace, thisSurfaceNormal, thisExtraInfo)) {
                bool isDrawInFront = thisOverlay->getDrawInFront();
                if (thisDistance < bestDistance && (!bestIsFront || isDrawInFront)) {
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

RayToOverlayIntersectionResult::RayToOverlayIntersectionResult() : 
    intersects(false), 
    overlayID(-1),
    distance(0),
    face(),
    intersection(),
    extraInfo()
{ 
}

QScriptValue RayToOverlayIntersectionResultToScriptValue(QScriptEngine* engine, const RayToOverlayIntersectionResult& value) {
    auto obj = engine->newObject();
    obj.setProperty("intersects", value.intersects);
    obj.setProperty("overlayID", value.overlayID);
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
    value.overlayID = object["overlayID"].toInt();
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

bool Overlays::isLoaded(unsigned int id) {
    QReadLocker lock(&_lock);
    Overlay::Pointer thisOverlay = getOverlay(id);
    if (!thisOverlay) {
        return false; // not found
    }
    return thisOverlay->isLoaded();
}

QSizeF Overlays::textSize(unsigned int id, const QString& text) const {
    Overlay::Pointer thisOverlay = _overlaysHUD[id];
    if (thisOverlay) {
        if (auto textOverlay = std::dynamic_pointer_cast<TextOverlay>(thisOverlay)) {
            return textOverlay->textSize(text);
        }
    } else {
        thisOverlay = _overlaysWorld[id];
        if (auto text3dOverlay = std::dynamic_pointer_cast<Text3DOverlay>(thisOverlay)) {
            return text3dOverlay->textSize(text);
        }
    }
    return QSizeF(0.0f, 0.0f);
}

unsigned int Overlays::addPanel(OverlayPanel::Pointer panel) {
    QWriteLocker lock(&_lock);

    unsigned int thisID = _nextOverlayID;
    _nextOverlayID++;
    _panels[thisID] = panel;

    return thisID;
}

unsigned int Overlays::addPanel(const QVariant& properties) {
    OverlayPanel::Pointer panel = std::make_shared<OverlayPanel>();
    panel->init(_scriptEngine);
    panel->setProperties(properties.toMap());
    return addPanel(panel);
}

void Overlays::editPanel(unsigned int panelId, const QVariant& properties) {
    if (_panels.contains(panelId)) {
        _panels[panelId]->setProperties(properties.toMap());
    }
}

OverlayPropertyResult Overlays::getPanelProperty(unsigned int panelId, const QString& property) {
    OverlayPropertyResult result;
    if (_panels.contains(panelId)) {
        OverlayPanel::Pointer thisPanel = getPanel(panelId);
        QReadLocker lock(&_lock);
        result.value = thisPanel->getProperty(property);
    }
    return result;
}


void Overlays::deletePanel(unsigned int panelId) {
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
        unsigned int childId = panelToDelete->popLastChild();
        deleteOverlay(childId);
        deletePanel(childId);
    }

    emit panelDeleted(panelId);
}

bool Overlays::isAddedOverlay(unsigned int id) {
    return _overlaysHUD.contains(id) || _overlaysWorld.contains(id);
}

void Overlays::sendMousePressOnOverlay(unsigned int overlayID, const PointerEvent& event) {
    emit mousePressOnOverlay(overlayID, event);
}

void Overlays::sendMouseReleaseOnOverlay(unsigned int overlayID, const PointerEvent& event) {
    emit mouseReleaseOnOverlay(overlayID, event);
}

void Overlays::sendMouseMoveOnOverlay(unsigned int overlayID, const PointerEvent& event) {
    emit mouseMoveOnOverlay(overlayID, event);
}

void Overlays::sendHoverEnterOverlay(unsigned int id, PointerEvent event) {
    emit hoverEnterOverlay(id, event);
}

void Overlays::sendHoverOverOverlay(unsigned int  id, PointerEvent event) {
    emit hoverOverOverlay(id, event);
}

void Overlays::sendHoverLeaveOverlay(unsigned int  id, PointerEvent event) {
    emit hoverLeaveOverlay(id, event);
}

unsigned int Overlays::getKeyboardFocusOverlay() const {
    return qApp->getKeyboardFocusOverlay();
}

void Overlays::setKeyboardFocusOverlay(unsigned int id) {
    qApp->setKeyboardFocusOverlay(id);
}

float Overlays::width() const {
    auto offscreenUi = DependencyManager::get<OffscreenUi>();
    return offscreenUi->getWindow()->size().width();
}

float Overlays::height() const {
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
    RayToOverlayIntersectionResult rayPickResult, QMouseEvent* event, PointerEvent::EventType eventType) {

    auto thisOverlay = std::dynamic_pointer_cast<Web3DOverlay>(overlay);

    QReadLocker lock(&_lock);
    auto position = thisOverlay->getPosition();
    auto rotation = thisOverlay->getRotation();
    auto dimensions = thisOverlay->getSize();

    glm::vec2 pos2D = projectOntoOverlayXYPlane(position, rotation, dimensions, ray, rayPickResult);
    PointerEvent pointerEvent(eventType, MOUSE_POINTER_ID,
        pos2D, rayPickResult.intersection,
        rayPickResult.surfaceNormal, ray.direction,
        toPointerButton(*event), toPointerButtons(*event));

    return pointerEvent;
}

void Overlays::mousePressEvent(QMouseEvent* event) {
    PerformanceTimer perfTimer("Overlays::mousePressEvent");

    PickRay ray = qApp->computePickRay(event->x(), event->y());
    RayToOverlayIntersectionResult rayPickResult = findRayIntersection(ray);
    if (rayPickResult.intersects) {
        _currentClickingOnOverlayID = rayPickResult.overlayID;

        // Only Web overlays can have focus.
        auto thisOverlay = std::dynamic_pointer_cast<Web3DOverlay>(getOverlay(_currentClickingOnOverlayID));
        if (thisOverlay) {
            auto pointerEvent = calculatePointerEvent(thisOverlay, ray, rayPickResult, event, PointerEvent::Press);
            emit mousePressOnOverlay(_currentClickingOnOverlayID, pointerEvent);
        } else {
            emit mousePressOffOverlay();
        }
    } else {
        emit mousePressOffOverlay();
    }
}

void Overlays::mouseReleaseEvent(QMouseEvent* event) {
    PerformanceTimer perfTimer("Overlays::mouseReleaseEvent");

    PickRay ray = qApp->computePickRay(event->x(), event->y());
    RayToOverlayIntersectionResult rayPickResult = findRayIntersection(ray);
    if (rayPickResult.intersects) {

        // Only Web overlays can have focus.
        auto thisOverlay = std::dynamic_pointer_cast<Web3DOverlay>(getOverlay(rayPickResult.overlayID));
        if (thisOverlay) {
            auto pointerEvent = calculatePointerEvent(thisOverlay, ray, rayPickResult, event, PointerEvent::Release);
            emit mouseReleaseOnOverlay(rayPickResult.overlayID, pointerEvent);
        }
    }

    _currentClickingOnOverlayID = UNKNOWN_OVERLAY_ID;
}

void Overlays::mouseMoveEvent(QMouseEvent* event) {
    PerformanceTimer perfTimer("Overlays::mouseMoveEvent");

    PickRay ray = qApp->computePickRay(event->x(), event->y());
    RayToOverlayIntersectionResult rayPickResult = findRayIntersection(ray);
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
}
