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
    _backgroundColor(DEFAULT_BACKGROUND_COLOR),
    _visible(true)
{
}

void Overlay::init(QGLWidget* parent) {
    _parent = parent;
}


Overlay::~Overlay() {
}

void Overlay::setProperties(const QScriptValue& properties) {
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

    QScriptValue color = properties.property("backgroundColor");
    if (color.isValid()) {
        QScriptValue red = color.property("red");
        QScriptValue green = color.property("green");
        QScriptValue blue = color.property("blue");
        if (red.isValid() && green.isValid() && blue.isValid()) {
            _backgroundColor.red = red.toVariant().toInt();
            _backgroundColor.green = green.toVariant().toInt();
            _backgroundColor.blue = blue.toVariant().toInt();
        }
    }

    if (properties.property("alpha").isValid()) {
        setAlpha(properties.property("alpha").toVariant().toFloat());
    }

    if (properties.property("visible").isValid()) {
        setVisible(properties.property("visible").toVariant().toBool());
    }
}
