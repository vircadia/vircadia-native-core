//
//  RenderableSphereEntityItem.cpp
//  interface/src
//
//  Created by Brad Hefta-Gaub on 8/6/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "RenderableSphereEntityItem.h"

#include <glm/gtx/quaternion.hpp>

#include <gpu/Batch.h>

#include <DependencyManager.h>
#include <DeferredLightingEffect.h>
#include <PerfStat.h>

#include "RenderableDebugableEntityItem.h"

EntityItemPointer RenderableSphereEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    return std::make_shared<RenderableSphereEntityItem>(entityID, properties);
}

void RenderableSphereEntityItem::render(RenderArgs* args) {
    PerformanceTimer perfTimer("RenderableSphereEntityItem::render");
    Q_ASSERT(getType() == EntityTypes::Sphere);
    glm::vec4 sphereColor(toGlm(getXColor()), getLocalRenderAlpha());
    
    // TODO: it would be cool to select different slices/stacks geometry based on the size of the sphere
    // and the distance to the viewer. This would allow us to reduce the triangle count for smaller spheres
    // that aren't close enough to see the tessellation and use larger triangle count for spheres that would
    // expose that effect
    const int SLICES = 15, STACKS = 15;
    
    Q_ASSERT(args->_batch);
    gpu::Batch& batch = *args->_batch;
    batch.setModelTransform(getTransformToCenter()); // use a transform with scale, rotation, registration point and translation
    DependencyManager::get<DeferredLightingEffect>()->renderSolidSphere(batch, 0.5f, SLICES, STACKS, sphereColor);

    RenderableDebugableEntityItem::render(this, args);
};
