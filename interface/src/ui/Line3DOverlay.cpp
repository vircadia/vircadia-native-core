//
//  Line3DOverlay.cpp
//  interface
//
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

// include this before QGLWidget, which includes an earlier version of OpenGL
#include "InterfaceConfig.h"

#include "Line3DOverlay.h"


Line3DOverlay::Line3DOverlay() {
}

Line3DOverlay::~Line3DOverlay() {
}

void Line3DOverlay::render() {
    if (!_visible) {
        return; // do nothing if we're not visible
    }

    const float MAX_COLOR = 255;
    glDisable(GL_LIGHTING);
    glLineWidth(_lineWidth);
    glColor4f(_color.red / MAX_COLOR, _color.green / MAX_COLOR, _color.blue / MAX_COLOR, _alpha);

    glBegin(GL_LINES);
        glVertex3f(_position.x, _position.y, _position.z);
        glVertex3f(_end.x, _end.y, _end.z);
    glEnd();
    glEnable(GL_LIGHTING);
}

void Line3DOverlay::setProperties(const QScriptValue& properties) {
    Base3DOverlay::setProperties(properties);

    QScriptValue end = properties.property("end");
    // if "end" property was not there, check to see if they included aliases: endPoint, or p2
    if (!end.isValid()) {
        end = properties.property("endPoint");
        if (!end.isValid()) {
            end = properties.property("p2");
        }
    }
    if (end.isValid()) {
        QScriptValue x = end.property("x");
        QScriptValue y = end.property("y");
        QScriptValue z = end.property("z");
        if (x.isValid() && y.isValid() && z.isValid()) {
            glm::vec3 newEnd;
            newEnd.x = x.toVariant().toFloat();
            newEnd.y = y.toVariant().toFloat();
            newEnd.z = z.toVariant().toFloat();
            setEnd(newEnd);
        }
    }
}
