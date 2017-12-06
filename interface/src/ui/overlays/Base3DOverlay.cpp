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


const bool DEFAULT_IS_SOLID = false;
const bool DEFAULT_IS_DASHED_LINE = false;

Base3DOverlay::Base3DOverlay() :
    SpatiallyNestable(NestableType::Overlay, QUuid::createUuid()),
    _isSolid(DEFAULT_IS_SOLID),
    _isDashedLine(DEFAULT_IS_DASHED_LINE),
    _ignoreRayIntersection(false),
    _drawInFront(false),
    _drawHUDLayer(false)
{
}

Base3DOverlay::Base3DOverlay(const Base3DOverlay* base3DOverlay) :
    Overlay(base3DOverlay),
    SpatiallyNestable(NestableType::Overlay, QUuid::createUuid()),
    _isSolid(base3DOverlay->_isSolid),
    _isDashedLine(base3DOverlay->_isDashedLine),
    _ignoreRayIntersection(base3DOverlay->_ignoreRayIntersection),
    _drawInFront(base3DOverlay->_drawInFront),
    _drawHUDLayer(base3DOverlay->_drawHUDLayer),
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
            properties["position"] = vec3toVariant(getWorldPosition());
        }
        if (!properties["orientation"].isValid() && !properties["localOrientation"].isValid()) {
            properties["orientation"] = quatToVariant(getWorldOrientation());
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

    auto drawHUDLayer = properties["drawHUDLayer"];
    if (drawHUDLayer.isValid()) {
        bool value = drawHUDLayer.toBool();
        setDrawHUDLayer(value);
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

// JSDoc for copying to @typedefs of overlay types that inherit Base3DOverlay.
/**jsdoc
 * @property {string} name="" - A friendly name for the overlay.
 * @property {Vec3} position - The position of the overlay center. Synonyms: <code>p1</code>, <code>point</code>, and 
 *     <code>start</code>.
 * @property {Vec3} localPosition - The local position of the overlay relative to its parent if the overlay has a
 *     <code>parentID</code> set, otherwise the same value as <code>position</code>.
 * @property {Quat} rotation - The orientation of the overlay. Synonym: <code>orientation</code>.
 * @property {Quat} localRotation - The orientation of the overlay relative to its parent if the overlay has a
 *     <code>parentID</code> set, otherwise the same value as <code>rotation</code>.
 * @property {boolean} isSolid=false - Synonyms: <ode>solid</code>, <code>isFilled</code>,
 *     <code>filled</code>, and <code>filed</code>. Antonyms: <code>isWire</code> and <code>wire</code>.
 *     <strong>Deprecated:</strong> The erroneous property spelling "<code>filed</code>" is deprecated and support for it will
 *     be removed.
 * @property {boolean} isDashedLine=false - If <code>true</code>, a dashed line is drawn on the overlay's edges. Synonym:
 *     <code>dashed</code>.
 * @property {boolean} ignoreRayIntersection=false - If <code>true</code>, 
 *     {@link Overlays.findRayIntersection|findRayIntersection} ignores the overlay.
 * @property {boolean} drawInFront=false - If <code>true</code>, the overlay is rendered in front of other overlays that don't
 *     have <code>drawInFront</code> set to <code>true</code>, and in front of entities.
 * @property {boolean} grabbable=false - Signal to grabbing scripts whether or not this overlay can be grabbed.
 * @property {Uuid} parentID=null - The avatar, entity, or overlay that the overlay is parented to.
 * @property {number} parentJointIndex=65535 - Integer value specifying the skeleton joint that the overlay is attached to if
 *     <code>parentID</code> is an avatar skeleton. A value of <code>65535</code> means "no joint".
 */
QVariant Base3DOverlay::getProperty(const QString& property) {
    if (property == "name") {
        return _name;
    }
    if (property == "position" || property == "start" || property == "p1" || property == "point") {
        return vec3toVariant(getWorldPosition());
    }
    if (property == "localPosition") {
        return vec3toVariant(getLocalPosition());
    }
    if (property == "rotation" || property == "orientation") {
        return quatToVariant(getWorldOrientation());
    }
    if (property == "localRotation" || property == "localOrientation") {
        return quatToVariant(getLocalOrientation());
    }
    if (property == "isSolid" || property == "isFilled" || property == "solid" || property == "filled" || property == "filed") {
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
    notifyRenderVariableChange();
}

void Base3DOverlay::parentDeleted() {
    qApp->getOverlays().deleteOverlay(getOverlayID());
}

void Base3DOverlay::update(float duration) {
    // In Base3DOverlay, if its location or bound changed, the renderTrasnformDirty flag is true.
    // then the correct transform used for rendering is computed in the update transaction and assigned.
    if (_renderVariableDirty) {
        auto itemID = getRenderItemID();
        if (render::Item::isValidID(itemID)) {
            // Capture the render transform value in game loop before
            auto latestTransform = evalRenderTransform();
            bool latestVisible = getVisible();
            _renderVariableDirty = false;
            render::ScenePointer scene = qApp->getMain3DScene();
            render::Transaction transaction;
            transaction.updateItem<Overlay>(itemID, [latestTransform, latestVisible](Overlay& data) {
                auto overlay3D = dynamic_cast<Base3DOverlay*>(&data);
                if (overlay3D) {
                    // TODO: overlays need to communicate all relavent render properties through transactions
                    overlay3D->setRenderTransform(latestTransform);
                    overlay3D->setRenderVisible(latestVisible);
                }
            });
            scene->enqueueTransaction(transaction);
        }
    }
}

void Base3DOverlay::notifyRenderVariableChange() const {
    _renderVariableDirty = true;
}

Transform Base3DOverlay::evalRenderTransform() {
    return getTransform();
}

void Base3DOverlay::setRenderTransform(const Transform& transform) {
    _renderTransform = transform;
}

void Base3DOverlay::setRenderVisible(bool visible) {
    _renderVisible = visible;
}

SpatialParentTree* Base3DOverlay::getParentTree() const {
    auto entityTreeRenderer = qApp->getEntities();
    EntityTreePointer entityTree = entityTreeRenderer ? entityTreeRenderer->getTree() : nullptr;
    return entityTree.get();
}

void Base3DOverlay::setVisible(bool visible) {
    Parent::setVisible(visible);
    notifyRenderVariableChange();
}