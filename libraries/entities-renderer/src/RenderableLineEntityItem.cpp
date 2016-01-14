//
//  RenderableLineEntityItem.cpp
//  libraries/entities-renderer/src/
//
//  Created by Seth Alves on 5/11/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <glm/gtx/quaternion.hpp>

#include <gpu/Batch.h>
#include <GeometryCache.h>

#include <PerfStat.h>

#include "RenderableLineEntityItem.h"

EntityItemPointer RenderableLineEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    EntityItemPointer entity{ new RenderableLineEntityItem(entityID) };
    entity->setProperties(properties);
    return entity;
}

void RenderableLineEntityItem::updateGeometry() {
    auto geometryCache = DependencyManager::get<GeometryCache>();
    if (_lineVerticesID == GeometryCache::UNKNOWN_ID) {
        _lineVerticesID = geometryCache ->allocateID();
    }
    if (_pointsChanged) {
        glm::vec4 lineColor(toGlm(getXColor()), getLocalRenderAlpha());
        geometryCache->updateVertices(_lineVerticesID, getLinePoints(), lineColor);
        _pointsChanged = false;
    }
}




void RenderableLineEntityItem::render(RenderArgs* args) {
    PerformanceTimer perfTimer("RenderableLineEntityItem::render");
    Q_ASSERT(getType() == EntityTypes::Line);
    updateGeometry();
    
    Q_ASSERT(args->_batch);
    gpu::Batch& batch = *args->_batch;
    Transform transform = Transform();
    transform.setTranslation(getPosition());
    transform.setRotation(getRotation());
    batch.setModelTransform(transform);

    if (getLinePoints().size() > 1) {
        DependencyManager::get<GeometryCache>()->bindSimpleProgram(batch);
        DependencyManager::get<GeometryCache>()->renderVertices(batch, gpu::LINE_STRIP, _lineVerticesID);
    }
};
