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
    SpatiallyNestable(NestableType::Overlay, QUuid::createUuid()),
    _lineWidth(DEFAULT_LINE_WIDTH),
    _isSolid(DEFAULT_IS_SOLID),
    _isDashedLine(DEFAULT_IS_DASHED_LINE),
    _ignoreRayIntersection(false),
    _drawInFront(false)
{
}

Base3DOverlay::Base3DOverlay(const Base3DOverlay* base3DOverlay) :
    Overlay(base3DOverlay),
    SpatiallyNestable(NestableType::Overlay, QUuid::createUuid()),
    _lineWidth(base3DOverlay->_lineWidth),
    _isSolid(base3DOverlay->_isSolid),
    _isDashedLine(base3DOverlay->_isDashedLine),
    _ignoreRayIntersection(base3DOverlay->_ignoreRayIntersection),
    _drawInFront(base3DOverlay->_drawInFront),
    _isGrabbable(base3DOverlay->_isGrabbable)
{
    setTransform(base3DOverlay->getTransform());
}

QVariantMap convertOverlayLocationFromScriptSemantics(const QVariantMap& properties) {
    // the position and rotation in _transform are relative to the parent (aka local).  The versions coming from
    // scripts are in world-frame, unless localPosition or localRotation are used.  Patch up the properties
    // so that "position" and "rotation" are relative-to-parent values.
    QVariantMap result = properties;
    QUuid parentID = result["parentID"].isValid() ? QUuid(result["parentID"].toString()) : QUuid();
    int parentJointIndex = result["parentJointIndex"].isValid() ? result["parentJointIndex"].toInt() : -1;
    bool success;

    // make "position" and "orientation" be relative-to-parent
    if (result["localPosition"].isValid()) {
        result["position"] = result["localPosition"];
    } else if (result["position"].isValid()) {
        glm::vec3 localPosition = SpatiallyNestable::worldToLocal(vec3FromVariant(result["position"]),
                                                                  parentID, parentJointIndex, success);
        if (success) {
            result["position"] = vec3toVariant(localPosition);
        }
    }

    if (result["localOrientation"].isValid()) {
        result["orientation"] = result["localOrientation"];
    } else if (result["orientation"].isValid()) {
        glm::quat localOrientation = SpatiallyNestable::worldToLocal(quatFromVariant(result["orientation"]),
                                                                     parentID, parentJointIndex, success);
        if (success) {
            result["orientation"] = quatToVariant(localOrientation);
        }
    }

    return result;
}

