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

void CauterizedMeshPartPayload::updateTransformForCauterizedMesh(
        const Transform& renderTransform,
        const gpu::BufferPointer& buffer) {
    _cauterizedTransform = renderTransform;
    _cauterizedClusterBuffer = buffer;
}

void CauterizedMeshPartPayload::bindTransform(gpu::Batch& batch, const render::ShapePipeline::LocationsPointer locations, RenderArgs::RenderMode renderMode) const {
    // Still relying on the raw data from the model
    bool useCauterizedMesh = (renderMode != RenderArgs::RenderMode::SHADOW_RENDER_MODE && renderMode != RenderArgs::RenderMode::SECONDARY_CAMERA_RENDER_MODE);
    if (useCauterizedMesh) {
        ModelPointer model = _model.lock();
        if (model) {
            CauterizedModel* skeleton = static_cast<CauterizedModel*>(model.get());
            useCauterizedMesh = useCauterizedMesh && skeleton->getEnableCauterization();
        } else {
            useCauterizedMesh = false;
        }
    }

    if (useCauterizedMesh) {
        if (_cauterizedClusterBuffer) {
            batch.setUniformBuffer(ShapePipeline::Slot::BUFFER::SKINNING, _cauterizedClusterBuffer);
        }
        batch.setModelTransform(_cauterizedTransform);
    } else {
        if (_clusterBuffer) {
            batch.setUniformBuffer(ShapePipeline::Slot::BUFFER::SKINNING, _clusterBuffer);
        }
        batch.setModelTransform(_transform);
    }
}

