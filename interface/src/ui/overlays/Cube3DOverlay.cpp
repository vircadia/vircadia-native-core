//
//  Cube3DOverlay.cpp
//  interface/src/ui/overlays
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// include this before QGLWidget, which includes an earlier version of OpenGL
#include "Cube3DOverlay.h"

#include <SharedUtil.h>
#include <StreamUtils.h>
#include <GeometryCache.h>
#include <DependencyManager.h>

QString const Cube3DOverlay::TYPE = "cube";

Cube3DOverlay::Cube3DOverlay() {
    auto geometryCache = DependencyManager::get<GeometryCache>();
    for (size_t i = 0; i < _geometryIds.size(); ++i) {
        _geometryIds[i] = geometryCache->allocateID();
    }
}

Cube3DOverlay::Cube3DOverlay(const Cube3DOverlay* cube3DOverlay) :
    Volume3DOverlay(cube3DOverlay)
{
    auto geometryCache = DependencyManager::get<GeometryCache>();
    for (size_t i = 0; i < _geometryIds.size(); ++i) {
        _geometryIds[i] = geometryCache->allocateID();
    }
}

Cube3DOverlay::~Cube3DOverlay() {
    auto geometryCache = DependencyManager::get<GeometryCache>();
    if (geometryCache) {
        for (size_t i = 0; i < _geometryIds.size(); ++i) {
            geometryCache->releaseID(_geometryIds[i]);
        }
    }
}

void Cube3DOverlay::render(RenderArgs* args) {
    if (!_renderVisible) {
        return; // do nothing if we're not visible
    }

    float alpha = getAlpha();
    xColor color = getColor();
    const float MAX_COLOR = 255.0f;
    glm::vec4 cubeColor(color.red / MAX_COLOR, color.green / MAX_COLOR, color.blue / MAX_COLOR, alpha);



    auto batch = args->_batch;
    if (batch) {
        Transform transform = getRenderTransform();
        auto geometryCache = DependencyManager::get<GeometryCache>();
        auto shapePipeline = args->_shapePipeline;
        if (!shapePipeline) {
            shapePipeline = _isSolid ? geometryCache->getOpaqueShapePipeline() : geometryCache->getWireShapePipeline();
        }

        if (_isSolid) {
            batch->setModelTransform(transform);
            geometryCache->renderSolidCubeInstance(args, *batch, cubeColor, shapePipeline);
        } else {
            geometryCache->bindSimpleProgram(*batch, false, false, false, true, true);
            if (getIsDashedLine()) {
                auto dimensions = transform.getScale();
                transform.setScale(1.0f);
                batch->setModelTransform(transform);

                glm::vec3 halfDimensions = dimensions / 2.0f;
                glm::vec3 bottomLeftNear(-halfDimensions.x, -halfDimensions.y, -halfDimensions.z);
                glm::vec3 bottomRightNear(halfDimensions.x, -halfDimensions.y, -halfDimensions.z);
                glm::vec3 topLeftNear(-halfDimensions.x, halfDimensions.y, -halfDimensions.z);
                glm::vec3 topRightNear(halfDimensions.x, halfDimensions.y, -halfDimensions.z);

                glm::vec3 bottomLeftFar(-halfDimensions.x, -halfDimensions.y, halfDimensions.z);
                glm::vec3 bottomRightFar(halfDimensions.x, -halfDimensions.y, halfDimensions.z);
                glm::vec3 topLeftFar(-halfDimensions.x, halfDimensions.y, halfDimensions.z);
                glm::vec3 topRightFar(halfDimensions.x, halfDimensions.y, halfDimensions.z);

                geometryCache->renderDashedLine(*batch, bottomLeftNear, bottomRightNear, cubeColor, _geometryIds[0]);
                geometryCache->renderDashedLine(*batch, bottomRightNear, bottomRightFar, cubeColor, _geometryIds[1]);
                geometryCache->renderDashedLine(*batch, bottomRightFar, bottomLeftFar, cubeColor, _geometryIds[2]);
                geometryCache->renderDashedLine(*batch, bottomLeftFar, bottomLeftNear, cubeColor, _geometryIds[3]);

                geometryCache->renderDashedLine(*batch, topLeftNear, topRightNear, cubeColor, _geometryIds[4]);
                geometryCache->renderDashedLine(*batch, topRightNear, topRightFar, cubeColor, _geometryIds[5]);
                geometryCache->renderDashedLine(*batch, topRightFar, topLeftFar, cubeColor, _geometryIds[6]);
                geometryCache->renderDashedLine(*batch, topLeftFar, topLeftNear, cubeColor, _geometryIds[7]);

                geometryCache->renderDashedLine(*batch, bottomLeftNear, topLeftNear, cubeColor, _geometryIds[8]);
                geometryCache->renderDashedLine(*batch, bottomRightNear, topRightNear, cubeColor, _geometryIds[9]);
                geometryCache->renderDashedLine(*batch, bottomLeftFar, topLeftFar, cubeColor, _geometryIds[10]);
                geometryCache->renderDashedLine(*batch, bottomRightFar, topRightFar, cubeColor, _geometryIds[11]);

            } else {
                batch->setModelTransform(transform);
                geometryCache->renderWireCubeInstance(args, *batch, cubeColor, shapePipeline);
            }
        }
    }
}

const render::ShapeKey Cube3DOverlay::getShapeKey() {
    auto builder = render::ShapeKey::Builder();
    if (isTransparent()) {
        builder.withTranslucent();
    }
    if (!getIsSolid()) {
        builder.withUnlit().withDepthBias();
    }
    return builder.build();
}

Cube3DOverlay* Cube3DOverlay::createClone() const {
    return new Cube3DOverlay(this);
}

void Cube3DOverlay::setProperties(const QVariantMap& properties) {
    Volume3DOverlay::setProperties(properties);

    auto borderSize = properties["borderSize"];

    if (borderSize.isValid()) {
        float value = borderSize.toFloat();
        setBorderSize(value);
    }
}

/**jsdoc
 * These are the properties of a <code>cube</code> {@link Overlays.OverlayType|OverlayType}.
 * @typedef {object} Overlays.CubeProperties
 * 
 * @property {string} type=cube - Has the value <code>"cube"</code>. <em>Read-only.</em>
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
 * @property {Vec3} dimensions - The dimensions of the overlay. Synonyms: <code>scale</code>, <code>size</code>.
 *
 * @property {number} borderSize - Not used.
 */
QVariant Cube3DOverlay::getProperty(const QString& property) {
    if (property == "borderSize") {
        return _borderSize;
    }

    return Volume3DOverlay::getProperty(property);
}

Transform Cube3DOverlay::evalRenderTransform() {
    // TODO: handle registration point??
    glm::vec3 position = getWorldPosition();
    glm::vec3 dimensions = getDimensions();
    glm::quat rotation = getWorldOrientation();

    Transform transform;
    transform.setScale(dimensions);
    transform.setTranslation(position);
    transform.setRotation(rotation);
    return transform;
}
