//
//  Overlay.cpp
//  interface
//
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
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
    _alpha(DEFAULT_ALPHA),
    _color(DEFAULT_BACKGROUND_COLOR),
    _visible(true)
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
}
