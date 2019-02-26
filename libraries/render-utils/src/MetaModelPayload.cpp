//
//  MetaModelPayload.cpp
//
//  Created by Sam Gondelman on 10/9/18.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "MetaModelPayload.h"

#include "AbstractViewStateInterface.h"
#include "MeshPartPayload.h"

void MetaModelPayload::setBlendedVertices(int blendNumber, const QVector<BlendshapeOffset>& blendshapeOffsets, const QVector<int>& blendedMeshSizes, const render::ItemIDs& subRenderItems) {
    PROFILE_RANGE(render, __FUNCTION__);
    if (blendNumber < _appliedBlendNumber) {
        return;
    }
    _appliedBlendNumber = blendNumber;

    // We have fewer meshes than before.  Invalidate everything
    if (blendedMeshSizes.length() < (int)_blendshapeBuffers.size()) {
        _blendshapeBuffers.clear();
    }

    int index = 0;
    for (int i = 0; i < blendedMeshSizes.size(); i++) {
        int numVertices = blendedMeshSizes.at(i);

        // This mesh isn't blendshaped
        if (numVertices == 0) {
            _blendshapeBuffers.erase(i);
            continue;
        }

        const auto& buffer = _blendshapeBuffers.find(i);
        const auto blendShapeBufferSize = numVertices * sizeof(BlendshapeOffset);
        if (buffer == _blendshapeBuffers.end()) {
            _blendshapeBuffers[i] = std::make_shared<gpu::Buffer>(blendShapeBufferSize, (gpu::Byte*) blendshapeOffsets.constData() + index * sizeof(BlendshapeOffset), blendShapeBufferSize);
        } else {
            buffer->second->setData(blendShapeBufferSize, (gpu::Byte*) blendshapeOffsets.constData() + index * sizeof(BlendshapeOffset));
        }

        index += numVertices;
    }

    render::Transaction transaction;
    for (auto& id : subRenderItems) {
        transaction.updateItem<ModelMeshPartPayload>(id, [this, blendedMeshSizes](ModelMeshPartPayload& data) {
            data.setBlendshapeBuffer(_blendshapeBuffers, blendedMeshSizes);
        });
    }
    AbstractViewStateInterface::instance()->getMain3DScene()->enqueueTransaction(transaction);
}
