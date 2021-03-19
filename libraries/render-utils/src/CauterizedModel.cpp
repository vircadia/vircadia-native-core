//
//  Created by Andrew Meadows 2017.01.17
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "CauterizedModel.h"

#include <PerfStat.h>
#include <DualQuaternion.h>

#include "AbstractViewStateInterface.h"
#include "MeshPartPayload.h"
#include "CauterizedMeshPartPayload.h"
#include "RenderUtilsLogging.h"

CauterizedModel::CauterizedModel(QObject* parent) :
        Model(parent) {
}

CauterizedModel::~CauterizedModel() {
}

void CauterizedModel::deleteGeometry() {
    Model::deleteGeometry();
    _cauterizeMeshStates.clear();
}

bool CauterizedModel::updateGeometry() {
    bool needsFullUpdate = Model::updateGeometry();
    if (_isCauterized && needsFullUpdate) {
        assert(_cauterizeMeshStates.empty());
        const HFMModel& hfmModel = getHFMModel();
        foreach (const HFMMesh& mesh, hfmModel.meshes) {
            Model::MeshState state;
            if (_useDualQuaternionSkinning) {
                state.clusterDualQuaternions.resize(mesh.clusters.size());
                _cauterizeMeshStates.append(state);
            } else {
                state.clusterMatrices.resize(mesh.clusters.size());
                _cauterizeMeshStates.append(state);
            }
        }
    }
    return needsFullUpdate;
}

void CauterizedModel::createRenderItemSet() {
    if (_isCauterized) {
        assert(isLoaded());
        const auto& meshes = _renderGeometry->getMeshes();

        // all of our mesh vectors must match in size
        if (meshes.size() != _meshStates.size()) {
            qCDebug(renderutils) << "WARNING!!!! Mesh Sizes don't match! We will not segregate mesh groups yet.";
            return;
        }

        // We should not have any existing renderItems if we enter this section of code
        Q_ASSERT(_modelMeshRenderItems.isEmpty());

        _modelMeshRenderItems.clear();
        _modelMeshMaterialNames.clear();
        _modelMeshRenderItemShapes.clear();

        Transform transform;
        transform.setTranslation(_translation);
        transform.setRotation(_rotation);

        // Run through all of the meshes, and place them into their segregated, but unsorted buckets
        int shapeID = 0;
        uint32_t numMeshes = (uint32_t)meshes.size();
        for (uint32_t i = 0; i < numMeshes; i++) {
            const auto& mesh = meshes.at(i);
            if (!mesh) {
                continue;
            }

            // Create the render payloads
            int numParts = (int)mesh->getNumParts();
            for (int partIndex = 0; partIndex < numParts; partIndex++) {
                auto ptr = std::make_shared<CauterizedMeshPartPayload>(shared_from_this(), i, partIndex, shapeID, transform, _created);
                _modelMeshRenderItems << std::static_pointer_cast<ModelMeshPartPayload>(ptr);
                auto material = getGeometry()->getShapeMaterial(shapeID);
                _modelMeshMaterialNames.push_back(material ? material->getName() : "");
                _modelMeshRenderItemShapes.emplace_back(ShapeInfo{ (int)i });
                shapeID++;
            }
        }
    } else {
        Model::createRenderItemSet();
    }
}

