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
 * @typedef {object} Overlays.cube3dProperties
 * @property {number} borderSize - TODO
 * 
 * @property {Vec3} dimensions - The dimensions of the overlay. Synonyms: <code>scale</code>, <code>size</code>.
 *
 * @property {string} name - TODO
 * @property {Vec3} position - The position of the overlay center. Synonyms: <code>p1</code>, <code>point</code>, and
 *     <code>start</code>
 * @property {Vec3} localPosition - The local position of the overlay relative to its parent.
 * @property {Quat} rotation - The orientation of the overlay. Synonym: <code>orientation</code>.
 * @property {Quat} localRotation - The orientation of the overlay relative to its parent.
 * @property {boolean} isSolid - TODO w.r.t. isWire and isDashedLine. Synonyms: <ode>solid</code>, <code>isFilled</code>, and
 *     <code>filled</code>.
 * @property {boolean} isWire - TODO. Synonym: <code>wire</code>.
 * @property {boolean} isDashedLine - TODO. Synonym: <code>dashed</code>.
 * @property {boolean} ignoreRayIntersection - TODO.
 * @property {boolean} drawInFront - TODO.
 * @property {boolean} grabbable - TODO.
 * @property {Uuid} parentID - TODO.
 * @property {number} parentJointIndex - TODO. Integer.
 *
 * @property {string} type - TODO.
 * @property {RGB} color - TODO.
 * @property {number} alpha - TODO.
 * @property {number} pulseMax - TODO.
 * @property {number} pulseMin - TODO.
 * @property {number} pulsePeriod - TODO.
 * @property {number} alphaPulse - TODO.
 * @property {number} colorPulse - TODO.
 * @property {boolean} visible - TODO.
 * @property {string} anchor - TODO.
 */
QVariant Cube3DOverlay::getProperty(const QString& property) {
    if (property == "borderSize") {
        return _borderSize;
    }

    return Volume3DOverlay::getProperty(property);
}

Transform Cube3DOverlay::evalRenderTransform() {
    // TODO: handle registration point??
    glm::vec3 position = getPosition();
    glm::vec3 dimensions = getDimensions();
    glm::quat rotation = getRotation();

    Transform transform;
    transform.setScale(dimensions);
    transform.setTranslation(position);
    transform.setRotation(rotation);
    return transform;
}
