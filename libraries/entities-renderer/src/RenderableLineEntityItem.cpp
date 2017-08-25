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

#include "RenderableLineEntityItem.h"

#include <gpu/Batch.h>
#include <PerfStat.h>

using namespace render;
using namespace render::entities;

void LineEntityRenderer::onRemoveFromSceneTyped(const TypedEntityPointer& entity) {
    if (_lineVerticesID != GeometryCache::UNKNOWN_ID) {
        auto geometryCache = DependencyManager::get<GeometryCache>();
        if (geometryCache) {
            geometryCache->releaseID(_lineVerticesID);
        }
    }
}

bool LineEntityRenderer::needsRenderUpdateFromTypedEntity(const TypedEntityPointer& entity) const {
    return entity->pointsChanged();
}

void LineEntityRenderer::doRenderUpdateAsynchronousTyped(const TypedEntityPointer& entity) {
    entity->resetPointsChanged();
    _linePoints = entity->getLinePoints();
    auto geometryCache = DependencyManager::get<GeometryCache>();
    if (_lineVerticesID == GeometryCache::UNKNOWN_ID) {
        _lineVerticesID = geometryCache->allocateID();
    }
    glm::vec4 lineColor(toGlm(entity->getXColor()), entity->getLocalRenderAlpha());
    geometryCache->updateVertices(_lineVerticesID, _linePoints, lineColor);
}

void LineEntityRenderer::doRender(RenderArgs* args) {
    if (_lineVerticesID == GeometryCache::UNKNOWN_ID) {
        return;
    }

    PerformanceTimer perfTimer("RenderableLineEntityItem::render");
    Q_ASSERT(args->_batch);
    gpu::Batch& batch = *args->_batch;
    Transform transform = Transform();
    transform.setTranslation(_modelTransform.getTranslation());
    transform.setRotation(_modelTransform.getRotation());
    batch.setModelTransform(transform);
    if (_linePoints.size() > 1) {
        DependencyManager::get<GeometryCache>()->bindSimpleProgram(batch);
        DependencyManager::get<GeometryCache>()->renderVertices(batch, gpu::LINE_STRIP, _lineVerticesID);
    }
}
