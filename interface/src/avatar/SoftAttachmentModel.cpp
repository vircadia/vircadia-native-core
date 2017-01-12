//
//  SoftAttachmentModel.cpp
//  interface/src/avatar
//
//  Created by Anthony J. Thibault on 12/17/15.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "SoftAttachmentModel.h"
#include "InterfaceLogging.h"

SoftAttachmentModel::SoftAttachmentModel(RigPointer rig, QObject* parent, RigPointer rigOverride) :
    Model(rig, parent),
    _rigOverride(rigOverride) {
    assert(_rig);
    assert(_rigOverride);
}

SoftAttachmentModel::~SoftAttachmentModel() {
}

// virtual
void SoftAttachmentModel::updateRig(float deltaTime, glm::mat4 parentTransform) {
    _needsUpdateClusterMatrices = true;
}

int SoftAttachmentModel::getJointIndexOverride(int i) const {
    QString name = _rig->nameOfJoint(i);
    if (name.isEmpty()) {
        return -1;
    }
    return _rigOverride->indexOfJoint(name);
}

// virtual
// use the _rigOverride matrices instead of the Model::_rig
void SoftAttachmentModel::updateClusterMatrices() {
    if (!_needsUpdateClusterMatrices) {
        return;
    }
    _needsUpdateClusterMatrices = false;

    const FBXGeometry& geometry = getFBXGeometry();

    for (int i = 0; i < _meshStates.size(); i++) {
        MeshState& state = _meshStates[i];
        const FBXMesh& mesh = geometry.meshes.at(i);

        for (int j = 0; j < mesh.clusters.size(); j++) {
            const FBXCluster& cluster = mesh.clusters.at(j);

            // TODO: cache these look-ups as an optimization
            int jointIndexOverride = getJointIndexOverride(cluster.jointIndex);
            glm::mat4 jointMatrix;
            if (jointIndexOverride >= 0 && jointIndexOverride < _rigOverride->getJointStateCount()) {
                jointMatrix = _rigOverride->getJointTransform(jointIndexOverride);
            } else {
                jointMatrix = _rig->getJointTransform(cluster.jointIndex);
            }
#if GLM_ARCH & GLM_ARCH_SSE2
            glm::mat4 out, inverseBindMatrix = cluster.inverseBindMatrix;
            glm_mat4_mul((glm_vec4*)&jointMatrix, (glm_vec4*)&inverseBindMatrix, (glm_vec4*)&out);
            state.clusterMatrices[j] = out;
#else
            state.clusterMatrices[j] = jointMatrix * cluster.inverseBindMatrix;
#endif
        }

        // Once computed the cluster matrices, update the buffer(s)
        if (mesh.clusters.size() > 1) {
            if (!state.clusterBuffer) {
                state.clusterBuffer = std::make_shared<gpu::Buffer>(state.clusterMatrices.size() * sizeof(glm::mat4),
                                                                    (const gpu::Byte*) state.clusterMatrices.constData());
            } else {
                state.clusterBuffer->setSubData(0, state.clusterMatrices.size() * sizeof(glm::mat4),
                                                (const gpu::Byte*) state.clusterMatrices.constData());
            }
        }
    }

    // post the blender if we're not currently waiting for one to finish
    if (geometry.hasBlendedMeshes() && _blendshapeCoefficients != _blendedBlendshapeCoefficients) {
        _blendedBlendshapeCoefficients = _blendshapeCoefficients;
        DependencyManager::get<ModelBlender>()->noteRequiresBlend(getThisPointer());
    }
}
