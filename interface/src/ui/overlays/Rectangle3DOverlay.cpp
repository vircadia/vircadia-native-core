//
//  Rectangle3DOverlay.cpp
//  interface/src/ui/overlays
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Rectangle3DOverlay.h"

#include <GeometryCache.h>
#include <SharedUtil.h>


QString const Rectangle3DOverlay::TYPE = "rectangle3d";

Rectangle3DOverlay::Rectangle3DOverlay() :
    _geometryCacheID(DependencyManager::get<GeometryCache>()->allocateID())
{
    auto geometryCache = DependencyManager::get<GeometryCache>();
    for (size_t i = 0; i < _rectGeometryIds.size(); ++i) {
        _rectGeometryIds[i] = geometryCache->allocateID();
    }
}

Rectangle3DOverlay::Rectangle3DOverlay(const Rectangle3DOverlay* rectangle3DOverlay) :
    Planar3DOverlay(rectangle3DOverlay),
    _geometryCacheID(DependencyManager::get<GeometryCache>()->allocateID())
{
    auto geometryCache = DependencyManager::get<GeometryCache>();
    for (size_t i = 0; i < _rectGeometryIds.size(); ++i) {
        _rectGeometryIds[i] = geometryCache->allocateID();
    }
}

Rectangle3DOverlay::~Rectangle3DOverlay() {
    auto geometryCache = DependencyManager::get<GeometryCache>();
    if (geometryCache) {
        geometryCache->releaseID(_geometryCacheID);
        for (size_t i = 0; i < _rectGeometryIds.size(); ++i) {
            geometryCache->releaseID(_rectGeometryIds[i]);
        }
    }
}

void Rectangle3DOverlay::render(RenderArgs* args) {
    if (!_renderVisible) {
        return; // do nothing if we're not visible
    }

    float alpha = getAlpha();
    xColor color = getColor();
    const float MAX_COLOR = 255.0f;
    glm::vec4 rectangleColor(color.red / MAX_COLOR, color.green / MAX_COLOR, color.blue / MAX_COLOR, alpha);

    auto batch = args->_batch;
    if (batch) {
        Transform transform = getRenderTransform();
        glm::vec2 halfDimensions = transform.getScale() * 0.5f;
        transform.setScale(1.0f);

        batch->setModelTransform(transform);
        auto geometryCache = DependencyManager::get<GeometryCache>();

        if (getIsSolid()) {
            glm::vec3 topLeft(-halfDimensions.x, -halfDimensions.y, 0.0f);
            glm::vec3 bottomRight(halfDimensions.x, halfDimensions.y, 0.0f);
            geometryCache->bindSimpleProgram(*batch);
            geometryCache->renderQuad(*batch, topLeft, bottomRight, rectangleColor, _geometryCacheID);
        } else {
            geometryCache->bindSimpleProgram(*batch, false, false, false, true, true);
            if (getIsDashedLine()) {
                glm::vec3 point1(-halfDimensions.x, -halfDimensions.y, 0.0f);
                glm::vec3 point2(halfDimensions.x, -halfDimensions.y, 0.0f);
                glm::vec3 point3(halfDimensions.x, halfDimensions.y, 0.0f);
                glm::vec3 point4(-halfDimensions.x, halfDimensions.y, 0.0f);

                geometryCache->renderDashedLine(*batch, point1, point2, rectangleColor, _rectGeometryIds[0]);
                geometryCache->renderDashedLine(*batch, point2, point3, rectangleColor, _rectGeometryIds[1]);
                geometryCache->renderDashedLine(*batch, point3, point4, rectangleColor, _rectGeometryIds[2]);
                geometryCache->renderDashedLine(*batch, point4, point1, rectangleColor, _rectGeometryIds[3]);
            } else {
                if (halfDimensions != _previousHalfDimensions) {
                    QVector<glm::vec3> border;
                    border << glm::vec3(-halfDimensions.x, -halfDimensions.y, 0.0f);
                    border << glm::vec3(halfDimensions.x, -halfDimensions.y, 0.0f);
                    border << glm::vec3(halfDimensions.x, halfDimensions.y, 0.0f);
                    border << glm::vec3(-halfDimensions.x, halfDimensions.y, 0.0f);
                    border << glm::vec3(-halfDimensions.x, -halfDimensions.y, 0.0f);
                    geometryCache->updateVertices(_geometryCacheID, border, rectangleColor);

                    _previousHalfDimensions = halfDimensions;
                }
                geometryCache->renderVertices(*batch, gpu::LINE_STRIP, _geometryCacheID);
            }
        }
    }
}

const render::ShapeKey Rectangle3DOverlay::getShapeKey() {
    auto builder = render::ShapeKey::Builder().withOwnPipeline();
    if (isTransparent()) {
        builder.withTranslucent();
    }
    return builder.build();
}

/**jsdoc
 * These are the properties of a <code>rectangle3d</code> {@link Overlays.OverlayType|OverlayType}.
 * @typedef {object} Overlays.Rectangle3DProperties
 * 
 * @property {string} type=rectangle3d - Has the value <code>"rectangle3d"</code>. <em>Read-only.</em>
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
 * @property {Vec2} dimensions=1,1 - The dimensions of the overlay. Synonyms: <code>scale</code>, <code>size</code>.
 */
void Rectangle3DOverlay::setProperties(const QVariantMap& properties) {
    Planar3DOverlay::setProperties(properties);
}

Rectangle3DOverlay* Rectangle3DOverlay::createClone() const {
    return new Rectangle3DOverlay(this);
}
