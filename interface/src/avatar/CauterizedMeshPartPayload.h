//
//  CauterizedModelMeshPartPayload.h
//  interface/src/avatar
//
//  Created by AndrewMeadows 2017.01.17
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_CauterizedMeshPartPayload_h
#define hifi_CauterizedMeshPartPayload_h

#include <MeshPartPayload.h>

class CauterizedMeshPartPayload : public ModelMeshPartPayload {
public:
    CauterizedMeshPartPayload(Model* model, int meshIndex, int partIndex, int shapeIndex, const Transform& transform, const Transform& offsetTransform);
    void updateTransformForSkinnedCauterizedMesh(const Transform& transform,
            const QVector<glm::mat4>& clusterMatrices,
            const QVector<glm::mat4>& cauterizedClusterMatrices);

    void bindTransform(gpu::Batch& batch, const render::ShapePipeline::LocationsPointer locations, RenderArgs::RenderMode renderMode) const override;
private:
    Transform _cauterizedTransform;
};

#endif // hifi_CauterizedMeshPartPayload_h