void Base3DOverlay::setProperties(const QVariantMap& originalProperties) {
    QVariantMap properties = originalProperties;

    if (properties["name"].isValid()) {
        setName(properties["name"].toString());
    }

    // carry over some legacy keys
    if (!properties["position"].isValid() && !properties["localPosition"].isValid()) {
        if (properties["p1"].isValid()) {
            properties["position"] = properties["p1"];
        } else if (properties["point"].isValid()) {
            properties["position"] = properties["point"];
        } else if (properties["start"].isValid()) {
            properties["position"] = properties["start"];
        }
    }
    if (!properties["orientation"].isValid() && properties["rotation"].isValid()) {
        properties["orientation"] = properties["rotation"];
    }
    if (!properties["localOrientation"].isValid() && properties["localRotation"].isValid()) {
        properties["localOrientation"] = properties["localRotation"];
    }

    // All of parentID, parentJointIndex, position, orientation are needed to make sense of any of them.
    // If any of these changed, pull any missing properties from the overlay.
    if (properties["parentID"].isValid() || properties["parentJointIndex"].isValid() ||
        properties["position"].isValid() || properties["localPosition"].isValid() ||
        properties["orientation"].isValid() || properties["localOrientation"].isValid()) {
        if (!properties["parentID"].isValid()) {
            properties["parentID"] = getParentID();
        }
        if (!properties["parentJointIndex"].isValid()) {
            properties["parentJointIndex"] = getParentJointIndex();
        }
        if (!properties["position"].isValid() && !properties["localPosition"].isValid()) {
            properties["position"] = vec3toVariant(getPosition());
        }
        if (!properties["orientation"].isValid() && !properties["localOrientation"].isValid()) {
            properties["orientation"] = quatToVariant(getOrientation());
        }
    }

    properties = convertOverlayLocationFromScriptSemantics(properties);
    Overlay::setProperties(properties);

    bool needRenderItemUpdate = false;

    auto drawInFront = properties["drawInFront"];

    if (drawInFront.isValid()) {
        bool value = drawInFront.toBool();
        setDrawInFront(value);
        needRenderItemUpdate = true;
    }

    auto isGrabbable = properties["grabbable"];
    if (isGrabbable.isValid()) {
        setIsGrabbable(isGrabbable.toBool());
    }

    if (properties["position"].isValid()) {
        setLocalPosition(vec3FromVariant(properties["position"]));
        needRenderItemUpdate = true;
    }
    if (properties["orientation"].isValid()) {
        setLocalOrientation(quatFromVariant(properties["orientation"]));
        needRenderItemUpdate = true;
    }

    if (properties["lineWidth"].isValid()) {
        setLineWidth(properties["lineWidth"].toFloat());
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

    if (properties["parentID"].isValid()) {
        setParentID(QUuid(properties["parentID"].toString()));
        needRenderItemUpdate = true;
    }
    if (properties["parentJointIndex"].isValid()) {
        setParentJointIndex(properties["parentJointIndex"].toInt());
        needRenderItemUpdate = true;
    }

    // Communicate changes to the renderItem if needed
    if (needRenderItemUpdate) {
        auto itemID = getRenderItemID();
        if (render::Item::isValidID(itemID)) {
            render::ScenePointer scene = qApp->getMain3DScene();
            render::Transaction transaction;
            transaction.updateItem(itemID);
            scene->enqueueTransaction(transaction);
        }
    }
}

QVariant Base3DOverlay::getProperty(const QString& property) {
    if (property == "name") {
        return _name;
    }
    if (property == "position" || property == "start" || property == "p1" || property == "point") {
        return vec3toVariant(getPosition());
    }
    if (property == "localPosition") {
        return vec3toVariant(getLocalPosition());
    }
    if (property == "rotation" || property == "orientation") {
        return quatToVariant(getOrientation());
    }
    if (property == "localRotation" || property == "localOrientation") {
        return quatToVariant(getLocalOrientation());
    }
    if (property == "lineWidth") {
        return _lineWidth;
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
    if (property == "grabbable") {
        return _isGrabbable;
    }
    if (property == "parentID") {
        return getParentID();
    }
    if (property == "parentJointIndex") {
        return getParentJointIndex();
    }

    return Overlay::getProperty(property);
}

bool Base3DOverlay::findRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                                                        float& distance, BoxFace& face, glm::vec3& surfaceNormal) {
    return false;
}

void Base3DOverlay::locationChanged(bool tellPhysics) {
    SpatiallyNestable::locationChanged(tellPhysics);

    // Force the actual update of the render transform through the notify call
    notifyRenderTransformChange();
}

void Base3DOverlay::parentDeleted() {
    qApp->getOverlays().deleteOverlay(getOverlayID());
}

void Base3DOverlay::update(float duration) {
    
    // In Base3DOverlay, if its location or bound changed, the renderTrasnformDirty flag is true.
    // then the correct transform used for rendering is computed in the update transaction and assigned.
    // TODO: Fix the value to be computed in main thread now and passed by value to the render item.
    //       This is the simplest fix for the web overlay of the tablet for now
    if (_renderTransformDirty) {
        _renderTransformDirty = false;
        auto itemID = getRenderItemID();
        if (render::Item::isValidID(itemID)) {
            render::ScenePointer scene = qApp->getMain3DScene();
            render::Transaction transaction;
            transaction.updateItem<Overlay>(itemID, [](Overlay& data) {
                auto overlay3D = dynamic_cast<Base3DOverlay*>(&data);
                if (overlay3D) {
                    auto latestTransform = overlay3D->evalRenderTransform();
                    overlay3D->setRenderTransform(latestTransform);
                }
            });
            scene->enqueueTransaction(transaction);       
        }
    }
}

void Base3DOverlay::notifyRenderTransformChange() const {
    _renderTransformDirty = true;
}

Transform Base3DOverlay::evalRenderTransform() const {
    return getTransform();
}

void Base3DOverlay::setRenderTransform(const Transform& transform) {
    _renderTransform = transform;
}
