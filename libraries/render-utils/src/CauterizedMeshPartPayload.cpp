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

#include "CauterizedModel.h"

using namespace render;

CauterizedMeshPartPayload::CauterizedMeshPartPayload(ModelPointer model, int meshIndex, int partIndex, int shapeIndex, const Transform& transform, const Transform& offsetTransform)
    : ModelMeshPartPayload(model, meshIndex, partIndex, shapeIndex, transform, offsetTransform) {}

void CauterizedMeshPartPayload::updateClusterBuffer(const std::vector<TransformType>& clusterTransforms, const std::vector<TransformType>& cauterizedClusterTransforms) {
    ModelMeshPartPayload::updateClusterBuffer(clusterTransforms);

    if (cauterizedClusterTransforms.size() > 1) {
        if (!_cauterizedClusterBuffer) {
            _cauterizedClusterBuffer = std::make_shared<gpu::Buffer>(cauterizedClusterTransforms.size() * sizeof(TransformType),
                (const gpu::Byte*) cauterizedClusterTransforms.data());
        } else {
            _cauterizedClusterBuffer->setSubData(0, cauterizedClusterTransforms.size() * sizeof(TransformType),
                (const gpu::Byte*) cauterizedClusterTransforms.data());
        }
    }
}

void CauterizedMeshPartPayload::updateTransformForCauterizedMesh(const Transform& renderTransform) {
    _cauterizedTransform = renderTransform;
}

void CauterizedMeshPartPayload::bindTransform(gpu::Batch& batch, const render::ShapePipeline::LocationsPointer locations, RenderArgs::RenderMode renderMode) const {
    bool useCauterizedMesh = (renderMode != RenderArgs::RenderMode::SHADOW_RENDER_MODE && renderMode != RenderArgs::RenderMode::SECONDARY_CAMERA_RENDER_MODE) && _enableCauterization;
    if (useCauterizedMesh) {
        if (_cauterizedClusterBuffer) {
            batch.setUniformBuffer(ShapePipeline::Slot::BUFFER::SKINNING, _cauterizedClusterBuffer);
        }
        batch.setModelTransform(_cauterizedTransform);
    } else {
        ModelMeshPartPayload::bindTransform(batch, locations, renderMode);
    }
}

