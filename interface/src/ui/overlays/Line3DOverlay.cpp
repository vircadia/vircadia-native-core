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
    _lineWidth = line3DOverlay->getLineWidth();
    _glow = line3DOverlay->getGlow();
}

Line3DOverlay::~Line3DOverlay() {
    auto geometryCache = DependencyManager::get<GeometryCache>();
    if (_geometryCacheID && geometryCache) {
        geometryCache->releaseID(_geometryCacheID);
    }
}

glm::vec3 Line3DOverlay::getStart() const {
    return getWorldPosition();
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
    setWorldPosition(start);
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
    notifyRenderVariableChange();
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
    if (!_renderVisible) {
        return; // do nothing if we're not visible
    }

    float alpha = getAlpha();
    xColor color = getColor();
    const float MAX_COLOR = 255.0f;
    glm::vec4 colorv4(color.red / MAX_COLOR, color.green / MAX_COLOR, color.blue / MAX_COLOR, alpha);
    auto batch = args->_batch;
    if (batch) {
        batch->setModelTransform(Transform());
        auto& renderTransform = getRenderTransform();
        glm::vec3 start = renderTransform.getTranslation();
        glm::vec3 end = renderTransform.transform(vec3(0.0, 0.0, -1.0));

        auto geometryCache = DependencyManager::get<GeometryCache>();
        if (getIsDashedLine()) {
            // TODO: add support for color to renderDashedLine()
            geometryCache->bindSimpleProgram(*batch, false, false, false, true, true);
            geometryCache->renderDashedLine(*batch, start, end, colorv4, _geometryCacheID);
        } else {
            // renderGlowLine handles both glow = 0 and glow > 0 cases
            geometryCache->renderGlowLine(*batch, start, end, colorv4, _glow, _lineWidth, _geometryCacheID);
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
    // If "start" property was not there, check to see if they included aliases: startPoint, p1
    if (!start.isValid()) {
        start = properties["startPoint"];
    }
    if (!start.isValid()) {
        start = properties["p1"];
    }
    if (start.isValid()) {
        newStart = vec3FromVariant(start);
        newStartSet = true;
    }
    properties.remove("start"); // so that Base3DOverlay doesn't respond to it
    properties.remove("startPoint");
    properties.remove("p1");

    auto end = properties["end"];
    // If "end" property was not there, check to see if they included aliases: endPoint, p2
    if (!end.isValid()) {
        end = properties["endPoint"];
    }
    if (!end.isValid()) {
        end = properties["p2"];
    }
    if (end.isValid()) {
        newEnd = vec3FromVariant(end);
        newEndSet = true;
    }
    properties.remove("end"); // so that Base3DOverlay doesn't respond to it
    properties.remove("endPoint");
    properties.remove("p2");

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

    auto lineWidth = properties["lineWidth"];
    if (lineWidth.isValid()) {
        setLineWidth(lineWidth.toFloat());
    }
}

/**jsdoc
 * These are the properties of a <code>line3d</code> {@link Overlays.OverlayType|OverlayType}.
 * @typedef {object} Overlays.Line3DProperties
 * 
 * @property {string} type=line3d - Has the value <code>"line3d"</code>. <em>Read-only.</em>
 * @property {Color} color=255,255,255 - The color of the overlay.
 * @property {number} alpha=0.7 - The opacity of the overlay, <code>0.0</code> - <code>1.0</code>.
 * @property {number} pulseMax=0 - The maximum value of the pulse multiplier.
 * @property {number} pulseMin=0 - The minimum value of the pulse multiplier.
 * @property {number} pulsePeriod=1 - The duration of the color and alpha pulse, in seconds. A pulse multiplier value goes from
 *     <code>pulseMin</code> to <code>pulseMax</code>, then <code>pulseMax</code> to <code>pulseMin</code> in one period.
 * @property {number} alphaPulse=0 - If non-zero, the alpha of the overlay is pulsed: the alpha value is multiplied by the
 *     current pulse multiplier value each frame. If > 0 the pulse multiplier is applied in phase with the pulse period; if < 0
 *     the pulse multiplier is applied out of phase with the pulse period. (The magnitude of the property isn't otherwise
 *     used.)
 * @property {number} colorPulse=0 - If non-zero, the color of the overlay is pulsed: the color value is multiplied by the
 *     current pulse multiplier value each frame. If > 0 the pulse multiplier is applied in phase with the pulse period; if < 0
 *     the pulse multiplier is applied out of phase with the pulse period. (The magnitude of the property isn't otherwise
 *     used.)
 * @property {boolean} visible=true - If <code>true</code>, the overlay is rendered, otherwise it is not rendered.
 * @property {string} anchor="" - If set to <code>"MyAvatar"</code> then the overlay is attached to your avatar, moving and
 *     rotating as you move your avatar.
 *
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
 *
 * @property {Uuid} endParentID=null - The avatar, entity, or overlay that the end point of the line is parented to.
 * @property {number} endParentJointIndex=65535 - Integer value specifying the skeleton joint that the end point of the line is
 *     attached to if <code>parentID</code> is an avatar skeleton. A value of <code>65535</code> means "no joint".
 * @property {Vec3} start - The start point of the line. Synonyms: <code>startPoint</code>, <code>p1</code>, and
 *     <code>position</code>.
 * @property {Vec3} end - The end point of the line. Synonyms: <code>endPoint</code> and <code>p2</code>.
 * @property {Vec3} localStart - The local position of the overlay relative to its parent if the overlay has a
 *     <code>parentID</code> set, otherwise the same value as <code>start</code>. Synonym: <code>localPosition</code>.
 * @property {Vec3} localEnd - The local position of the overlay relative to its parent if the overlay has a
 *     <code>endParentID</code> set, otherwise the same value as <code>end</code>.
 * @property {number} length - The length of the line, in meters. This can be set after creating a line with start and end
 *     points.
 * @property {number} glow=0 - If <code>glow > 0</code>, the line is rendered with a glow.
 * @property {number} lineWidth=0.02 - If <code>glow > 0</code>, this is the width of the glow, in meters.
 */
QVariant Line3DOverlay::getProperty(const QString& property) {
    if (property == "start" || property == "startPoint" || property == "p1") {
        return vec3toVariant(getStart());
    }
    if (property == "end" || property == "endPoint" || property == "p2") {
        return vec3toVariant(getEnd());
    }
    if (property == "length") {
        return QVariant(getLength());
    }
    if (property == "endParentID") {
        return _endParentID;
    }
    if (property == "endParentJointIndex") {
        return _endParentJointIndex;
    }
    if (property == "localStart") {
        return vec3toVariant(getLocalStart());
    }
    if (property == "localEnd") {
        return vec3toVariant(getLocalEnd());
    }
    if (property == "glow") {
        return getGlow();
    }
    if (property == "lineWidth") {
        return _lineWidth;
    }

    return Base3DOverlay::getProperty(property);
}

Line3DOverlay* Line3DOverlay::createClone() const {
    return new Line3DOverlay(this);
}

Transform Line3DOverlay::evalRenderTransform() {
    // Capture start and endin the renderTransform:
    // start is the origin
    // end is at the tip of the front axis aka -Z
    Transform transform;
    transform.setTranslation( getStart());
    auto endPos = getEnd();

    auto vec = endPos - transform.getTranslation();
    const float MIN_LINE_LENGTH = 0.0001f;
    auto scale = glm::max(glm::length(vec), MIN_LINE_LENGTH);
    auto dir = vec / scale;
    auto orientation = glm::rotation(glm::vec3(0.0f, 0.0f, -1.0f), dir);
    transform.setRotation(orientation);
    transform.setScale(scale);

    return transform;
}
