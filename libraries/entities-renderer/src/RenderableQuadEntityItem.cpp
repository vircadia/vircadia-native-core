//
//  RenderableQuadEntityItem.cpp
//  libraries/entities-renderer/src/
//
//  Created by Eric Levin on 6/22/15
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <glm/gtx/quaternion.hpp>

#include <gpu/GPUConfig.h>
#include <gpu/Batch.h>
#include <GeometryCache.h>

#include <DeferredLightingEffect.h>
#include <PerfStat.h>

#include "RenderableQuadEntityItem.h"

EntityItemPointer RenderableQuadEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    return EntityItemPointer(new RenderableQuadEntityItem(entityID, properties));
}

void RenderableQuadEntityItem::updateGeometry() {
    auto geometryCache = DependencyManager::get<GeometryCache>();
    if (_lineVerticesID == GeometryCache::UNKNOWN_ID) {
        _lineVerticesID = geometryCache ->allocateID();
    }
    if (_pointsChanged) {
        glm::vec4 lineColor(toGlm(getXColor()), getLocalRenderAlpha());
        geometryCache->updateVertices(_lineVerticesID, getQuadVertices(), lineColor);
        _pointsChanged = false;
    }
}

void RenderableQuadEntityItem::render(RenderArgs* args) {
    PerformanceTimer perfTimer("RenderableQuadEntityItem::render");
    Q_ASSERT(getType() == EntityTypes::Quad);
    updateGeometry();
    
    Q_ASSERT(args->_batch);
    gpu::Batch& batch = *args->_batch;
    Transform transform = Transform();
    transform.setTranslation(getPosition());
    batch.setModelTransform(transform);
    

    DependencyManager::get<DeferredLightingEffect>()->bindSimpleProgram(batch);
    DependencyManager::get<GeometryCache>()->renderVertices(batch, gpu::TRIANGLE_STRIP, _lineVerticesID);

    
    RenderableDebugableEntityItem::render(this, args);
};
