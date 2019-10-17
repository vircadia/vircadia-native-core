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
        const auto& hfmDynamicTransforms = hfmModel.skinDeformers;
        for (int i = 0; i < hfmDynamicTransforms.size(); i++) {
            const auto& dynT = hfmDynamicTransforms[i];
            MeshState state;
            state.clusterDualQuaternions.resize(dynT.clusters.size());
            state.clusterMatrices.resize(dynT.clusters.size());
            _cauterizeMeshStates.push_back(state);
        }
    }
    return needsFullUpdate;
}

void CauterizedModel::createRenderItemSet() {
    if (_isCauterized) {
        assert(isLoaded());
        const auto& meshes = _renderGeometry->getMeshes();


        // We should not have any existing renderItems if we enter this section of code
        Q_ASSERT(_modelMeshRenderItems.isEmpty());

        _modelMeshRenderItems.clear();
        _modelMeshMaterialNames.clear();
        _modelMeshRenderItemShapes.clear();

        Transform transform;
        transform.setTranslation(_translation);
        transform.setRotation(_rotation);

        Transform offset;
        offset.setScale(_scale);
        offset.postTranslate(_offset);

        Transform::mult(transform, transform, offset);

        // Run through all of the meshes, and place them into their segregated, but unsorted buckets
        int shapeID = 0;
        const auto& shapes = _renderGeometry->getHFMModel().shapes;
        for (shapeID; shapeID < shapes.size(); shapeID++) {
            const auto& shape = shapes[shapeID];

            _modelMeshRenderItems << std::make_shared<CauterizedMeshPartPayload>(shared_from_this(), shape.mesh, shape.meshPart, shapeID, transform);

            auto material = getNetworkModel()->getShapeMaterial(shapeID);
            _modelMeshMaterialNames.push_back(material ? material->getName() : "");
            _modelMeshRenderItemShapes.emplace_back(ShapeInfo{ (int)shape.mesh, shape.skinDeformer });
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

    updateShapeStatesFromRig();

    _needsUpdateClusterMatrices = false;


    const HFMModel& hfmModel = getHFMModel();
    const auto& hfmSkinDeformers = hfmModel.skinDeformers;
    for (int meshIndex = 0; meshIndex < (int)_meshStates.size(); meshIndex++) {
        MeshState& state = _meshStates[meshIndex];
        const auto& deformer = hfmSkinDeformers[meshIndex];

        for (int clusterIndex = 0; clusterIndex < deformer.clusters.size(); clusterIndex++) {
            const auto& cluster = deformer.clusters[clusterIndex];

            const auto& cbmov = _rig.getAnimSkeleton()->getClusterBindMatricesOriginalValues(meshIndex, clusterIndex);

            if (_useDualQuaternionSkinning) {
                auto jointPose = _rig.getJointPose(cluster.jointIndex);
                Transform jointTransform(jointPose.rot(), jointPose.scale(), jointPose.trans());
                Transform clusterTransform;
                Transform::mult(clusterTransform, jointTransform, cbmov.inverseBindTransform);
                state.clusterDualQuaternions[clusterIndex] = Model::TransformDualQuaternion(clusterTransform);
            } else {
                auto jointMatrix = _rig.getJointTransform(cluster.jointIndex);
                glm_mat4u_mul(jointMatrix, cbmov.inverseBindMatrix, state.clusterMatrices[clusterIndex]);
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

        for (int meshIndex = 0; meshIndex < _cauterizeMeshStates.size(); meshIndex++) {
            Model::MeshState& state = _cauterizeMeshStates[meshIndex];
            const auto& deformer = hfmSkinDeformers[meshIndex];

            for (int clusterIndex = 0; clusterIndex < deformer.clusters.size(); clusterIndex++) {
                const auto& cluster = deformer.clusters[clusterIndex];

                const auto& cbmov = _rig.getAnimSkeleton()->getClusterBindMatricesOriginalValues(meshIndex, clusterIndex);

                if (_useDualQuaternionSkinning) {
                    if (_cauterizeBoneSet.find(cluster.jointIndex) == _cauterizeBoneSet.end()) {
                        // not cauterized so just copy the value from the non-cauterized version.
                        state.clusterDualQuaternions[clusterIndex] = _meshStates[meshIndex].clusterDualQuaternions[clusterIndex];
                    } else {
                        Transform jointTransform(cauterizePose.rot(), cauterizePose.scale(), cauterizePose.trans());
                        Transform clusterTransform;
                        Transform::mult(clusterTransform, jointTransform, cbmov.inverseBindTransform);
                        state.clusterDualQuaternions[clusterIndex] = Model::TransformDualQuaternion(clusterTransform);
                        state.clusterDualQuaternions[clusterIndex].setCauterizationParameters(1.0f, cauterizePose.trans());
                    }
                } else {
                    if (_cauterizeBoneSet.find(cluster.jointIndex) == _cauterizeBoneSet.end()) {
                        // not cauterized so just copy the value from the non-cauterized version.
                        state.clusterMatrices[clusterIndex] = _meshStates[meshIndex].clusterMatrices[clusterIndex];
                    } else {
                        glm_mat4u_mul(cauterizeMatrix, cbmov.inverseBindMatrix, state.clusterMatrices[clusterIndex]);
                    }
                }
            }
        }
    }

    // post the blender if we're not currently waiting for one to finish
    auto modelBlender = DependencyManager::get<ModelBlender>();
    if (modelBlender->shouldComputeBlendshapes() && hfmModel.hasBlendedMeshes() && _blendshapeCoefficients != _blendedBlendshapeCoefficients) {
        _blendedBlendshapeCoefficients = _blendshapeCoefficients;
        modelBlender->noteRequiresBlend(getThisPointer());
    }
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

                const auto& shapeState = self->getShapeState(i);

                auto deformerIndex = self->_modelMeshRenderItemShapes[i].deformerIndex;
                bool isDeformed = (deformerIndex != hfm::UNDEFINED_KEY);

                bool invalidatePayloadShapeKey = self->shouldInvalidatePayloadShapeKey(meshIndex);
                bool useDualQuaternionSkinning = self->getUseDualQuaternionSkinning();

                if (isDeformed) { 

                    const auto& meshState = self->getMeshState(deformerIndex);
                    const auto& cauterizedMeshState = self->getCauterizeMeshState(deformerIndex);

                    transaction.updateItem<ModelMeshPartPayload>(itemID,
                        [modelTransform, shapeState, meshState, useDualQuaternionSkinning, cauterizedMeshState, invalidatePayloadShapeKey,
                            primitiveMode, renderItemKeyGlobalFlags, enableCauterization](ModelMeshPartPayload& mmppData) {
                        CauterizedMeshPartPayload& data = static_cast<CauterizedMeshPartPayload&>(mmppData);
                        if (useDualQuaternionSkinning) {
                            data.updateClusterBuffer(meshState.clusterDualQuaternions, cauterizedMeshState.clusterDualQuaternions);
                        } else {
                            data.updateClusterBuffer(meshState.clusterMatrices, cauterizedMeshState.clusterMatrices);
                        }

                        Transform renderTransform = modelTransform;
                       // if (meshState.clusterMatrices.size() <= 2) {
                       //     renderTransform = modelTransform.worldTransform(shapeState._rootFromJointTransform);
                       // }
                        data.updateTransform(renderTransform);
                        data.updateTransformForCauterizedMesh(renderTransform);
                        data.updateTransformAndBound(modelTransform.worldTransform(shapeState._rootFromJointTransform));

                        data.setEnableCauterization(enableCauterization);
                        data.updateKey(renderItemKeyGlobalFlags);
                        data.setShapeKey(invalidatePayloadShapeKey, primitiveMode, useDualQuaternionSkinning);
                    });
                } else {
                    transaction.updateItem<ModelMeshPartPayload>(itemID,
                        [modelTransform, shapeState, invalidatePayloadShapeKey, primitiveMode, renderItemKeyGlobalFlags, enableCauterization]
                             (ModelMeshPartPayload& mmppData) {
                        CauterizedMeshPartPayload& data = static_cast<CauterizedMeshPartPayload&>(mmppData);

                        Transform renderTransform = modelTransform;

                        renderTransform = modelTransform.worldTransform(shapeState._rootFromJointTransform);
                        data.updateTransform(renderTransform);
                        data.updateTransformForCauterizedMesh(renderTransform);

                        data.setEnableCauterization(enableCauterization);
                        data.updateKey(renderItemKeyGlobalFlags);
                        data.setShapeKey(invalidatePayloadShapeKey, primitiveMode, false);
                    });
                    
                }
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
