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

#include <QScriptValueIterator>

#include <limits>

#include <render/Scene.h>
#include <gpu/GLBackend.h>
#include <RegisteredMetaTypes.h>

#include "Application.h"
#include "BillboardOverlay.h"
#include "Circle3DOverlay.h"
#include "Cube3DOverlay.h"
#include "ImageOverlay.h"
#include "Line3DOverlay.h"
#include "LocalModelsOverlay.h"
#include "ModelOverlay.h"
#include "Rectangle3DOverlay.h"
#include "Sphere3DOverlay.h"
#include "Grid3DOverlay.h"
#include "TextOverlay.h"
#include "Text3DOverlay.h"


Overlays::Overlays() : _nextOverlayID(1) {
}

Overlays::~Overlays() {
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
        render::ScenePointer scene = Application::getInstance()->getMain3DScene();
        render::PendingChanges pendingChanges;

        {
            QWriteLocker lock(&_deleteLock);

            do {
                Overlay::Pointer overlay = _overlaysToDelete.takeLast();

                auto itemID = overlay->getRenderItemID();
                if (itemID != render::Item::INVALID_ITEM_ID) {
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

    auto size = qApp->getCanvasSize();
    int width = size.x;
    int height = size.y;
    mat4 legacyProjection = glm::ortho<float>(0, width, height, 0, -1000, 1000);


    foreach(Overlay::Pointer thisOverlay, _overlaysHUD) {
    
        // Reset all batch pipeline settings between overlay
        geometryCache->useSimpleDrawPipeline(batch);
        batch.setResourceTexture(0, textureCache->getWhiteTexture()); // FIXME - do we really need to do this??
        batch.setProjectionTransform(legacyProjection);
        batch.setModelTransform(Transform());
        batch.setViewTransform(Transform());
        batch._glLineWidth(1.0f); // default

        thisOverlay->render(renderArgs);
    }
}

unsigned int Overlays::addOverlay(const QString& type, const QScriptValue& properties) {
    unsigned int thisID = 0;
    Overlay* thisOverlay = NULL;
    
    bool created = true;
    if (type == "image") {
        thisOverlay = new ImageOverlay();
    } else if (type == "text") {
        thisOverlay = new TextOverlay();
    } else if (type == "text3d") {
        thisOverlay = new Text3DOverlay();
    } else if (type == "cube") {
        thisOverlay = new Cube3DOverlay();
    } else if (type == "sphere") {
        thisOverlay = new Sphere3DOverlay();
    } else if (type == "circle3d") {
        thisOverlay = new Circle3DOverlay();
    } else if (type == "rectangle3d") {
        thisOverlay = new Rectangle3DOverlay();
    } else if (type == "line3d") {
        thisOverlay = new Line3DOverlay();
    } else if (type == "grid") {
        thisOverlay = new Grid3DOverlay();
    } else if (type == "localmodels") {
        thisOverlay = new LocalModelsOverlay(Application::getInstance()->getEntityClipboardRenderer());
    } else if (type == "model") {
        thisOverlay = new ModelOverlay();
    } else if (type == "billboard") {
        thisOverlay = new BillboardOverlay();
    } else {
        created = false;
    }

    if (created) {
        thisOverlay->setProperties(properties);
        thisID = addOverlay(thisOverlay);
    }

    return thisID; 
}

unsigned int Overlays::addOverlay(Overlay* overlay) {
    Overlay::Pointer overlayPointer(overlay);
    overlay->init(_scriptEngine);

    QWriteLocker lock(&_lock);
    unsigned int thisID = _nextOverlayID;
    _nextOverlayID++;
    if (overlay->is3D()) {
        Base3DOverlay* overlay3D = static_cast<Base3DOverlay*>(overlay);
        if (overlay3D->getDrawOnHUD()) {
            _overlaysHUD[thisID] = overlayPointer;
        } else {
            _overlaysWorld[thisID] = overlayPointer;

            render::ScenePointer scene = Application::getInstance()->getMain3DScene();
            render::PendingChanges pendingChanges;

            overlayPointer->addToScene(overlayPointer, scene, pendingChanges);

            scene->enqueuePendingChanges(pendingChanges);
        }
    } else {
        _overlaysHUD[thisID] = overlayPointer;
    }
    
    return thisID;
}

unsigned int Overlays::cloneOverlay(unsigned int id) {
    Overlay::Pointer thisOverlay = NULL;
    if (_overlaysHUD.contains(id)) {
        thisOverlay = _overlaysHUD[id];
    } else if (_overlaysWorld.contains(id)) {
        thisOverlay = _overlaysWorld[id];
    }

    if (thisOverlay) {
        return addOverlay(thisOverlay->createClone());
    } 
    
    return 0;  // Not found
}

bool Overlays::editOverlay(unsigned int id, const QScriptValue& properties) {
    QWriteLocker lock(&_lock);
    Overlay::Pointer thisOverlay;

    if (_overlaysHUD.contains(id)) {
        thisOverlay = _overlaysHUD[id];
    } else if (_overlaysWorld.contains(id)) {
        thisOverlay = _overlaysWorld[id];
    }

    if (thisOverlay) {
        if (thisOverlay->is3D()) {
            auto overlay3D = std::static_pointer_cast<Base3DOverlay>(thisOverlay);

            bool oldDrawOnHUD = overlay3D->getDrawOnHUD();
            thisOverlay->setProperties(properties);
            bool drawOnHUD = overlay3D->getDrawOnHUD();

            if (drawOnHUD != oldDrawOnHUD) {
                if (drawOnHUD) {
                    _overlaysWorld.remove(id);
                    _overlaysHUD[id] = thisOverlay;
                } else {
                    _overlaysHUD.remove(id);
                    _overlaysWorld[id] = thisOverlay;
                }
            }
        } else {
            thisOverlay->setProperties(properties);
        }

        return true;
    }
    return false;
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

    QWriteLocker lock(&_deleteLock);
    _overlaysToDelete.push_back(overlayToDelete);
}

unsigned int Overlays::getOverlayAtPoint(const glm::vec2& point) {
    glm::vec2 pointCopy = point;
    if (qApp->isHMDMode()) {
        pointCopy = qApp->getApplicationCompositor().screenToOverlay(point);
    }
    
    QReadLocker lock(&_lock);
    QMapIterator<unsigned int, Overlay::Pointer> i(_overlaysHUD);
    i.toBack();

    const float LARGE_NEGATIVE_FLOAT = -9999999;
    glm::vec3 origin(pointCopy.x, pointCopy.y, LARGE_NEGATIVE_FLOAT);
    glm::vec3 direction(0, 0, 1);
    BoxFace thisFace;
    float distance;

    while (i.hasPrevious()) {
        i.previous();
        unsigned int thisID = i.key();
        if (i.value()->is3D()) {
            auto thisOverlay = std::dynamic_pointer_cast<Base3DOverlay>(i.value());
            if (thisOverlay && !thisOverlay->getIgnoreRayIntersection()) {
                if (thisOverlay->findRayIntersection(origin, direction, distance, thisFace)) {
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
    Overlay::Pointer thisOverlay;
    QReadLocker lock(&_lock);
    if (_overlaysHUD.contains(id)) {
        thisOverlay = _overlaysHUD[id];
    } else if (_overlaysWorld.contains(id)) {
        thisOverlay = _overlaysWorld[id];
    }
    if (thisOverlay) {
        result.value = thisOverlay->getProperty(property);
    }
    return result;
}

OverlayPropertyResult::OverlayPropertyResult() :
    value(QScriptValue())
{
}

QScriptValue OverlayPropertyResultToScriptValue(QScriptEngine* engine, const OverlayPropertyResult& result)
{
    if (!result.value.isValid()) {
        return QScriptValue::UndefinedValue;
    }

    QScriptValue object = engine->newObject();
    if (result.value.isObject()) {
        QScriptValueIterator it(result.value);
        while (it.hasNext()) {
            it.next();
            object.setProperty(it.name(), QScriptValue(it.value().toString()));
        }

    } else {
        object = result.value;
    }
    return object;
}

void OverlayPropertyResultFromScriptValue(const QScriptValue& value, OverlayPropertyResult& result)
{
    result.value = value;
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
            QString thisExtraInfo;
            if (thisOverlay->findRayIntersectionExtraInfo(ray.origin, ray.direction, thisDistance, thisFace, thisExtraInfo)) {
                bool isDrawInFront = thisOverlay->getDrawInFront();
                if (thisDistance < bestDistance && (!bestIsFront || isDrawInFront)) {
                    bestIsFront = isDrawInFront;
                    bestDistance = thisDistance;
                    result.intersects = true;
                    result.distance = thisDistance;
                    result.face = thisFace;
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
    QScriptValue obj = engine->newObject();
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
    QScriptValue intersection = vec3toScriptValue(engine, value.intersection);
    obj.setProperty("intersection", intersection);
    obj.setProperty("extraInfo", value.extraInfo);
    return obj;
}

void RayToOverlayIntersectionResultFromScriptValue(const QScriptValue& object, RayToOverlayIntersectionResult& value) {
    value.intersects = object.property("intersects").toVariant().toBool();
    value.overlayID = object.property("overlayID").toVariant().toInt();
    value.distance = object.property("distance").toVariant().toFloat();

    QString faceName = object.property("face").toVariant().toString();
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
    QScriptValue intersection = object.property("intersection");
    if (intersection.isValid()) {
        vec3FromScriptValue(intersection, value.intersection);
    }
    value.extraInfo = object.property("extraInfo").toVariant().toString();
}

bool Overlays::isLoaded(unsigned int id) {
    QReadLocker lock(&_lock);
    Overlay::Pointer thisOverlay = NULL;
    if (_overlaysHUD.contains(id)) {
        thisOverlay = _overlaysHUD[id];
    } else if (_overlaysWorld.contains(id)) {
        thisOverlay = _overlaysWorld[id];
    } else {
        return false; // not found
    }
    return thisOverlay->isLoaded();
}

QSizeF Overlays::textSize(unsigned int id, const QString& text) const {
    Overlay::Pointer thisOverlay = _overlaysHUD[id];
    if (thisOverlay) {
        if (typeid(*thisOverlay) == typeid(TextOverlay)) {
            return std::dynamic_pointer_cast<TextOverlay>(thisOverlay)->textSize(text);
        }
    } else {
        thisOverlay = _overlaysWorld[id];
        if (thisOverlay) {
            if (typeid(*thisOverlay) == typeid(Text3DOverlay)) {
                return std::dynamic_pointer_cast<Text3DOverlay>(thisOverlay)->textSize(text);
            }
        }
    }
    return QSizeF(0.0f, 0.0f);
}