void CauterizedModel::updateClusterMatrices() {
    PerformanceTimer perfTimer("CauterizedModel::updateClusterMatrices");

    if (!_needsUpdateClusterMatrices || !isLoaded()) {
        return;
    }
    _needsUpdateClusterMatrices = false;
    const HFMModel& hfmModel = getHFMModel();

    for (int i = 0; i < (int)_meshStates.size(); i++) {
        Model::MeshState& state = _meshStates[i];
        const HFMMesh& mesh = hfmModel.meshes.at(i);
        int meshIndex = i;

        for (int j = 0; j < mesh.clusters.size(); j++) {
            const HFMCluster& cluster = mesh.clusters.at(j);
            int clusterIndex = j;

            if (_useDualQuaternionSkinning) {
                auto jointPose = _rig.getJointPose(cluster.jointIndex);
                Transform jointTransform(jointPose.rot(), jointPose.scale(), jointPose.trans());
                Transform clusterTransform;
                Transform::mult(clusterTransform, jointTransform, _rig.getAnimSkeleton()->getClusterBindMatricesOriginalValues(meshIndex, clusterIndex).inverseBindTransform);
                state.clusterDualQuaternions[j] = Model::TransformDualQuaternion(clusterTransform);
                state.clusterDualQuaternions[j].setCauterizationParameters(0.0f, jointPose.trans());
            } else {
                auto jointMatrix = _rig.getJointTransform(cluster.jointIndex);
                glm_mat4u_mul(jointMatrix, _rig.getAnimSkeleton()->getClusterBindMatricesOriginalValues(meshIndex, clusterIndex).inverseBindMatrix, state.clusterMatrices[j]);
            }
        }
    }

    // as an optimization, don't build cautrizedClusterMatrices if the boneSet is empty.
    if (!_cauterizeBoneSet.empty()) {

        AnimPose cauterizePose = _rig.getJointPose(_rig.indexOfJoint("Neck"));
        cauterizePose.scale() = glm::vec3(0.0001f, 0.0001f, 0.0001f);

        static const glm::mat4 zeroScale(
            glm::vec4(0.0001f, 0.0f, 0.0f, 0.0f),
            glm::vec4(0.0f, 0.0001f, 0.0f, 0.0f),
            glm::vec4(0.0f, 0.0f, 0.0001f, 0.0f),
            glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
        auto cauterizeMatrix = _rig.getJointTransform(_rig.indexOfJoint("Neck")) * zeroScale;

        for (int i = 0; i < _cauterizeMeshStates.size(); i++) {
            Model::MeshState& state = _cauterizeMeshStates[i];
            const HFMMesh& mesh = hfmModel.meshes.at(i);
            int meshIndex = i;

            for (int j = 0; j < mesh.clusters.size(); j++) {
                const HFMCluster& cluster = mesh.clusters.at(j);
                int clusterIndex = j;

                if (_useDualQuaternionSkinning) {
                    if (_cauterizeBoneSet.find(cluster.jointIndex) == _cauterizeBoneSet.end()) {
                        // not cauterized so just copy the value from the non-cauterized version.
                        state.clusterDualQuaternions[j] = _meshStates[i].clusterDualQuaternions[j];
                    } else {
                        Transform jointTransform(cauterizePose.rot(), cauterizePose.scale(), cauterizePose.trans());
                        Transform clusterTransform;
                        Transform::mult(clusterTransform, jointTransform, _rig.getAnimSkeleton()->getClusterBindMatricesOriginalValues(meshIndex, clusterIndex).inverseBindTransform);
                        state.clusterDualQuaternions[j] = Model::TransformDualQuaternion(clusterTransform);
                        state.clusterDualQuaternions[j].setCauterizationParameters(1.0f, cauterizePose.trans());
                    }
                } else {
                    if (_cauterizeBoneSet.find(cluster.jointIndex) == _cauterizeBoneSet.end()) {
                        // not cauterized so just copy the value from the non-cauterized version.
                        state.clusterMatrices[j] = _meshStates[i].clusterMatrices[j];
                    } else {
                        glm_mat4u_mul(cauterizeMatrix, _rig.getAnimSkeleton()->getClusterBindMatricesOriginalValues(meshIndex, clusterIndex).inverseBindMatrix, state.clusterMatrices[j]);
                    }
                }
            }
        }
    }

    updateBlendshapes();
}

void CauterizedModel::updateRenderItems() {
    if (_isCauterized) {
        if (!_addedToScene) {
            return;
        }
        _needsUpdateClusterMatrices = true;
        _renderItemsNeedUpdate = false;

        // queue up this work for later processing, at the end of update and just before rendering.
        // the application will ensure only the last lambda is actually invoked.
        void* key = (void*)this;
        std::weak_ptr<CauterizedModel> weakSelf = std::dynamic_pointer_cast<CauterizedModel>(shared_from_this());
        AbstractViewStateInterface::instance()->pushPostUpdateLambda(key, [weakSelf]() {
            // do nothing, if the model has already been destroyed.
            auto self = weakSelf.lock();
            if (!self || !self->isLoaded()) {
                return;
            }

            // lazy update of cluster matrices used for rendering.  We need to update them here, so we can correctly update the bounding box.
            self->updateClusterMatrices();

            render::ScenePointer scene = AbstractViewStateInterface::instance()->getMain3DScene();

            Transform modelTransform;
            modelTransform.setTranslation(self->getTranslation());
            modelTransform.setRotation(self->getRotation());

            PrimitiveMode primitiveMode = self->getPrimitiveMode();
            auto renderItemKeyGlobalFlags = self->getRenderItemKeyGlobalFlags();
            bool enableCauterization = self->getEnableCauterization();

            render::Transaction transaction;
            for (int i = 0; i < (int)self->_modelMeshRenderItemIDs.size(); i++) {

                auto itemID = self->_modelMeshRenderItemIDs[i];
                auto meshIndex = self->_modelMeshRenderItemShapes[i].meshIndex;

                const auto& meshState = self->getMeshState(meshIndex);
                const auto& cauterizedMeshState = self->getCauterizeMeshState(meshIndex);

                bool invalidatePayloadShapeKey = self->shouldInvalidatePayloadShapeKey(meshIndex);
                bool useDualQuaternionSkinning = self->getUseDualQuaternionSkinning();

                transaction.updateItem<ModelMeshPartPayload>(itemID, [modelTransform, meshState, useDualQuaternionSkinning, cauterizedMeshState, invalidatePayloadShapeKey,
                        primitiveMode, renderItemKeyGlobalFlags, enableCauterization](ModelMeshPartPayload& mmppData) {
                    CauterizedMeshPartPayload& data = static_cast<CauterizedMeshPartPayload&>(mmppData);
                    if (useDualQuaternionSkinning) {
                        data.updateClusterBuffer(meshState.clusterDualQuaternions,
                                                 cauterizedMeshState.clusterDualQuaternions);
                        data.computeAdjustedLocalBound(meshState.clusterDualQuaternions);
                    } else {
                        data.updateClusterBuffer(meshState.clusterMatrices,
                                                 cauterizedMeshState.clusterMatrices);
                        data.computeAdjustedLocalBound(meshState.clusterMatrices);
                    }

                    data.updateTransformForSkinnedMesh(modelTransform, meshState, useDualQuaternionSkinning);
                    data.updateTransformForCauterizedMesh(modelTransform, cauterizedMeshState, useDualQuaternionSkinning);

                    data.setEnableCauterization(enableCauterization);
                    data.updateKey(renderItemKeyGlobalFlags);
                    data.setShapeKey(invalidatePayloadShapeKey, primitiveMode, useDualQuaternionSkinning);
                });
            }

            scene->enqueueTransaction(transaction);
        });
    } else {
        Model::updateRenderItems();
    }
}

const Model::MeshState& CauterizedModel::getCauterizeMeshState(int index) const {
    assert((size_t)index < _meshStates.size());
    return _cauterizeMeshStates.at(index);
}
