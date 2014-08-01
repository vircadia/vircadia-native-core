//
//  Overlay.cpp
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

#include "Overlay.h"


Overlay::Overlay() :
    _parent(NULL),
    _isLoaded(true),
    _alpha(DEFAULT_ALPHA),
    _color(DEFAULT_OVERLAY_COLOR),
    _visible(true),
    _anchor(NO_ANCHOR)
{
}

void Overlay::init(QGLWidget* parent) {
    _parent = parent;
}


Overlay::~Overlay() {
}

void Overlay::setProperties(const QScriptValue& properties) {
    QScriptValue color = properties.property("color");
    if (color.isValid()) {
        QScriptValue red = color.property("red");
        QScriptValue green = color.property("green");
        QScriptValue blue = color.property("blue");
        if (red.isValid() && green.isValid() && blue.isValid()) {
            _color.red = red.toVariant().toInt();
            _color.green = green.toVariant().toInt();
            _color.blue = blue.toVariant().toInt();
        }
    }

    if (properties.property("alpha").isValid()) {
        setAlpha(properties.property("alpha").toVariant().toFloat());
    }
    
    if (properties.property("visible").isValid()) {
        setVisible(properties.property("visible").toVariant().toBool());
    }
    
    if (properties.property("anchor").isValid()) {
        QString property = properties.property("anchor").toVariant().toString();
        if (property == "MyAvatar") {
            setAnchor(MY_AVATAR);
        }
    }
}
