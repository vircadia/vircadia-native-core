//
//  MetaModelPayload.h
//
//  Created by Sam Gondelman on 10/9/18.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_MetaModelPayload_h
#define hifi_MetaModelPayload_h

#include <unordered_map>

#include "Model.h"

#include "gpu/Buffer.h"

class MetaModelPayload {
public:
    void setBlendedVertices(int blendNumber, const QVector<BlendshapeOffset>& blendshapeOffsets, const QVector<int>& blendedMeshSizes, const render::ItemIDs& subRenderItems);

private:
    std::unordered_map<int, gpu::BufferPointer> _blendshapeBuffers;
    int _appliedBlendNumber { 0 };

};

#endif