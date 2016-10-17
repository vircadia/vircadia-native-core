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


QString const Line3DOverlay::TYPE = "line3d";

Line3DOverlay::Line3DOverlay() :
    _geometryCacheID(DependencyManager::get<GeometryCache>()->allocateID())
{
    qDebug() << "Building line3D overlay";
}

Line3DOverlay::Line3DOverlay(const Line3DOverlay* line3DOverlay) :
    Base3DOverlay(line3DOverlay),
    _start(line3DOverlay->_start),
    _end(line3DOverlay->_end),
    _geometryCacheID(DependencyManager::get<GeometryCache>()->allocateID())
{
    qDebug() << "Building line3D overlay";
}

Line3DOverlay::~Line3DOverlay() {
    qDebug() << "Destryoing line3D overlay";
    auto geometryCache = DependencyManager::get<GeometryCache>();
    if (_geometryCacheID) {
        geometryCache->releaseID(_geometryCacheID);
    }
}

glm::vec3 Line3DOverlay::getStart() const {
    bool success;
    glm::vec3 worldStart = localToWorld(_start, _parentID, _parentJointIndex, success);
    if (!success) {
        qDebug() << "Line3DOverlay::getStart failed";
    }
    return worldStart;
}

glm::vec3 Line3DOverlay::getEnd() const {
    bool success;
    glm::vec3 worldEnd = localToWorld(_end, _parentID, _parentJointIndex, success);
    if (!success) {
        qDebug() << "Line3DOverlay::getEnd failed";
    }
    return worldEnd;
}

void Line3DOverlay::setStart(const glm::vec3& start) {
    bool success;
    _start = worldToLocal(start, _parentID, _parentJointIndex, success);
    if (!success) {
        qDebug() << "Line3DOverlay::setStart failed";
    }
}

void Line3DOverlay::setEnd(const glm::vec3& end) {
    bool success;
    _end = worldToLocal(end, _parentID, _parentJointIndex, success);
    if (!success) {
        qDebug() << "Line3DOverlay::setEnd failed";
    }
}

AABox Line3DOverlay::getBounds() const {
    auto extents = Extents{};
    extents.addPoint(_start);
    extents.addPoint(_end);
    extents.transform(getTransform());

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
        batch->setModelTransform(getTransform());

        auto geometryCache = DependencyManager::get<GeometryCache>();
        if (getIsDashedLine()) {
            // TODO: add support for color to renderDashedLine()
            geometryCache->bindSimpleProgram(*batch, false, false, false, true, true);
            geometryCache->renderDashedLine(*batch, _start, _end, colorv4, _geometryCacheID);
        } else if (_glow > 0.0f) {
            geometryCache->renderGlowLine(*batch, _start, _end, colorv4, _glow, _glowWidth, _geometryCacheID);
        } else {
            geometryCache->bindSimpleProgram(*batch, false, false, false, true, true);
            geometryCache->renderLine(*batch, _start, _end, colorv4, _geometryCacheID);
        }
    }
}

const render::ShapeKey Line3DOverlay::getShapeKey() {
    auto builder = render::ShapeKey::Builder().withOwnPipeline();
    if (getAlpha() != 1.0f || _glow > 0.0f) {
        builder.withTranslucent();
    }
    return builder.build();
}

void Line3DOverlay::setProperties(const QVariantMap& originalProperties) {
    QVariantMap properties = originalProperties;

    auto start = properties["start"];
    // if "start" property was not there, check to see if they included aliases: startPoint
    if (!start.isValid()) {
        start = properties["startPoint"];
    }
    if (start.isValid()) {
        setStart(vec3FromVariant(start));
    }
    properties.remove("start"); // so that Base3DOverlay doesn't respond to it

    auto localStart = properties["localStart"];
    if (localStart.isValid()) {
        _start = vec3FromVariant(localStart);
    }
    properties.remove("localStart"); // so that Base3DOverlay doesn't respond to it

    auto end = properties["end"];
    // if "end" property was not there, check to see if they included aliases: endPoint
    if (!end.isValid()) {
        end = properties["endPoint"];
    }
    if (end.isValid()) {
        setEnd(vec3FromVariant(end));
    }

    auto localEnd = properties["localEnd"];
    if (localEnd.isValid()) {
        _end = vec3FromVariant(localEnd);
    }
    properties.remove("localEnd"); // so that Base3DOverlay doesn't respond to it

    auto glow = properties["glow"];
    if (glow.isValid()) {
        setGlow(glow.toFloat());
        if (_glow > 0.0f) {
            _alpha = 0.5f;
        }
    }

    auto glowWidth = properties["glow"];
    if (glowWidth.isValid()) {
        setGlow(glowWidth.toFloat());
    }

    Base3DOverlay::setProperties(properties);
}

QVariant Line3DOverlay::getProperty(const QString& property) {
    if (property == "start" || property == "startPoint" || property == "p1") {
        return vec3toVariant(getStart());
    }
    if (property == "end" || property == "endPoint" || property == "p2") {
        return vec3toVariant(getEnd());
    }

    return Base3DOverlay::getProperty(property);
}

Line3DOverlay* Line3DOverlay::createClone() const {
    return new Line3DOverlay(this);
}


void Line3DOverlay::locationChanged(bool tellPhysics) {
    // do nothing
}
