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

LineEntityRenderer::LineEntityRenderer(const EntityItemPointer& entity) :
        Parent(entity) {
    _lineVerticesID = DependencyManager::get<GeometryCache>()->allocateID();
}

LineEntityRenderer::~LineEntityRenderer() {
    auto geometryCache = DependencyManager::get<GeometryCache>();
    if (geometryCache) {
        geometryCache->releaseID(_lineVerticesID);
        for (auto id : _glowGeometryIDs) {
            geometryCache->releaseID(id);
        }
    }
}

bool LineEntityRenderer::isTransparent() const {
    return Parent::isTransparent() || _glow > 0.0f || _alpha < 1.0f;
}

bool LineEntityRenderer::needsRenderUpdateFromTypedEntity(const TypedEntityPointer& entity) const {
    if (entity->pointsChanged()) {
        return true;
    }

    if (_color != entity->getColor()) {
        return true;
    }

    if (_alpha != entity->getAlpha()) {
        return true;
    }

    if (_lineWidth != entity->getLineWidth()) {
        return true;
    }

    if (_glow != entity->getGlow()) {
        return true;
    }

    return false;
}

void LineEntityRenderer::doRenderUpdateAsynchronousTyped(const TypedEntityPointer& entity) {
    entity->resetPointsChanged();
    _linePoints = entity->getLinePoints();
    _color = entity->getColor();
    _alpha = entity->getAlpha();
    _lineWidth = entity->getLineWidth();
    _glow = entity->getGlow();

    // TODO: take into account _lineWidth
    geometryCache->updateVertices(_lineVerticesID, _linePoints, lineColor);
}

void LineEntityRenderer::doRender(RenderArgs* args) {
    PerformanceTimer perfTimer("RenderableLineEntityItem::render");

    Q_ASSERT(args->_batch);
    gpu::Batch& batch = *args->_batch;

    const auto& modelTransform = getModelTransform();
    Transform transform = Transform();
    transform.setTranslation(modelTransform.getTranslation());
    transform.setRotation(modelTransform.getRotation());
    batch.setModelTransform(transform);
    if (_linePoints.size() > 1) {
        DependencyManager::get<GeometryCache>()->bindSimpleProgram(batch);
        DependencyManager::get<GeometryCache>()->renderVertices(batch, gpu::LINE_STRIP, _lineVerticesID);
    }
}
