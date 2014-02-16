//
//  Overlays.cpp
//  interface
//
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//


#include "Overlays.h"
#include "ImageOverlay.h"
#include "TextOverlay.h"


unsigned int Overlays::_nextOverlayID = 1;

Overlays::Overlays() {
}

Overlays::~Overlays() {
}

void Overlays::init(QGLWidget* parent) {
    _parent = parent;
}

void Overlays::render() {
    foreach(Overlay* thisOverlay, _overlays) {
        thisOverlay->render();
    }
}

// TODO: make multi-threaded safe
unsigned int Overlays::addOverlay(const QString& type, const QScriptValue& properties) {
    unsigned int thisID = 0;
    
    if (type == "image") {
        thisID = _nextOverlayID;
        _nextOverlayID++;
        ImageOverlay* thisOverlay = new ImageOverlay();
        thisOverlay->init(_parent);
        thisOverlay->setProperties(properties);
        _overlays[thisID] = thisOverlay;
    } else if (type == "text") {
        thisID = _nextOverlayID;
        _nextOverlayID++;
        TextOverlay* thisOverlay = new TextOverlay();
        thisOverlay->init(_parent);
        thisOverlay->setProperties(properties);
        _overlays[thisID] = thisOverlay;
    }

    return thisID; 
}

// TODO: make multi-threaded safe
bool Overlays::editOverlay(unsigned int id, const QScriptValue& properties) {
    if (!_overlays.contains(id)) {
        return false;
    }
    Overlay* thisOverlay = _overlays[id];
    thisOverlay->setProperties(properties);
    return true;
}

// TODO: make multi-threaded safe
void Overlays::deleteOverlay(unsigned int id) {
    if (_overlays.contains(id)) {
        _overlays.erase(_overlays.find(id));
    }
}

unsigned int Overlays::getOverlayAtPoint(const glm::vec2& point) {
    QMapIterator<unsigned int, Overlay*> i(_overlays);
    i.toBack();
    while (i.hasPrevious()) {
        i.previous();
        unsigned int thisID = i.key();
        Overlay* thisOverlay = i.value();
        if (thisOverlay->getVisible() && thisOverlay->getBounds().contains(point.x, point.y, false)) {
            return thisID;
        }
    }
    return 0; // not found
}


