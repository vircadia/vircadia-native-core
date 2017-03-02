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
}

Line3DOverlay::Line3DOverlay(const Line3DOverlay* line3DOverlay) :
    Base3DOverlay(line3DOverlay),
    _geometryCacheID(DependencyManager::get<GeometryCache>()->allocateID())
{
    setParentID(line3DOverlay->getParentID());
    setParentJointIndex(line3DOverlay->getParentJointIndex());
    setLocalTransform(line3DOverlay->getLocalTransform());
    _direction = line3DOverlay->getDirection();
    _length = line3DOverlay->getLength();
}

Line3DOverlay::~Line3DOverlay() {
    auto geometryCache = DependencyManager::get<GeometryCache>();
    if (_geometryCacheID && geometryCache) {
        geometryCache->releaseID(_geometryCacheID);
    }
}

glm::vec3 Line3DOverlay::getStart() const {
    return getPosition();
}

glm::vec3 Line3DOverlay::getEnd() const {
    bool success;

    glm::vec3 localEnd = getLocalEnd();
    glm::vec3 worldEnd = localToWorld(localEnd, getParentID(), getParentJointIndex(), success);
    if (!success) {
        qDebug() << "Line3DOverlay::getEnd failed";
    }
    return worldEnd;
}

void Line3DOverlay::setStart(const glm::vec3& start) {
    setPosition(start);
}

void Line3DOverlay::setEnd(const glm::vec3& end) {
    bool success;

    glm::vec3 localStart = getLocalStart();
    glm::vec3 localEnd = worldToLocal(end, getParentID(), getParentJointIndex(), success);
    if (!success) {
        qDebug() << "Line3DOverlay::setEnd failed";
        return;
    }

    glm::vec3 offset = localEnd - localStart;
    _direction = glm::normalize(offset);
    _length = glm::length(offset);
}

AABox Line3DOverlay::getBounds() const {
    auto extents = Extents{};
    extents.addPoint(getStart());
    extents.addPoint(getEnd());
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
        // batch->setModelTransform(getTransform());

        auto geometryCache = DependencyManager::get<GeometryCache>();
        if (getIsDashedLine()) {
            // TODO: add support for color to renderDashedLine()
            geometryCache->bindSimpleProgram(*batch, false, false, false, true, true);
            geometryCache->renderDashedLine(*batch, getStart(), getEnd(), colorv4, _geometryCacheID);
        } else if (_glow > 0.0f) {
            geometryCache->renderGlowLine(*batch, getStart(), getEnd(),
                                          colorv4, _glow, _glowWidth, _geometryCacheID);
        } else {
            geometryCache->bindSimpleProgram(*batch, false, false, false, true, true);
            geometryCache->renderLine(*batch, getStart(), getEnd(), colorv4, _geometryCacheID);
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
    glm::vec3 newStart(0.0f);
    bool newStartSet { false };
    glm::vec3 newEnd(0.0f);
    bool newEndSet { false };

    auto start = properties["start"];
    // if "start" property was not there, check to see if they included aliases: startPoint
    if (!start.isValid()) {
        start = properties["startPoint"];
    }
    if (start.isValid()) {
        newStart = vec3FromVariant(start);
        newStartSet = true;
    }
    properties.remove("start"); // so that Base3DOverlay doesn't respond to it

    auto localStart = properties["localStart"];
    if (localStart.isValid()) {
        setLocalPosition(vec3FromVariant(localStart));
        // in case "end" isn't updated...
        glm::vec3 offset = getLocalEnd() - getLocalStart();
        _direction = glm::normalize(offset);
        _length = glm::length(offset);
    }
    properties.remove("localStart"); // so that Base3DOverlay doesn't respond to it

    auto end = properties["end"];
    // if "end" property was not there, check to see if they included aliases: endPoint
    if (!end.isValid()) {
        end = properties["endPoint"];
    }
    if (end.isValid()) {
        newEnd = vec3FromVariant(end);
        newEndSet = true;
    }
    properties.remove("end"); // so that Base3DOverlay doesn't respond to it

    auto localEnd = properties["localEnd"];
    if (localEnd.isValid()) {
        glm::vec3 offset = vec3FromVariant(localEnd) - getLocalStart();
        _direction = glm::normalize(offset);
        _length = glm::length(offset);
    }
    properties.remove("localEnd"); // so that Base3DOverlay doesn't respond to it

    auto length = properties["length"];
    if (length.isValid()) {
        _length = length.toFloat();
    }

    Base3DOverlay::setProperties(properties);

    // these are saved until after Base3DOverlay::setProperties so parenting infomation can be set, first
    if (newStartSet) {
        setStart(newStart);
    }
    if (newEndSet) {
        setEnd(newEnd);
    }

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

}

QVariant Line3DOverlay::getProperty(const QString& property) {
    if (property == "start" || property == "startPoint" || property == "p1") {
        return vec3toVariant(getStart());
    }
    if (property == "end" || property == "endPoint" || property == "p2") {
        return vec3toVariant(getEnd());
    }
    if (property == "localStart") {
        return vec3toVariant(getLocalStart());
    }
    if (property == "localEnd") {
        return vec3toVariant(getLocalEnd());
    }
    if (property == "length") {
        return QVariant(getLength());
    }

    return Base3DOverlay::getProperty(property);
}

Line3DOverlay* Line3DOverlay::createClone() const {
    return new Line3DOverlay(this);
}


void Line3DOverlay::locationChanged(bool tellPhysics) {
    // do nothing
}
