//
//  Overlays.cpp
//  interface
//
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//


#include "Cube3DOverlay.h"
#include "ImageOverlay.h"
#include "Line3DOverlay.h"
#include "Overlays.h"
#include "Sphere3DOverlay.h"
#include "TextOverlay.h"
#include "LocalVoxelsOverlay.h"

Overlays::Overlays() : _nextOverlayID(1) {
}

Overlays::~Overlays() {
    QMap<unsigned int, Overlay*>::iterator it;
    for (it = _overlays2D.begin(); it != _overlays2D.end(); ++it) {
        delete _overlays2D.take(it.key());
    }
    for (it = _overlays3D.begin(); it != _overlays3D.end(); ++it) {
        delete _overlays3D.take(it.key());
    }
    while (!_overlaysToDelete.isEmpty()) {
        delete _overlaysToDelete.takeLast();
    }
}

void Overlays::init(QGLWidget* parent) {
    _parent = parent;
}

void Overlays::update(float deltatime) {
    foreach (Overlay* thisOverlay, _overlays2D) {
        thisOverlay->update(deltatime);
    }
    foreach (Overlay* thisOverlay, _overlays3D) {
        thisOverlay->update(deltatime);
    }
    while (!_overlaysToDelete.isEmpty()) {
        delete _overlaysToDelete.takeLast();
    }
    
}

void Overlays::render2D() {
    foreach(Overlay* thisOverlay, _overlays2D) {
        thisOverlay->render();
    }
}

void Overlays::render3D() {
    foreach(Overlay* thisOverlay, _overlays3D) {
        thisOverlay->render();
    }
}

// TODO: make multi-threaded safe
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
    }

    if (created) {
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

// TODO: make multi-threaded safe
bool Overlays::editOverlay(unsigned int id, const QScriptValue& properties) {
    Overlay* thisOverlay = NULL;
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

// TODO: make multi-threaded safe
void Overlays::deleteOverlay(unsigned int id) {
    Overlay* overlayToDelete;
    if (_overlays2D.contains(id)) {
        overlayToDelete = _overlays2D.take(id);
    } else if (_overlays3D.contains(id)) {
        overlayToDelete = _overlays3D.take(id);
    } else {
        return;
    }
    
    _overlaysToDelete.push_back(overlayToDelete);
}

unsigned int Overlays::getOverlayAtPoint(const glm::vec2& point) {
    QMapIterator<unsigned int, Overlay*> i(_overlays2D);
    i.toBack();
    while (i.hasPrevious()) {
        i.previous();
        unsigned int thisID = i.key();
        Overlay2D* thisOverlay = static_cast<Overlay2D*>(i.value());
        if (thisOverlay->getVisible() && thisOverlay->getBounds().contains(point.x, point.y, false)) {
            return thisID;
        }
    }
    return 0; // not found
}


