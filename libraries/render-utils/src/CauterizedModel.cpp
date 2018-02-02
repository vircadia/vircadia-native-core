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
        const FBXGeometry& fbxGeometry = getFBXGeometry();
        foreach (const FBXMesh& mesh, fbxGeometry.meshes) {
            Model::MeshState state;
            state.clusterTransforms.resize(mesh.clusters.size());
            _cauterizeMeshStates.append(state);
        }
    }
    return needsFullUpdate;
}

void CauterizedModel::createVisibleRenderItemSet() {
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
        _modelMeshRenderItemShapes.clear();

        Transform transform;
        transform.setTranslation(_translation);
        transform.setRotation(_rotation);

        Transform offset;
        offset.setScale(_scale);
        offset.postTranslate(_offset);

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
                auto ptr = std::make_shared<CauterizedMeshPartPayload>(shared_from_this(), i, partIndex, shapeID, transform, offset);
                _modelMeshRenderItems << std::static_pointer_cast<ModelMeshPartPayload>(ptr);
                _modelMeshRenderItemShapes.emplace_back(ShapeInfo{ (int)i });
                shapeID++;
            }
        }
    } else {
        Model::createVisibleRenderItemSet();
    }
}

void CauterizedModel::createCollisionRenderItemSet() {
    // Temporary HACK: use base class method for now
    Model::createCollisionRenderItemSet();
}

