//
//  Base3DOverlay.cpp
//  interface/src/ui/overlays
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#include "Base3DOverlay.h"

#include <RegisteredMetaTypes.h>
#include <SharedUtil.h>
#include "Application.h"


const float DEFAULT_LINE_WIDTH = 1.0f;
const bool DEFAULT_IS_SOLID = false;
const bool DEFAULT_IS_DASHED_LINE = false;

Base3DOverlay::Base3DOverlay() :
    _lineWidth(DEFAULT_LINE_WIDTH),
    _isSolid(DEFAULT_IS_SOLID),
    _isDashedLine(DEFAULT_IS_DASHED_LINE),
    _ignoreRayIntersection(false),
    _drawInFront(false)
{
}

Base3DOverlay::Base3DOverlay(const Base3DOverlay* base3DOverlay) :
    Overlay(base3DOverlay),
    _transform(base3DOverlay->_transform),
    _lineWidth(base3DOverlay->_lineWidth),
    _isSolid(base3DOverlay->_isSolid),
    _isDashedLine(base3DOverlay->_isDashedLine),
    _ignoreRayIntersection(base3DOverlay->_ignoreRayIntersection),
    _drawInFront(base3DOverlay->_drawInFront)
{
}
void Base3DOverlay::setProperties(const QVariantMap& properties) {
    Overlay::setProperties(properties);

    bool needRenderItemUpdate = false;

    auto drawInFront = properties["drawInFront"];

    if (drawInFront.isValid()) {
        bool value = drawInFront.toBool();
        setDrawInFront(value);
        needRenderItemUpdate = true;
    }

    auto position = properties["position"];

    // if "position" property was not there, check to see if they included aliases: point, p1
    if (!position.isValid()) {
        position = properties["p1"];
        if (!position.isValid()) {
            position = properties["point"];
        }
    }
    if (position.isValid()) {
        setPosition(vec3FromVariant(position));
        needRenderItemUpdate = true;
    }

    if (properties["lineWidth"].isValid()) {
        setLineWidth(properties["lineWidth"].toFloat());
        needRenderItemUpdate = true;
    }

    auto rotation = properties["rotation"];

    if (rotation.isValid()) {
        setRotation(quatFromVariant(rotation));
        needRenderItemUpdate = true;
    }

    if (properties["isSolid"].isValid()) {
        setIsSolid(properties["isSolid"].toBool());
    }
    if (properties["isFilled"].isValid()) {
        setIsSolid(properties["isSolid"].toBool());
    }
    if (properties["isWire"].isValid()) {
        setIsSolid(!properties["isWire"].toBool());
    }
    if (properties["solid"].isValid()) {
        setIsSolid(properties["solid"].toBool());
    }
    if (properties["filled"].isValid()) {
        setIsSolid(properties["filled"].toBool());
    }
    if (properties["wire"].isValid()) {
        setIsSolid(!properties["wire"].toBool());
    }

    if (properties["isDashedLine"].isValid()) {
        setIsDashedLine(properties["isDashedLine"].toBool());
    }
    if (properties["dashed"].isValid()) {
        setIsDashedLine(properties["dashed"].toBool());
    }
    if (properties["ignoreRayIntersection"].isValid()) {
        setIgnoreRayIntersection(properties["ignoreRayIntersection"].toBool());
    }

    // Communicate changes to the renderItem if needed
    if (needRenderItemUpdate) {
        auto itemID = getRenderItemID();
        if (render::Item::isValidID(itemID)) {
            render::ScenePointer scene = qApp->getMain3DScene();
            render::PendingChanges pendingChanges;
            pendingChanges.updateItem(itemID);
            scene->enqueuePendingChanges(pendingChanges);
        }
    }
}

QVariant Base3DOverlay::getProperty(const QString& property) {
    if (property == "position" || property == "start" || property == "p1" || property == "point") {
        return vec3toVariant(getPosition());
    }
    if (property == "lineWidth") {
        return _lineWidth;
    }
    if (property == "rotation") {
        return quatToVariant(getRotation());
    }
    if (property == "isSolid" || property == "isFilled" || property == "solid" || property == "filed") {
        return _isSolid;
    }
    if (property == "isWire" || property == "wire") {
        return !_isSolid;
    }
    if (property == "isDashedLine" || property == "dashed") {
        return _isDashedLine;
    }
    if (property == "ignoreRayIntersection") {
        return _ignoreRayIntersection;
    }
    if (property == "drawInFront") {
        return _drawInFront;
    }

    return Overlay::getProperty(property);
}

bool Base3DOverlay::findRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                                                        float& distance, BoxFace& face, glm::vec3& surfaceNormal) {
    return false;
}
