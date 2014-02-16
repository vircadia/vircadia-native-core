//
//  Overlays.cpp
//  interface
//
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//


#include "Overlays.h"

unsigned int Overlays::_nextOverlayID = 1;

Overlays::Overlays() {
}

Overlays::~Overlays() {
}

void Overlays::init(QGLWidget* parent) {
    _parent = parent;
}

void Overlays::render() {
    foreach(ImageOverlay* thisOverlay, _imageOverlays) {
        thisOverlay->render();
    }
}

// TODO: make multi-threaded safe
unsigned int Overlays::addOverlay(const QScriptValue& properties) {
    unsigned int thisID = _nextOverlayID;
    _nextOverlayID++;
    ImageOverlay* thisOverlay = new ImageOverlay();
    thisOverlay->init(_parent);
    thisOverlay->setProperties(properties);
    _imageOverlays[thisID] = thisOverlay;
    return thisID; 
}

// TODO: make multi-threaded safe
bool Overlays::editOverlay(unsigned int id, const QScriptValue& properties) {
    if (!_imageOverlays.contains(id)) {
        return false;
    }
    ImageOverlay* thisOverlay = _imageOverlays[id];
    thisOverlay->setProperties(properties);
    return true;
}

// TODO: make multi-threaded safe
void Overlays::deleteOverlay(unsigned int id) {
    if (_imageOverlays.contains(id)) {
        _imageOverlays.erase(_imageOverlays.find(id));
    }
}

unsigned int Overlays::getOverlayAtPoint(const glm::vec2& point) {
    QMapIterator<unsigned int, ImageOverlay*> i(_imageOverlays);
    i.toBack();
    while (i.hasPrevious()) {
        i.previous();
        unsigned int thisID = i.key();
        ImageOverlay* thisOverlay = i.value();
        if (thisOverlay->getBounds().contains(point.x, point.y, false)) {
            return thisID;
        }
    }
    return 0; // not found
}


