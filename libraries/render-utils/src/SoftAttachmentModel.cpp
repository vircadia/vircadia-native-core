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


    for (int skinDeformerIndex = 0; skinDeformerIndex < (int)_meshStates.size(); skinDeformerIndex++) {
        MeshState& state = _meshStates[skinDeformerIndex];
        auto numClusters = state.getNumClusters();
        for (uint32_t clusterIndex = 0; clusterIndex < numClusters; clusterIndex++) {
            const auto& cbmov = _rig.getAnimSkeleton()->getClusterBindMatricesOriginalValues(skinDeformerIndex, clusterIndex);

            // TODO: cache these look-ups as an optimization
            int jointIndexOverride = getJointIndexOverride(cbmov.jointIndex);
            auto rig = &_rigOverride;
            if (jointIndexOverride >= 0 && jointIndexOverride < _rigOverride.getJointStateCount()) {
                rig = &_rig;
            }

            if (_useDualQuaternionSkinning) {
                auto jointPose = rig->getJointPose(cbmov.jointIndex);
                Transform jointTransform(jointPose.rot(), jointPose.scale(), jointPose.trans());
                Transform clusterTransform;
                Transform::mult(clusterTransform, jointTransform, cbmov.inverseBindTransform);
                state.clusterDualQuaternions[clusterIndex] = Model::TransformDualQuaternion(clusterTransform);
            } else {
                auto jointMatrix = rig->getJointTransform(cbmov.jointIndex);
                glm_mat4u_mul(jointMatrix, cbmov.inverseBindMatrix, state.clusterMatrices[clusterIndex]);
            }
        }
    }

    // post the blender if we're not currently waiting for one to finish
    auto modelBlender = DependencyManager::get<ModelBlender>();
    if (modelBlender->shouldComputeBlendshapes() && getHFMModel().hasBlendedMeshes() && _blendshapeCoefficients != _blendedBlendshapeCoefficients) {
        _blendedBlendshapeCoefficients = _blendshapeCoefficients;
        modelBlender->noteRequiresBlend(getThisPointer());
    }
}
