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

#include "AbstractViewStateInterface.h"

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
    _endParentID = line3DOverlay->getEndParentID();
    _endParentJointIndex = line3DOverlay->getEndJointIndex();
    _glow = line3DOverlay->getGlow();
    _glowWidth = line3DOverlay->getGlowWidth();
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
    glm::vec3 localEnd;
    glm::vec3 worldEnd;

    if (_endParentID != QUuid()) {
        glm::vec3 localOffset = _direction * _length;
        bool success;
        worldEnd = localToWorld(localOffset, _endParentID, _endParentJointIndex, success);
        return worldEnd;
    }

    localEnd = getLocalEnd();
    worldEnd = localToWorld(localEnd, getParentID(), getParentJointIndex(), success);
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
    glm::vec3 localStart;
    glm::vec3 localEnd;
    glm::vec3 offset;

    if (_endParentID != QUuid()) {
        offset = worldToLocal(end, _endParentID, _endParentJointIndex, success);
    } else {
        localStart = getLocalStart();
        localEnd = worldToLocal(end, getParentID(), getParentJointIndex(), success);
        offset = localEnd - localStart;
    }
    if (!success) {
        qDebug() << "Line3DOverlay::setEnd failed";
        return;
    }

    _length = glm::length(offset);
    if (_length > 0.0f) {
        _direction = glm::normalize(offset);
    } else {
        _direction = glm::vec3(0.0f);
    }
}

void Line3DOverlay::setLocalEnd(const glm::vec3& localEnd) {
    glm::vec3 offset;
    if (_endParentID != QUuid()) {
        offset = localEnd;
    } else {
        glm::vec3 localStart = getLocalStart();
        offset = localEnd - localStart;
    }
    _length = glm::length(offset);
    if (_length > 0.0f) {
        _direction = glm::normalize(offset);
    } else {
        _direction = glm::vec3(0.0f);
    }
}

AABox Line3DOverlay::getBounds() const {
    auto extents = Extents{};
    extents.addPoint(getStart());
    extents.addPoint(getEnd());
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
        // FIXME Start using the _renderTransform instead of calling for Transform and start and end from here, do the custom things needed in evalRenderTransform()
        batch->setModelTransform(Transform());
        glm::vec3 start = getStart();
        glm::vec3 end = getEnd();

        auto geometryCache = DependencyManager::get<GeometryCache>();
        if (getIsDashedLine()) {
            // TODO: add support for color to renderDashedLine()
            geometryCache->bindSimpleProgram(*batch, false, false, false, true, true);
            geometryCache->renderDashedLine(*batch, start, end, colorv4, _geometryCacheID);
        } else {
            // renderGlowLine handles both glow = 0 and glow > 0 cases
            geometryCache->renderGlowLine(*batch, start, end, colorv4, _glow, _glowWidth, _geometryCacheID);
        }
    }
}

const render::ShapeKey Line3DOverlay::getShapeKey() {
    auto builder = render::ShapeKey::Builder().withOwnPipeline();
    if (isTransparent()) {
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

    auto length = properties["length"];
    if (length.isValid()) {
        _length = length.toFloat();
    }

    Base3DOverlay::setProperties(properties);

    auto endParentIDProp = properties["endParentID"];
    if (endParentIDProp.isValid()) {
        _endParentID = QUuid(endParentIDProp.toString());
    }
    auto endParentJointIndexProp = properties["endParentJointIndex"];
    if (endParentJointIndexProp.isValid()) {
        _endParentJointIndex = endParentJointIndexProp.toInt();
    }

    auto localStart = properties["localStart"];
    if (localStart.isValid()) {
        glm::vec3 tmpLocalEnd = getLocalEnd();
        setLocalStart(vec3FromVariant(localStart));
        setLocalEnd(tmpLocalEnd);
    }

    auto localEnd = properties["localEnd"];
    if (localEnd.isValid()) {
        setLocalEnd(vec3FromVariant(localEnd));
    }

    // these are saved until after Base3DOverlay::setProperties so parenting infomation can be set, first
    if (newStartSet) {
        setStart(newStart);
    }
    if (newEndSet) {
        setEnd(newEnd);
    }

    auto glow = properties["glow"];
    if (glow.isValid()) {
        float prevGlow = _glow;
        setGlow(glow.toFloat());
        // Update our payload key if necessary to handle transparency
        if ((prevGlow <= 0.0f && _glow > 0.0f) || (prevGlow > 0.0f && _glow <= 0.0f)) {
            auto itemID = getRenderItemID();
            if (render::Item::isValidID(itemID)) {
                render::ScenePointer scene = AbstractViewStateInterface::instance()->getMain3DScene();
                render::Transaction transaction;
                transaction.updateItem(itemID);
                scene->enqueueTransaction(transaction);
            }
        }
    }

    auto glowWidth = properties["glowWidth"];
    if (glowWidth.isValid()) {
        setGlowWidth(glowWidth.toFloat());
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
