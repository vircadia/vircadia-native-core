//
//  Created by AndrewMeadows 2017.01.17
//  Copyright 2013-2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_CauterizedMeshPartPayload_h
#define hifi_CauterizedMeshPartPayload_h

#include "MeshPartPayload.h"

class CauterizedMeshPartPayload : public ModelMeshPartPayload {
public:
    CauterizedMeshPartPayload(ModelPointer model, int meshIndex, int partIndex, int shapeIndex, const Transform& transform, const uint64_t& created);

    // matrix palette skinning
    void updateClusterBuffer(const std::vector<glm::mat4>& clusterMatrices,
                             const std::vector<glm::mat4>& cauterizedClusterMatrices);

    // dual quaternion skinning
    void updateClusterBuffer(const std::vector<Model::TransformDualQuaternion>& clusterDualQuaternions,
                             const std::vector<Model::TransformDualQuaternion>& cauterizedClusterQuaternions);

    void updateTransformForCauterizedMesh(const Transform& modelTransform, const Model::MeshState& meshState, bool useDualQuaternionSkinning);

    void bindTransform(gpu::Batch& batch, const Transform& transform, RenderArgs::RenderMode renderMode) const override;

    void setEnableCauterization(bool enableCauterization) { _enableCauterization = enableCauterization; }

private:
    gpu::BufferPointer _cauterizedClusterBuffer;
    Transform _cauterizedTransform;
    bool _enableCauterization { false };
};

#endif // hifi_CauterizedMeshPartPayload_h