void CauterizedModel::updateClusterMatrices() {
    PerformanceTimer perfTimer("CauterizedModel::updateClusterMatrices");

    if (!_needsUpdateClusterMatrices || !isLoaded()) {
        return;
    }
    _needsUpdateClusterMatrices = false;
    const FBXGeometry& geometry = getFBXGeometry();

    for (int i = 0; i < (int)_meshStates.size(); i++) {
        Model::MeshState& state = _meshStates[i];
        const FBXMesh& mesh = geometry.meshes.at(i);
        for (int j = 0; j < mesh.clusters.size(); j++) {
            const FBXCluster& cluster = mesh.clusters.at(j);
#if defined(SKIN_DQ)
            auto jointPose = _rig.getJointPose(cluster.jointIndex);
            Transform jointTransform(jointPose.rot(), jointPose.scale(), jointPose.trans());
            Transform clusterTransform;
            Transform::mult(clusterTransform, jointTransform, cluster.inverseBindTransform);
            state.clusterTransforms[j] = Model::TransformDualQuaternion(clusterTransform);
            state.clusterTransforms[j].setCauterizationParameters(0.0f, jointPose.trans());
#else
            auto jointMatrix = _rig.getJointTransform(cluster.jointIndex);
            glm_mat4u_mul(jointMatrix, cluster.inverseBindMatrix, state.clusterTransforms[j]);
#endif
        }
    }

    // as an optimization, don't build cautrizedClusterMatrices if the boneSet is empty.
    if (!_cauterizeBoneSet.empty()) {
#if defined(SKIN_DQ)
        AnimPose cauterizePose = _rig.getJointPose(geometry.neckJointIndex);
        cauterizePose.scale() = glm::vec3(0.0001f, 0.0001f, 0.0001f);
#else
        static const glm::mat4 zeroScale(
            glm::vec4(0.0001f, 0.0f, 0.0f, 0.0f),
            glm::vec4(0.0f, 0.0001f, 0.0f, 0.0f),
            glm::vec4(0.0f, 0.0f, 0.0001f, 0.0f),
            glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
        auto cauterizeMatrix = _rig.getJointTransform(geometry.neckJointIndex) * zeroScale;
#endif
        for (int i = 0; i < _cauterizeMeshStates.size(); i++) {
            Model::MeshState& state = _cauterizeMeshStates[i];
            const FBXMesh& mesh = geometry.meshes.at(i);

            for (int j = 0; j < mesh.clusters.size(); j++) {
                const FBXCluster& cluster = mesh.clusters.at(j);

                if (_cauterizeBoneSet.find(cluster.jointIndex) == _cauterizeBoneSet.end()) {
                    // not cauterized so just copy the value from the non-cauterized version.
                    state.clusterTransforms[j] = _meshStates[i].clusterTransforms[j];
                } else {
#if defined(SKIN_DQ)
                    Transform jointTransform(cauterizePose.rot(), cauterizePose.scale(), cauterizePose.trans());
                    Transform clusterTransform;
                    Transform::mult(clusterTransform, jointTransform, cluster.inverseBindTransform);
                    state.clusterTransforms[j] = Model::TransformDualQuaternion(clusterTransform);
                    state.clusterTransforms[j].setCauterizationParameters(1.0f, cauterizePose.trans());
#else
                    glm_mat4u_mul(cauterizeMatrix, cluster.inverseBindMatrix, state.clusterTransforms[j]);
#endif
                }
            }
        }
    }

    // post the blender if we're not currently waiting for one to finish
    if (geometry.hasBlendedMeshes() && _blendshapeCoefficients != _blendedBlendshapeCoefficients) {
        _blendedBlendshapeCoefficients = _blendshapeCoefficients;
        DependencyManager::get<ModelBlender>()->noteRequiresBlend(getThisPointer());
    }
}

void CauterizedModel::updateRenderItems() {
    if (_isCauterized) {
        if (!_addedToScene) {
            return;
        }

        glm::vec3 scale = getScale();
        if (_collisionGeometry) {
            // _collisionGeometry is already scaled
            scale = glm::vec3(1.0f);
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

            bool isWireframe = self->isWireframe();
            bool isVisible = self->isVisible();
            bool isLayeredInFront = self->isLayeredInFront();
            bool isLayeredInHUD = self->isLayeredInHUD();
            bool enableCauterization = self->getEnableCauterization();

            render::Transaction transaction;
            for (int i = 0; i < (int)self->_modelMeshRenderItemIDs.size(); i++) {

                auto itemID = self->_modelMeshRenderItemIDs[i];
                auto meshIndex = self->_modelMeshRenderItemShapes[i].meshIndex;
                auto clusterTransforms(self->getMeshState(meshIndex).clusterTransforms);
                auto clusterTransformsCauterized(self->getCauterizeMeshState(meshIndex).clusterTransforms);

                bool invalidatePayloadShapeKey = self->shouldInvalidatePayloadShapeKey(meshIndex);

                transaction.updateItem<CauterizedMeshPartPayload>(itemID, [modelTransform, clusterTransforms, clusterTransformsCauterized, invalidatePayloadShapeKey,
                        isWireframe, isVisible, isLayeredInFront, isLayeredInHUD, enableCauterization](CauterizedMeshPartPayload& data) {
                    data.updateClusterBuffer(clusterTransforms, clusterTransformsCauterized);

                    Transform renderTransform = modelTransform;
                    if (clusterTransforms.size() == 1) {
#if defined(SKIN_DQ)
                        Transform transform(clusterTransforms[0].getRotation(),
                                            clusterTransforms[0].getScale(),
                                            clusterTransforms[0].getTranslation());
                        renderTransform = modelTransform.worldTransform(transform);
#else
                        renderTransform = modelTransform.worldTransform(Transform(clusterTransforms[0]));
#endif
                    }
                    data.updateTransformForSkinnedMesh(renderTransform, modelTransform);

                    renderTransform = modelTransform;
                    if (clusterTransformsCauterized.size() == 1) {
#if defined(SKIN_DQ)
                        Transform transform(clusterTransformsCauterized[0].getRotation(),
                                            clusterTransformsCauterized[0].getScale(),
                                            clusterTransformsCauterized[0].getTranslation());
                        renderTransform = modelTransform.worldTransform(Transform(transform));
#else
                        renderTransform = modelTransform.worldTransform(Transform(clusterTransformsCauterized[0]));
#endif
                    }
                    data.updateTransformForCauterizedMesh(renderTransform);

                    data.setEnableCauterization(enableCauterization);
                    data.updateKey(isVisible, isLayeredInFront || isLayeredInHUD, render::ItemKey::TAG_BITS_ALL);
                    data.setLayer(isLayeredInFront, isLayeredInHUD);
                    data.setShapeKey(invalidatePayloadShapeKey, isWireframe);
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
