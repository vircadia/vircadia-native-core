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

#include <GlowEffect.h>
#include <GeometryCache.h>

#include "Line3DOverlay.h"


Line3DOverlay::Line3DOverlay() {
}

Line3DOverlay::Line3DOverlay(const Line3DOverlay* line3DOverlay) :
    Base3DOverlay(line3DOverlay),
    _end(line3DOverlay->_end),
    _geometryCacheID(DependencyManager::get<GeometryCache>()->allocateID())
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

    glPushMatrix();

    glDisable(GL_LIGHTING);
    glLineWidth(_lineWidth);

    float alpha = getAlpha();
    xColor color = getColor();
    const float MAX_COLOR = 255.0f;
    glm::vec4 colorv4(color.red / MAX_COLOR, color.green / MAX_COLOR, color.blue / MAX_COLOR, alpha);

    glm::vec3 position = getPosition();
    glm::quat rotation = getRotation();

    glTranslatef(position.x, position.y, position.z);
    glm::vec3 axis = glm::axis(rotation);
    glRotatef(glm::degrees(glm::angle(rotation)), axis.x, axis.y, axis.z);

    if (getIsDashedLine()) {
        // TODO: add support for color to renderDashedLine()
        DependencyManager::get<GeometryCache>()->renderDashedLine(_position, _end, colorv4, _geometryCacheID);
    } else {
        DependencyManager::get<GeometryCache>()->renderLine(_start, _end, colorv4, _geometryCacheID);
    }
    glEnable(GL_LIGHTING);

    glPopMatrix();

    if (glower) {
        delete glower;
    }
}

void Line3DOverlay::setProperties(const QScriptValue& properties) {
    Base3DOverlay::setProperties(properties);

    QScriptValue start = properties.property("start");
    // if "start" property was not there, check to see if they included aliases: startPoint
    if (!start.isValid()) {
        start = properties.property("startPoint");
    }
    if (start.isValid()) {
        QScriptValue x = start.property("x");
        QScriptValue y = start.property("y");
        QScriptValue z = start.property("z");
        if (x.isValid() && y.isValid() && z.isValid()) {
            glm::vec3 newStart;
            newStart.x = x.toVariant().toFloat();
            newStart.y = y.toVariant().toFloat();
            newStart.z = z.toVariant().toFloat();
            setStart(newStart);
        }
    }

    QScriptValue end = properties.property("end");
    // if "end" property was not there, check to see if they included aliases: endPoint
    if (!end.isValid()) {
        end = properties.property("endPoint");
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

Line3DOverlay* Line3DOverlay::createClone() const {
    return new Line3DOverlay(this);
}
