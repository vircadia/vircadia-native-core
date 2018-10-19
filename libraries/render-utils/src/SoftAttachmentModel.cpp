//
//  Created by Anthony J. Thibault on 12/17/15.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "SoftAttachmentModel.h"

SoftAttachmentModel::SoftAttachmentModel(QObject* parent, const Rig& rigOverride) :
    CauterizedModel(parent),
    _rigOverride(rigOverride) {
}

SoftAttachmentModel::~SoftAttachmentModel() {
}

// virtual
void SoftAttachmentModel::updateRig(float deltaTime, glm::mat4 parentTransform) {
    _needsUpdateClusterMatrices = true;
}

int SoftAttachmentModel::getJointIndexOverride(int i) const {
    QString name = _rig.nameOfJoint(i);
    if (name.isEmpty()) {
        return -1;
    }
    return _rigOverride.indexOfJoint(name);
}

// virtual
// use the _rigOverride matrices instead of the Model::_rig
void SoftAttachmentModel::updateClusterMatrices() {
    if (!_needsUpdateClusterMatrices) {
        return;
    }
    if (!isLoaded()) {
        return;
    }

    _needsUpdateClusterMatrices = false;

    const FBXGeometry& geometry = getFBXGeometry();

    for (int i = 0; i < (int) _meshStates.size(); i++) {
        MeshState& state = _meshStates[i];
        const FBXMesh& mesh = geometry.meshes.at(i);

        for (int j = 0; j < mesh.clusters.size(); j++) {
            const FBXCluster& cluster = mesh.clusters.at(j);

            // TODO: cache these look-ups as an optimization
            int jointIndexOverride = getJointIndexOverride(cluster.jointIndex);
            if (_useDualQuaternionSkinning) {
                glm::mat4 jointMatrix;
                if (jointIndexOverride >= 0 && jointIndexOverride < _rigOverride.getJointStateCount()) {
                    jointMatrix = _rigOverride.getJointTransform(jointIndexOverride);
                } else {
                    jointMatrix = _rig.getJointTransform(cluster.jointIndex);
                }

                glm::mat4 m;
                glm_mat4u_mul(jointMatrix, cluster.inverseBindMatrix, m);
                state.clusterDualQuaternions[j] = Model::TransformDualQuaternion(m);
            } else {
                glm::mat4 jointMatrix;
                if (jointIndexOverride >= 0 && jointIndexOverride < _rigOverride.getJointStateCount()) {
                    jointMatrix = _rigOverride.getJointTransform(jointIndexOverride);
                } else {
                    jointMatrix = _rig.getJointTransform(cluster.jointIndex);
                }

                glm_mat4u_mul(jointMatrix, cluster.inverseBindMatrix, state.clusterMatrices[j]);
            }
        }
    }

    // post the blender if we're not currently waiting for one to finish
    auto modelBlender = DependencyManager::get<ModelBlender>();
    if (_blendshapeBuffersInitialized && modelBlender->shouldComputeBlendshapes() && geometry.hasBlendedMeshes() && _blendshapeCoefficients != _blendedBlendshapeCoefficients) {
        _blendedBlendshapeCoefficients = _blendshapeCoefficients;
        modelBlender->noteRequiresBlend(getThisPointer());
    }
}
