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
#include <GeometryCache.h>
#include <ObjectMotionState.h>
#include <PerfStat.h>

#include "RenderableDebugableEntityItem.h"

EntityItemPointer RenderableBoxEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    return std::make_shared<RenderableBoxEntityItem>(entityID, properties);
}

void RenderableBoxEntityItem::setUserData(const QString& value) {
    if (value != getUserData()) {
        BoxEntityItem::setUserData(value);
        _procedural.reset();
    }
}

void RenderableBoxEntityItem::render(RenderArgs* args) {
    PerformanceTimer perfTimer("RenderableBoxEntityItem::render");
    Q_ASSERT(getType() == EntityTypes::Box);
    Q_ASSERT(args->_batch);
    gpu::Batch& batch = *args->_batch;
    batch.setModelTransform(getTransformToCenter()); // we want to include the scale as well
    
    if (!_procedural) {
        _procedural.reset(new ProceduralInfo(this));
    }

    if (_procedural->ready()) {
        _procedural->prepare(batch);
        DependencyManager::get<GeometryCache>()->renderUnitCube(batch);
    } else {
        glm::vec4 cubeColor(toGlm(getXColor()), getLocalRenderAlpha());
        DependencyManager::get<DeferredLightingEffect>()->renderSolidCube(batch, 1.0f, cubeColor);
    }

    RenderableDebugableEntityItem::render(this, args);
};
