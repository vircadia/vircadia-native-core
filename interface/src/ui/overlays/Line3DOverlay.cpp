//
//  Line3DOverlay.cpp
//  interface/src/ui/overlays
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Line3DOverlay.h"

#include <GeometryCache.h>
#include <RegisteredMetaTypes.h>

Line3DOverlay::Line3DOverlay() :
    _geometryCacheID(DependencyManager::get<GeometryCache>()->allocateID())
{
}

Line3DOverlay::Line3DOverlay(const Line3DOverlay* line3DOverlay) :
    Base3DOverlay(line3DOverlay),
    _end(line3DOverlay->_end),
    _geometryCacheID(DependencyManager::get<GeometryCache>()->allocateID())
{
}

Line3DOverlay::~Line3DOverlay() {
}

AABox Line3DOverlay::getBounds() const {
    auto extents = Extents{};
    extents.addPoint(_start);
    extents.addPoint(_end);
    extents.transform(_transform);
    
    return AABox(extents);
}

void Line3DOverlay::render(RenderArgs* args) {
    if (!_visible) {
        return; // do nothing if we're not visible
    }

    float alpha = getAlpha();
    xColor color = getColor();
    const float MAX_COLOR = 255.0f;
    glm::vec4 colorv4(color.red / MAX_COLOR, color.green / MAX_COLOR, color.blue / MAX_COLOR, alpha);

    auto batch = args->_batch;
    if (batch) {
        batch->setModelTransform(_transform);

        if (getIsDashedLine()) {
            // TODO: add support for color to renderDashedLine()
            DependencyManager::get<GeometryCache>()->renderDashedLine(*batch, _start, _end, colorv4, _geometryCacheID);
        } else {
            DependencyManager::get<GeometryCache>()->renderLine(*batch, _start, _end, colorv4, _geometryCacheID);
        }
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
