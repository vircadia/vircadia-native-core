//
//  RenderableBoxEntityItem.cpp
//  libraries/entities-renderer/src/
//
//  Created by Brad Hefta-Gaub on 8/6/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "RenderableBoxEntityItem.h"

#include <glm/gtx/quaternion.hpp>

#include <gpu/Batch.h>

#include <DeferredLightingEffect.h>
#include <ObjectMotionState.h>
#include <PerfStat.h>

#include "RenderableDebugableEntityItem.h"

EntityItemPointer RenderableBoxEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    return std::make_shared<RenderableBoxEntityItem>(entityID, properties);
}

void RenderableBoxEntityItem::render(RenderArgs* args) {
    PerformanceTimer perfTimer("RenderableBoxEntityItem::render");
    Q_ASSERT(getType() == EntityTypes::Box);
    glm::vec4 cubeColor(toGlm(getXColor()), getLocalRenderAlpha());
    
    Q_ASSERT(args->_batch);
    gpu::Batch& batch = *args->_batch;
    batch.setModelTransform(getTransformToCenter()); // we want to include the scale as well
    DependencyManager::get<DeferredLightingEffect>()->renderSolidCube(batch, 1.0f, cubeColor);

    RenderableDebugableEntityItem::render(this, args);
};
