//
//  Base3DOverlay.cpp
//  interface
//
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

// include this before QGLWidget, which includes an earlier version of OpenGL
#include "InterfaceConfig.h"

#include <QGLWidget>
#include <SharedUtil.h>

#include "Base3DOverlay.h"
#include "TextRenderer.h"

const glm::vec3 DEFAULT_POSITION = glm::vec3(0.0f, 0.0f, 0.0f);
const float DEFAULT_LINE_WIDTH = 1.0f;

Base3DOverlay::Base3DOverlay() :
    _position(DEFAULT_POSITION),
    _lineWidth(DEFAULT_LINE_WIDTH)
{
}

Base3DOverlay::~Base3DOverlay() {
}

void Base3DOverlay::setProperties(const QScriptValue& properties) {
    Overlay::setProperties(properties);

    QScriptValue position = properties.property("position");

    // if "position" property was not there, check to see if they included aliases: start, point, p1
    if (!position.isValid()) {
        position = properties.property("start");
        if (!position.isValid()) {
            position = properties.property("p1");
            if (!position.isValid()) {
                position = properties.property("point");
            }
        }
    }

    if (position.isValid()) {
        QScriptValue x = position.property("x");
        QScriptValue y = position.property("y");
        QScriptValue z = position.property("z");
        if (x.isValid() && y.isValid() && z.isValid()) {
            glm::vec3 newPosition;
            newPosition.x = x.toVariant().toFloat();
            newPosition.y = y.toVariant().toFloat();
            newPosition.z = z.toVariant().toFloat();
            setPosition(newPosition);
        }
    }

    if (properties.property("lineWidth").isValid()) {
        setLineWidth(properties.property("lineWidth").toVariant().toFloat());
    }
}
