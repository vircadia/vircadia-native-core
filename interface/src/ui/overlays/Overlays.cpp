//
//  Overlays.cpp
//  interface/src/ui/overlays
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <Application.h>

#include "BillboardOverlay.h"
#include "Cube3DOverlay.h"
#include "ImageOverlay.h"
#include "Line3DOverlay.h"
#include "LocalVoxelsOverlay.h"
#include "ModelOverlay.h"
#include "Overlays.h"
#include "Sphere3DOverlay.h"
#include "TextOverlay.h"

Overlays::Overlays() : _nextOverlayID(1) {
}

Overlays::~Overlays() {
    
    {
        QWriteLocker lock(&_lock);
        foreach(Overlay* thisOverlay, _overlays2D) {
            delete thisOverlay;
        }
        _overlays2D.clear();
        foreach(Overlay* thisOverlay, _overlays3D) {
            delete thisOverlay;
        }
        _overlays3D.clear();
    }
    
    if (!_overlaysToDelete.isEmpty()) {
        QWriteLocker lock(&_deleteLock);
        do {
            delete _overlaysToDelete.takeLast();
        } while (!_overlaysToDelete.isEmpty());
    }
    
}

void Overlays::init(QGLWidget* parent) {
    _parent = parent;
}

void Overlays::update(float deltatime) {

    {
        QWriteLocker lock(&_lock);
        foreach(Overlay* thisOverlay, _overlays2D) {
            thisOverlay->update(deltatime);
        }
        foreach(Overlay* thisOverlay, _overlays3D) {
            thisOverlay->update(deltatime);
        }
    }

    if (!_overlaysToDelete.isEmpty()) {
        QWriteLocker lock(&_deleteLock);
        do {
            delete _overlaysToDelete.takeLast();
        } while (!_overlaysToDelete.isEmpty());
    }
    
}

void Overlays::render2D() {
    QReadLocker lock(&_lock);
    foreach(Overlay* thisOverlay, _overlays2D) {
        thisOverlay->render();
    }
}

void Overlays::render3D() {
    QReadLocker lock(&_lock);
    if (_overlays3D.size() == 0) {
        return;
    }
    bool myAvatarComputed = false;
    MyAvatar* avatar = NULL;
    glm::quat myAvatarRotation;
    glm::vec3 myAvatarPosition(0.0f);
    float angle = 0.0f;
    glm::vec3 axis(0.0f, 1.0f, 0.0f);
    float myAvatarScale = 1.0f;

    foreach(Overlay* thisOverlay, _overlays3D) {
        glPushMatrix();
        switch (thisOverlay->getAnchor()) {
            case Overlay::MY_AVATAR:
                if (!myAvatarComputed) {
                    avatar = Application::getInstance()->getAvatar();
                    myAvatarRotation = avatar->getOrientation();
                    myAvatarPosition = avatar->getPosition();
                    angle = glm::degrees(glm::angle(myAvatarRotation));
                    axis = glm::axis(myAvatarRotation);
                    myAvatarScale = avatar->getScale();
                    
                    myAvatarComputed = true;
                }
                
                glTranslatef(myAvatarPosition.x, myAvatarPosition.y, myAvatarPosition.z);
                glRotatef(angle, axis.x, axis.y, axis.z);
                glScalef(myAvatarScale, myAvatarScale, myAvatarScale);
                break;
            default:
                break;
        }
        thisOverlay->render();
        glPopMatrix();
    }
}

unsigned int Overlays::addOverlay(const QString& type, const QScriptValue& properties) {
    unsigned int thisID = 0;
    bool created = false;
    bool is3D = false;
    Overlay* thisOverlay = NULL;
    
    if (type == "image") {
        thisOverlay = new ImageOverlay();
        thisOverlay->init(_parent);
        thisOverlay->setProperties(properties);
        created = true;
    } else if (type == "text") {
        thisOverlay = new TextOverlay();
        thisOverlay->init(_parent);
        thisOverlay->setProperties(properties);
        created = true;
    } else if (type == "cube") {
        thisOverlay = new Cube3DOverlay();
        thisOverlay->init(_parent);
        thisOverlay->setProperties(properties);
        created = true;
        is3D = true;
    } else if (type == "sphere") {
        thisOverlay = new Sphere3DOverlay();
        thisOverlay->init(_parent);
        thisOverlay->setProperties(properties);
        created = true;
        is3D = true;
    } else if (type == "line3d") {
        thisOverlay = new Line3DOverlay();
        thisOverlay->init(_parent);
        thisOverlay->setProperties(properties);
        created = true;
        is3D = true;
    } else if (type == "localvoxels") {
        thisOverlay = new LocalVoxelsOverlay();
        thisOverlay->init(_parent);
        thisOverlay->setProperties(properties);
        created = true;
        is3D = true;
    } else if (type == "model") {
        thisOverlay = new ModelOverlay();
        thisOverlay->init(_parent);
        thisOverlay->setProperties(properties);
        created = true;
        is3D = true;
    } else if (type == "billboard") {
        thisOverlay = new BillboardOverlay();
        thisOverlay->init(_parent);
        thisOverlay->setProperties(properties);
        created = true;
        is3D = true;
    }

    if (created) {
        QWriteLocker lock(&_lock);
        thisID = _nextOverlayID;
        _nextOverlayID++;
        if (is3D) {
            _overlays3D[thisID] = thisOverlay;
        } else {
            _overlays2D[thisID] = thisOverlay;
        }
    }

    return thisID; 
}

bool Overlays::editOverlay(unsigned int id, const QScriptValue& properties) {
    Overlay* thisOverlay = NULL;
    QWriteLocker lock(&_lock);
    if (_overlays2D.contains(id)) {
        thisOverlay = _overlays2D[id];
    } else if (_overlays3D.contains(id)) {
        thisOverlay = _overlays3D[id];
    }
    if (thisOverlay) {
        thisOverlay->setProperties(properties);
        return true;
    }
    return false;
}

void Overlays::deleteOverlay(unsigned int id) {
    Overlay* overlayToDelete;

    {
        QWriteLocker lock(&_lock);
        if (_overlays2D.contains(id)) {
            overlayToDelete = _overlays2D.take(id);
        } else if (_overlays3D.contains(id)) {
            overlayToDelete = _overlays3D.take(id);
        } else {
            return;
        }
    }

    QWriteLocker lock(&_deleteLock);
    _overlaysToDelete.push_back(overlayToDelete);
}

unsigned int Overlays::getOverlayAtPoint(const glm::vec2& point) {
    QReadLocker lock(&_lock);
    QMapIterator<unsigned int, Overlay*> i(_overlays2D);
    i.toBack();
    while (i.hasPrevious()) {
        i.previous();
        unsigned int thisID = i.key();
        Overlay2D* thisOverlay = static_cast<Overlay2D*>(i.value());
        if (thisOverlay->getVisible() && thisOverlay->isLoaded() && thisOverlay->getBounds().contains(point.x, point.y, false)) {
            return thisID;
        }
    }
    return 0; // not found
}

bool Overlays::isLoaded(unsigned int id) {
    QReadLocker lock(&_lock);
    Overlay* overlay = _overlays2D.value(id);
    if (!overlay) {
        _overlays3D.value(id);
    }
    if (!overlay) {
        return false; // not found
    }

    return overlay->isLoaded();
}

