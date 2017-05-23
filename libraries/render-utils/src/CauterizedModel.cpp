//
//  Created by Andrew Meadows 2017.01.17
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "CauterizedModel.h"

#include <PerfStat.h>

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
            state.clusterMatrices.resize(mesh.clusters.size());
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
        if ((int)meshes.size() != _meshStates.size()) {
            qCDebug(renderutils) << "WARNING!!!! Mesh Sizes don't match! We will not segregate mesh groups yet.";
            return;
        }

        // We should not have any existing renderItems if we enter this section of code
        Q_ASSERT(_modelMeshRenderItems.isEmpty());

        _modelMeshRenderItems.clear();

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

    for (int i = 0; i < _meshStates.size(); i++) {
        Model::MeshState& state = _meshStates[i];
        const FBXMesh& mesh = geometry.meshes.at(i);
        for (int j = 0; j < mesh.clusters.size(); j++) {
            const FBXCluster& cluster = mesh.clusters.at(j);
            auto jointMatrix = _rig.getJointTransform(cluster.jointIndex);
            glm_mat4u_mul(jointMatrix, cluster.inverseBindMatrix, state.clusterMatrices[j]);
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

    // as an optimization, don't build cautrizedClusterMatrices if the boneSet is empty.
    if (!_cauterizeBoneSet.empty()) {
        static const glm::mat4 zeroScale(
            glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
            glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
            glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
            glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
        auto cauterizeMatrix = _rig.getJointTransform(geometry.neckJointIndex) * zeroScale;

        for (int i = 0; i < _cauterizeMeshStates.size(); i++) {
            Model::MeshState& state = _cauterizeMeshStates[i];
            const FBXMesh& mesh = geometry.meshes.at(i);
            for (int j = 0; j < mesh.clusters.size(); j++) {
                const FBXCluster& cluster = mesh.clusters.at(j);
                auto jointMatrix = _rig.getJointTransform(cluster.jointIndex);
                if (_cauterizeBoneSet.find(cluster.jointIndex) != _cauterizeBoneSet.end()) {
                    jointMatrix = cauterizeMatrix;
                }
                glm_mat4u_mul(jointMatrix, cluster.inverseBindMatrix, state.clusterMatrices[j]);
            }

            if (!_cauterizeBoneSet.empty() && (state.clusterMatrices.size() > 1)) {
                if (!state.clusterBuffer) {
                    state.clusterBuffer =
                        std::make_shared<gpu::Buffer>(state.clusterMatrices.size() * sizeof(glm::mat4),
                                                      (const gpu::Byte*) state.clusterMatrices.constData());
                } else {
                    state.clusterBuffer->setSubData(0, state.clusterMatrices.size() * sizeof(glm::mat4),
                                                              (const gpu::Byte*) state.clusterMatrices.constData());
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
        std::weak_ptr<Model> weakSelf = shared_from_this();
        AbstractViewStateInterface::instance()->pushPostUpdateLambda(key, [weakSelf, scale]() {
            // do nothing, if the model has already been destroyed.
            auto self = weakSelf.lock();
            if (!self) {
                return;
            }

            // lazy update of cluster matrices used for rendering.  We need to update them here, so we can correctly update the bounding box.
            self->updateClusterMatrices();

            render::ScenePointer scene = AbstractViewStateInterface::instance()->getMain3DScene();

            Transform modelTransform;
            modelTransform.setTranslation(self->getTranslation());
            modelTransform.setRotation(self->getRotation());

            Transform scaledModelTransform(modelTransform);
            scaledModelTransform.setScale(scale);

            uint32_t deleteGeometryCounter = self->getGeometryCounter();

            render::Transaction transaction;
            QList<render::ItemID> keys = self->getRenderItems().keys();
            foreach (auto itemID, keys) {
                transaction.updateItem<CauterizedMeshPartPayload>(itemID, [modelTransform, deleteGeometryCounter](CauterizedMeshPartPayload& data) {
                    ModelPointer model = data._model.lock();
                    if (model && model->isLoaded()) {
                        // Ensure the model geometry was not reset between frames
                        if (deleteGeometryCounter == model->getGeometryCounter()) {
                            // this stuff identical to what happens in regular Model
                            const Model::MeshState& state = model->getMeshState(data._meshIndex);
                            Transform renderTransform = modelTransform;
                            if (state.clusterMatrices.size() == 1) {
                                renderTransform = modelTransform.worldTransform(Transform(state.clusterMatrices[0]));
                            }
                            data.updateTransformForSkinnedMesh(renderTransform, modelTransform, state.clusterBuffer);

                            // this stuff for cauterized mesh
                            CauterizedModel* cModel = static_cast<CauterizedModel*>(model.get());
                            const Model::MeshState& cState = cModel->getCauterizeMeshState(data._meshIndex);
                            renderTransform = modelTransform;
                            if (cState.clusterMatrices.size() == 1) {
                                renderTransform = modelTransform.worldTransform(Transform(cState.clusterMatrices[0]));
                            }
                            data.updateTransformForCauterizedMesh(renderTransform, cState.clusterBuffer);
                        }
                    }
                });
            }

            scene->enqueueTransaction(transaction);
        });
    } else {
        Model::updateRenderItems();
    }
}

const Model::MeshState& CauterizedModel::getCauterizeMeshState(int index) const {
    assert(index < _meshStates.size());
    return _cauterizeMeshStates.at(index);
}
