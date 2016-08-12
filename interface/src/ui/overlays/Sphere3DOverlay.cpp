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

void Sphere3DOverlay::render(RenderArgs* args) {
    if (!_visible) {
        return; // do nothing if we're not visible
    }

    float alpha = getAlpha();
    xColor color = getColor();
    const float MAX_COLOR = 255.0f;
    glm::vec4 sphereColor(color.red / MAX_COLOR, color.green / MAX_COLOR, color.blue / MAX_COLOR, alpha);

    auto batch = args->_batch;

    if (batch) {
        Transform transform = _transform;
        transform.postScale(getDimensions() * SPHERE_OVERLAY_SCALE);
        batch->setModelTransform(transform);

        auto geometryCache = DependencyManager::get<GeometryCache>();
        auto pipeline = args->_pipeline;
        if (!pipeline) {
            pipeline = _isSolid ? geometryCache->getOpaqueShapePipeline() : geometryCache->getWireShapePipeline();
        }

        if (_isSolid) {
            geometryCache->renderSolidSphereInstance(*batch, sphereColor, pipeline);
        } else {
            geometryCache->renderWireSphereInstance(*batch, sphereColor, pipeline);
        }
    }
}

const render::ShapeKey Sphere3DOverlay::getShapeKey() {
    auto builder = render::ShapeKey::Builder();
    if (getAlpha() != 1.0f) {
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
