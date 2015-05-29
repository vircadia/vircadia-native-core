//
//  Overlays.cpp
//  interface/src/ui/overlays
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QScriptValueIterator>

#include <limits>
#include <typeinfo>

#include <Application.h>
#include <avatar/AvatarManager.h>
#include <LODManager.h>
#include <render/Scene.h>

#include "BillboardOverlay.h"
#include "Circle3DOverlay.h"
#include "Cube3DOverlay.h"
#include "ImageOverlay.h"
#include "Line3DOverlay.h"
#include "LocalModelsOverlay.h"
#include "ModelOverlay.h"
#include "Overlays.h"
#include "Rectangle3DOverlay.h"
#include "Sphere3DOverlay.h"
#include "Grid3DOverlay.h"
#include "TextOverlay.h"
#include "Text3DOverlay.h"


namespace render {
    template <> const ItemKey payloadGetKey(const Overlay::Pointer& overlay) {
        if (overlay->is3D() && !static_cast<Base3DOverlay*>(overlay.get())->getDrawOnHUD()) {
            if (static_cast<Base3DOverlay*>(overlay.get())->getDrawInFront()) {
                return ItemKey::Builder().withTypeShape().withNoDepthSort().build();
            } else {
                return ItemKey::Builder::opaqueShape();
            }
        } else {
            return ItemKey::Builder().withTypeShape().withViewSpace().build();
        }
    }
    template <> const Item::Bound payloadGetBound(const Overlay::Pointer& overlay) {
        if (overlay->is3D()) {
            return static_cast<Base3DOverlay*>(overlay.get())->getBounds();
        } else {
            QRect bounds = static_cast<Overlay2D*>(overlay.get())->getBounds();
            return AABox(glm::vec3(bounds.x(), bounds.y(), 0.0f), glm::vec3(bounds.width(), bounds.height(), 0.1f));
        }
    }
    template <> void payloadRender(const Overlay::Pointer& overlay, RenderArgs* args) {
        if (args) {
            args->_elementsTouched++;

            glPushMatrix();
            if (overlay->getAnchor() == Overlay::MY_AVATAR) {
                MyAvatar* avatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
                glm::quat myAvatarRotation = avatar->getOrientation();
                glm::vec3 myAvatarPosition = avatar->getPosition();
                float angle = glm::degrees(glm::angle(myAvatarRotation));
                glm::vec3 axis = glm::axis(myAvatarRotation);
                float myAvatarScale = avatar->getScale();

                glTranslatef(myAvatarPosition.x, myAvatarPosition.y, myAvatarPosition.z);
                glRotatef(angle, axis.x, axis.y, axis.z);
                glScalef(myAvatarScale, myAvatarScale, myAvatarScale);
            }
            overlay->render(args);
            glPopMatrix();
        }
    }
}

Overlays::Overlays() : _nextOverlayID(1) {
}

Overlays::~Overlays() {
    
    {
        QWriteLocker lock(&_lock);
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
        QWriteLocker lock(&_deleteLock);
        render::PendingChanges pendingChanges;

        do {
            Overlay::Pointer overlay = _overlaysToDelete.takeLast();

            auto itemID = overlay->getRenderItemID();
            if (itemID != render::Item::INVALID_ITEM_ID) {
                pendingChanges.removeItem(itemID);
            }
        } while (!_overlaysToDelete.isEmpty());

        if (pendingChanges._removedItems.size() > 0) {
            render::ScenePointer scene = Application::getInstance()->getMain3DScene();
            scene->enqueuePendingChanges(pendingChanges);
        }
    }
}

void Overlays::renderHUD(RenderArgs* renderArgs) {
    QReadLocker lock(&_lock);
    
    auto lodManager = DependencyManager::get<LODManager>();

    foreach(Overlay::Pointer thisOverlay, _overlaysHUD) {
        if (thisOverlay->is3D()) {
            glEnable(GL_DEPTH_TEST);
            glEnable(GL_LIGHTING);

            thisOverlay->render(renderArgs);

            glDisable(GL_LIGHTING);
            glDisable(GL_DEPTH_TEST);
        } else {
            thisOverlay->render(renderArgs);
        }
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
            auto overlayPayload = new Overlay::Payload(overlayPointer);
            auto overlayPayloadPointer = Overlay::PayloadPointer(overlayPayload);
            render::ItemID itemID = scene->allocateID();
            overlay->setRenderItemID(itemID);

            render::PendingChanges pendingChanges;
            pendingChanges.resetItem(itemID, overlayPayloadPointer);

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
            Base3DOverlay* overlay3D = static_cast<Base3DOverlay*>(thisOverlay.get());

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
        pointCopy = qApp->getApplicationOverlay().screenToOverlay(point);
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
            Base3DOverlay* thisOverlay = static_cast<Base3DOverlay*>(i.value().get());
            if (!thisOverlay->getIgnoreRayIntersection()) {
                if (thisOverlay->findRayIntersection(origin, direction, distance, thisFace)) {
                    return thisID;
                }
            }
        } else {
            Overlay2D* thisOverlay = static_cast<Overlay2D*>(i.value().get());
            if (thisOverlay->getVisible() && thisOverlay->isLoaded() &&
                thisOverlay->getBounds().contains(pointCopy.x, pointCopy.y, false)) {
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
        Base3DOverlay* thisOverlay = static_cast<Base3DOverlay*>(i.value().get());
        if (thisOverlay->getVisible() && !thisOverlay->getIgnoreRayIntersection() && thisOverlay->isLoaded()) {
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
            return static_cast<TextOverlay*>(thisOverlay.get())->textSize(text);
        }
    } else {
        thisOverlay = _overlaysWorld[id];
        if (thisOverlay) {
            if (typeid(*thisOverlay) == typeid(Text3DOverlay)) {
                return static_cast<Text3DOverlay*>(thisOverlay.get())->textSize(text);
            }
        }
    }
    return QSizeF(0.0f, 0.0f);
}
