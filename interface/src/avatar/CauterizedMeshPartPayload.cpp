//
//  CauterizedMeshPartPayload.cpp
//  interface/src/renderer
//
//  Created by Andrew Meadows 2017.01.17
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "CauterizedMeshPartPayload.h"

#include <PerfStat.h>

#include "SkeletonModel.h"

using namespace render;

CauterizedMeshPartPayload::CauterizedMeshPartPayload(Model* model, int meshIndex, int partIndex, int shapeIndex, const Transform& transform, const Transform& offsetTransform)
    : ModelMeshPartPayload(model, meshIndex, partIndex, shapeIndex, transform, offsetTransform) {}

void CauterizedMeshPartPayload::updateTransformForSkinnedCauterizedMesh(const Transform& transform,
        const QVector<glm::mat4>& clusterMatrices,
        const QVector<glm::mat4>& cauterizedClusterMatrices) {
    _transform = transform;
    _cauterizedTransform = transform;

    if (clusterMatrices.size() > 0) {
        _worldBound = AABox();
        for (auto& clusterMatrix : clusterMatrices) {
            AABox clusterBound = _localBound;
            clusterBound.transform(clusterMatrix);
            _worldBound += clusterBound;
        }

        _worldBound.transform(transform);
        if (clusterMatrices.size() == 1) {
            _transform = _transform.worldTransform(Transform(clusterMatrices[0]));
            if (cauterizedClusterMatrices.size() != 0) {
                _cauterizedTransform = _cauterizedTransform.worldTransform(Transform(cauterizedClusterMatrices[0]));
            } else {
                _cauterizedTransform = _transform;
            }
        }
    } else {
        _worldBound = _localBound;
        _worldBound.transform(_drawTransform);
    }
}

void CauterizedMeshPartPayload::bindTransform(gpu::Batch& batch, const render::ShapePipeline::LocationsPointer locations, RenderArgs::RenderMode renderMode) const {
    // Still relying on the raw data from the model
    const Model::MeshState& state = _model->getMeshState(_meshIndex);
    SkeletonModel* skeleton = static_cast<SkeletonModel*>(_model);
    bool useCauterizedMesh = (renderMode != RenderArgs::RenderMode::SHADOW_RENDER_MODE) && skeleton->getEnableCauterization();

    if (state.clusterBuffer) {
        if (useCauterizedMesh) {
            const Model::MeshState& cState = skeleton->getCauterizeMeshState(_meshIndex);
            batch.setUniformBuffer(ShapePipeline::Slot::BUFFER::SKINNING, cState.clusterBuffer);
        } else {
            batch.setUniformBuffer(ShapePipeline::Slot::BUFFER::SKINNING, state.clusterBuffer);
        }
        batch.setModelTransform(_transform);
    } else {
        if (useCauterizedMesh) {
            batch.setModelTransform(_cauterizedTransform);
        } else {
            batch.setModelTransform(_transform);
        }
    }
}

