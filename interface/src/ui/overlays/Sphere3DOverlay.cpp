//
//  Sphere3DOverlay.cpp
//  interface/src/ui/overlays
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Sphere3DOverlay.h"

#include <DependencyManager.h>
#include <GeometryCache.h>
#include <gpu/Batch.h>
#include <SharedUtil.h>

QString const Sphere3DOverlay::TYPE = "sphere";

// Sphere overlays should fit inside a cube of the specified dimensions, hence it needs to be a half unit sphere.
// However, the geometry cache renders a UNIT sphere, so we need to scale down.
static const float SPHERE_OVERLAY_SCALE = 0.5f;

Sphere3DOverlay::Sphere3DOverlay(const Sphere3DOverlay* Sphere3DOverlay) :
    Volume3DOverlay(Sphere3DOverlay)
{
}

/**jsdoc
* @typedef {object} Overlays.sphere3dProperties
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

void Sphere3DOverlay::render(RenderArgs* args) {
    if (!_renderVisible) {
        return; // do nothing if we're not visible
    }

    float alpha = getAlpha();
    xColor color = getColor();
    const float MAX_COLOR = 255.0f;
    glm::vec4 sphereColor(color.red / MAX_COLOR, color.green / MAX_COLOR, color.blue / MAX_COLOR, alpha);

    auto batch = args->_batch;

    if (batch) {
        batch->setModelTransform(getRenderTransform());

        auto geometryCache = DependencyManager::get<GeometryCache>();
        auto shapePipeline = args->_shapePipeline;
        if (!shapePipeline) {
            shapePipeline = _isSolid ? geometryCache->getOpaqueShapePipeline() : geometryCache->getWireShapePipeline();
        }

        if (_isSolid) {
            geometryCache->renderSolidSphereInstance(args, *batch, sphereColor, shapePipeline);
        } else {
            geometryCache->renderWireSphereInstance(args, *batch, sphereColor, shapePipeline);
        }
    }
}

const render::ShapeKey Sphere3DOverlay::getShapeKey() {
    auto builder = render::ShapeKey::Builder();
    if (isTransparent()) {
        builder.withTranslucent();
    }
    if (!getIsSolid()) {
        builder.withUnlit().withDepthBias();
    }
    return builder.build();
}

Sphere3DOverlay* Sphere3DOverlay::createClone() const {
    return new Sphere3DOverlay(this);
}

Transform Sphere3DOverlay::evalRenderTransform() {
    Transform transform = getTransform();
    transform.setScale(1.0f);  // ignore inherited scale from SpatiallyNestable
    transform.postScale(getDimensions() * SPHERE_OVERLAY_SCALE);

    return transform;
}
