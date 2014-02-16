//
//  Cube3DOverlay.cpp
//  interface
//
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

// include this before QGLWidget, which includes an earlier version of OpenGL
#include "InterfaceConfig.h"

#include <QGLWidget>
#include <SharedUtil.h>

#include "Cube3DOverlay.h"
#include "TextRenderer.h"

const glm::vec3 DEFAULT_POSITION = glm::vec3(0.0f, 0.0f, 0.0f);
const float DEFAULT_SIZE = 1.0f;
const float DEFAULT_LINE_WIDTH = 1.0f;
const bool DEFAULT_isSolid = false;

Cube3DOverlay::Cube3DOverlay() :
    _position(DEFAULT_POSITION),
    _size(DEFAULT_SIZE),
    _lineWidth(DEFAULT_LINE_WIDTH),
    _isSolid(DEFAULT_isSolid)
{
}

Cube3DOverlay::~Cube3DOverlay() {
}

void Cube3DOverlay::render() {
    if (!_visible) {
        return; // do nothing if we're not visible
    }

    const float MAX_COLOR = 255;
    glColor4f(_color.red / MAX_COLOR, _color.green / MAX_COLOR, _color.blue / MAX_COLOR, _alpha);


    glDisable(GL_LIGHTING);
    glPushMatrix();
    glTranslatef(_position.x + _size * 0.5f,
                 _position.y + _size * 0.5f,
                 _position.z + _size * 0.5f);
    glLineWidth(_lineWidth);
    if (_isSolid) {
        glutSolidCube(_size);
    } else {
        glutWireCube(_size);
    }
    glPopMatrix();

}

void Cube3DOverlay::setProperties(const QScriptValue& properties) {
    Overlay::setProperties(properties);

    QScriptValue position = properties.property("position");
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

    if (properties.property("size").isValid()) {
        setSize(properties.property("size").toVariant().toFloat());
    }

    if (properties.property("lineWidth").isValid()) {
        setLineWidth(properties.property("lineWidth").toVariant().toFloat());
    }

    if (properties.property("isSolid").isValid()) {
        setIsSolid(properties.property("isSolid").toVariant().toBool());
    }
    if (properties.property("isWire").isValid()) {
        setIsSolid(!properties.property("isWire").toVariant().toBool());
    }
    if (properties.property("solid").isValid()) {
        setIsSolid(properties.property("solid").toVariant().toBool());
    }
    if (properties.property("wire").isValid()) {
        setIsSolid(!properties.property("wire").toVariant().toBool());
    }


}


