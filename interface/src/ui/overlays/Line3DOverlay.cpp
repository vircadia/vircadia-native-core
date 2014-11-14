//
//  Line3DOverlay.cpp
//  interface/src/ui/overlays
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// include this before QGLWidget, which includes an earlier version of OpenGL
#include "InterfaceConfig.h"

#include "Line3DOverlay.h"
#include "renderer/GlowEffect.h"


Line3DOverlay::Line3DOverlay() {
}

Line3DOverlay::Line3DOverlay(Line3DOverlay* line3DOverlay) :
    Base3DOverlay(line3DOverlay),
    _end(line3DOverlay->_end)
{
}

Line3DOverlay::~Line3DOverlay() {
}

void Line3DOverlay::render(RenderArgs* args) {
    if (!_visible) {
        return; // do nothing if we're not visible
    }

    float glowLevel = getGlowLevel();
    Glower* glower = NULL;
    if (glowLevel > 0.0f) {
        glower = new Glower(glowLevel);
    }

    glDisable(GL_LIGHTING);
    glLineWidth(_lineWidth);

    float alpha = getAlpha();
    xColor color = getColor();
    const float MAX_COLOR = 255.0f;
    glColor4f(color.red / MAX_COLOR, color.green / MAX_COLOR, color.blue / MAX_COLOR, alpha);

    if (getIsDashedLine()) {
        drawDashedLine(_position, _end);
    } else {
        glBegin(GL_LINES);
        glVertex3f(_position.x, _position.y, _position.z);
        glVertex3f(_end.x, _end.y, _end.z);
        glEnd();
    }
    glEnable(GL_LIGHTING);

    if (glower) {
        delete glower;
    }
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

QScriptValue Line3DOverlay::getProperty(const QString& property) {
    if (property == "end" || property == "endPoint" || property == "p2") {
        return vec3toScriptValue(_scriptEngine, _end);
    }

    return Base3DOverlay::getProperty(property);
}

Line3DOverlay* Line3DOverlay::createClone() {
    return new Line3DOverlay(this);
}
