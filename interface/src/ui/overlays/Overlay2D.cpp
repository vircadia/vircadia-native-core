//
//  Overlay2D.cpp
//  interface/src/ui/overlays
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// include this before QGLWidget, which includes an earlier version of OpenGL
#include "InterfaceConfig.h"

#include <QSvgRenderer>
#include <QPainter>
#include <QGLWidget>
#include <SharedUtil.h>

#include "Overlay2D.h"


Overlay2D::Overlay2D() {
}

Overlay2D::~Overlay2D() {
}

void Overlay2D::setProperties(const QScriptValue& properties) {
    Overlay::setProperties(properties);
    
    QScriptValue bounds = properties.property("bounds");
    if (bounds.isValid()) {
        QRect boundsRect;
        boundsRect.setX(bounds.property("x").toVariant().toInt());
        boundsRect.setY(bounds.property("y").toVariant().toInt());
        boundsRect.setWidth(bounds.property("width").toVariant().toInt());
        boundsRect.setHeight(bounds.property("height").toVariant().toInt());
        setBounds(boundsRect);
    } else {
        QRect oldBounds = getBounds();
        QRect newBounds = oldBounds;
        
        if (properties.property("x").isValid()) {
            newBounds.setX(properties.property("x").toVariant().toInt());
        } else {
            newBounds.setX(oldBounds.x());
        }
        if (properties.property("y").isValid()) {
            newBounds.setY(properties.property("y").toVariant().toInt());
        } else {
            newBounds.setY(oldBounds.y());
        }
        if (properties.property("width").isValid()) {
            newBounds.setWidth(properties.property("width").toVariant().toInt());
        } else {
            newBounds.setWidth(oldBounds.width());
        }
        if (properties.property("height").isValid()) {
            newBounds.setHeight(properties.property("height").toVariant().toInt());
        } else {
            newBounds.setHeight(oldBounds.height());
        }
        setBounds(newBounds);
        //qDebug() << "set bounds to " << getBounds();
    }
}

QScriptValue Overlay2D::getProperty(const QString& property) {
    if (property == "bounds") {
        QScriptValue bounds = _scriptEngine->newObject();
        bounds.setProperty("x", _bounds.x());
        bounds.setProperty("y", _bounds.y());
        bounds.setProperty("width", _bounds.width());
        bounds.setProperty("height", _bounds.height());
        return bounds;
    }
    if (property == "x") {
        return _bounds.x();
    }
    if (property == "y") {
        return _bounds.y();
    }
    if (property == "width") {
        return _bounds.width();
    }
    if (property == "height") {
        return _bounds.height();
    }

    return Overlay::getProperty(property);
}
