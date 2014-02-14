//
//  Overlays.cpp
//  interface
//
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//


#include "Overlays.h"

unsigned int Overlays::_nextOverlayID = 0;

Overlays::Overlays() {
}

Overlays::~Overlays() {
}

void Overlays::init(QGLWidget* parent) {
    qDebug() << "Overlays::init() parent=" << parent;
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

QScriptValue Overlays::getOverlayProperties(unsigned int id) {
    if (!_imageOverlays.contains(id)) {
        return QScriptValue();
    }
    ImageOverlay* thisOverlay = _imageOverlays[id];
    return thisOverlay->getProperties();
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

