//
//  MeshPartPayload.cpp
//  interface/src/renderer
//
//  Created by Sam Gateau on 10/3/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "MeshPartPayload.h"

#include <PerfStat.h>
#include <DualQuaternion.h>
#include <graphics/ShaderConstants.h>

#include "render-utils/ShaderConstants.h"
#include "DeferredLightingEffect.h"

#include "RenderPipelines.h"

using namespace render;

namespace render {
template <> const ItemKey payloadGetKey(const MeshPartPayload::Pointer& payload) {
    if (payload) {
        return payload->getKey();
    }
    return ItemKey::Builder::opaqueShape(); // for lack of a better idea
}

template <> const Item::Bound payloadGetBound(const MeshPartPayload::Pointer& payload) {
    if (payload) {
        return payload->getBound();
    }
    return Item::Bound();
}

template <> const ShapeKey shapeGetShapeKey(const MeshPartPayload::Pointer& payload) {
    if (payload) {
        return payload->getShapeKey();
    }
    return ShapeKey::Builder::invalid();
}

template <> void payloadRender(const MeshPartPayload::Pointer& payload, RenderArgs* args) {
    return payload->render(args);
}
}

const graphics::MaterialPointer MeshPartPayload::DEFAULT_MATERIAL = std::make_shared<graphics::Material>();

MeshPartPayload::MeshPartPayload(const std::shared_ptr<const graphics::Mesh>& mesh, int partIndex, graphics::MaterialPointer material) {
    updateMeshPart(mesh, partIndex);
    addMaterial(graphics::MaterialLayer(material, 0));
}

void MeshPartPayload::updateMeshPart(const std::shared_ptr<const graphics::Mesh>& drawMesh, int partIndex) {
    _drawMesh = drawMesh;
    if (_drawMesh) {
        auto vertexFormat = _drawMesh->getVertexFormat();
        _hasColorAttrib = vertexFormat->hasAttribute(gpu::Stream::COLOR);
        _drawPart = _drawMesh->getPartBuffer().get<graphics::Mesh::Part>(partIndex);
        _localBound = _drawMesh->evalPartBound(partIndex);
    }
}

void MeshPartPayload::updateTransform(const Transform& transform, const Transform& offsetTransform) {
    _transform = transform;
    Transform::mult(_drawTransform, _transform, offsetTransform);
    _worldBound = _localBound;
    _worldBound.transform(_drawTransform);
}

void MeshPartPayload::addMaterial(graphics::MaterialLayer material) {
    _drawMaterials.push(material);
}

void MeshPartPayload::removeMaterial(graphics::MaterialPointer material) {
    _drawMaterials.remove(material);
}

void MeshPartPayload::updateKey(const render::ItemKey& key) {
    ItemKey::Builder builder(key);
    builder.withTypeShape();

    if (topMaterialExists()) {
        auto matKey = _drawMaterials.top().material->getKey();
        if (matKey.isTranslucent()) {
            builder.withTransparent();
        }
    }

    _itemKey = builder.build();
}

ItemKey MeshPartPayload::getKey() const {
    return _itemKey;
}

Item::Bound MeshPartPayload::getBound() const {
    return _worldBound;
}

ShapeKey MeshPartPayload::getShapeKey() const {
    graphics::MaterialKey drawMaterialKey;
    if (topMaterialExists()) {
        drawMaterialKey = _drawMaterials.top().material->getKey();
    }

    ShapeKey::Builder builder;
    builder.withMaterial();

    if (drawMaterialKey.isTranslucent()) {
        builder.withTranslucent();
    }
    if (drawMaterialKey.isNormalMap()) {
        builder.withTangents();
    }
    if (drawMaterialKey.isLightmapMap()) {
        builder.withLightmap();
    }
    return builder.build();
}

void MeshPartPayload::drawCall(gpu::Batch& batch) const {
    batch.drawIndexed(gpu::TRIANGLES, _drawPart._numIndices, _drawPart._startIndex);
}

void MeshPartPayload::bindMesh(gpu::Batch& batch) {
    batch.setIndexBuffer(gpu::UINT32, (_drawMesh->getIndexBuffer()._buffer), 0);

    batch.setInputFormat((_drawMesh->getVertexFormat()));

    batch.setInputStream(0, _drawMesh->getVertexStream());
}

 void MeshPartPayload::bindTransform(gpu::Batch& batch, RenderArgs::RenderMode renderMode) const {
    batch.setModelTransform(_drawTransform);
}


void MeshPartPayload::render(RenderArgs* args) {
    PerformanceTimer perfTimer("MeshPartPayload::render");

    if (!args) {
        return;
    }

    gpu::Batch& batch = *(args->_batch);

    // Bind the model transform and the skinCLusterMatrices if needed
    bindTransform(batch, args->_renderMode);

    //Bind the index buffer and vertex buffer and Blend shapes if needed
    bindMesh(batch);

    // apply material properties
    if (args->_renderMode != render::Args::RenderMode::SHADOW_RENDER_MODE) {
        RenderPipelines::bindMaterial(!_drawMaterials.empty() ? _drawMaterials.top().material : DEFAULT_MATERIAL, batch, args->_enableTexturing);
        args->_details._materialSwitches++;
    }

    // Draw!
    {
        PerformanceTimer perfTimer("batch.drawIndexed()");
        drawCall(batch);
    }

    const int INDICES_PER_TRIANGLE = 3;
    args->_details._trianglesRendered += _drawPart._numIndices / INDICES_PER_TRIANGLE;
}

namespace render {
template <> const ItemKey payloadGetKey(const ModelMeshPartPayload::Pointer& payload) {
    if (payload) {
        return payload->getKey();
    }
    return ItemKey::Builder::opaqueShape(); // for lack of a better idea
}

template <> const Item::Bound payloadGetBound(const ModelMeshPartPayload::Pointer& payload) {
    if (payload) {
        return payload->getBound();
    }
    return Item::Bound();
}

template <> const ShapeKey shapeGetShapeKey(const ModelMeshPartPayload::Pointer& payload) {
    if (payload) {
        return payload->getShapeKey();
    }
    return ShapeKey::Builder::invalid();
}

template <> void payloadRender(const ModelMeshPartPayload::Pointer& payload, RenderArgs* args) {
    return payload->render(args);
}

}

ModelMeshPartPayload::ModelMeshPartPayload(ModelPointer model, int meshIndex, int partIndex, int shapeIndex, const Transform& transform, const Transform& offsetTransform) :
    _meshIndex(meshIndex),
    _shapeID(shapeIndex) {

    assert(model && model->isLoaded());

    bool useDualQuaternionSkinning = model->getUseDualQuaternionSkinning();

    auto buffer = model->_blendshapeBuffers.find(meshIndex);
    if (buffer != model->_blendshapeBuffers.end()) {
        _blendshapeBuffer = buffer->second;
    }

    auto& modelMesh = model->getGeometry()->getMeshes().at(_meshIndex);
    const Model::MeshState& state = model->getMeshState(_meshIndex);

    updateMeshPart(modelMesh, partIndex);

    if (useDualQuaternionSkinning) {
        computeAdjustedLocalBound(state.clusterDualQuaternions);
    } else {
        computeAdjustedLocalBound(state.clusterMatrices);
    }

    updateTransform(transform, offsetTransform);
    Transform renderTransform = transform;
    if (useDualQuaternionSkinning) {
        if (state.clusterDualQuaternions.size() == 1) {
            const auto& dq = state.clusterDualQuaternions[0];
            Transform transform(dq.getRotation(),
                                dq.getScale(),
                                dq.getTranslation());
            renderTransform = transform.worldTransform(Transform(transform));
        }
    } else {
        if (state.clusterMatrices.size() == 1) {
            renderTransform = transform.worldTransform(Transform(state.clusterMatrices[0]));
        }
    }
    updateTransformForSkinnedMesh(renderTransform, transform);

    initCache(model);
}

void ModelMeshPartPayload::initCache(const ModelPointer& model) {
    if (_drawMesh) {
        auto vertexFormat = _drawMesh->getVertexFormat();
        _hasColorAttrib = vertexFormat->hasAttribute(gpu::Stream::COLOR);
        _isSkinned = vertexFormat->hasAttribute(gpu::Stream::SKIN_CLUSTER_WEIGHT) && vertexFormat->hasAttribute(gpu::Stream::SKIN_CLUSTER_INDEX);

        const FBXGeometry& geometry = model->getFBXGeometry();
        const FBXMesh& mesh = geometry.meshes.at(_meshIndex);

        _isBlendShaped = !mesh.blendshapes.isEmpty();
        _hasTangents = !mesh.tangents.isEmpty();
    }

    auto networkMaterial = model->getGeometry()->getShapeMaterial(_shapeID);
    if (networkMaterial) {
        addMaterial(graphics::MaterialLayer(networkMaterial, 0));
    }
}

void ModelMeshPartPayload::notifyLocationChanged() {

}

void ModelMeshPartPayload::updateClusterBuffer(const std::vector<glm::mat4>& clusterMatrices) {

    // reset cluster buffer if we change the cluster buffer type
    if (_clusterBufferType != ClusterBufferType::Matrices) {
        _clusterBuffer.reset();
    }
    _clusterBufferType = ClusterBufferType::Matrices;

    // Once computed the cluster matrices, update the buffer(s)
    if (clusterMatrices.size() > 1) {
        if (!_clusterBuffer) {
            _clusterBuffer = std::make_shared<gpu::Buffer>(clusterMatrices.size() * sizeof(glm::mat4),
                (const gpu::Byte*) clusterMatrices.data());
        }
        else {
            _clusterBuffer->setSubData(0, clusterMatrices.size() * sizeof(glm::mat4),
                (const gpu::Byte*) clusterMatrices.data());
        }
    }
}

void ModelMeshPartPayload::updateClusterBuffer(const std::vector<Model::TransformDualQuaternion>& clusterDualQuaternions) {

    // reset cluster buffer if we change the cluster buffer type
    if (_clusterBufferType != ClusterBufferType::DualQuaternions) {
        _clusterBuffer.reset();
    }
    _clusterBufferType = ClusterBufferType::DualQuaternions;

    // Once computed the cluster matrices, update the buffer(s)
    if (clusterDualQuaternions.size() > 1) {
        if (!_clusterBuffer) {
            _clusterBuffer = std::make_shared<gpu::Buffer>(clusterDualQuaternions.size() * sizeof(Model::TransformDualQuaternion),
                (const gpu::Byte*) clusterDualQuaternions.data());
        }
        else {
            _clusterBuffer->setSubData(0, clusterDualQuaternions.size() * sizeof(Model::TransformDualQuaternion),
                (const gpu::Byte*) clusterDualQuaternions.data());
        }
    }
}

void ModelMeshPartPayload::updateTransformForSkinnedMesh(const Transform& renderTransform, const Transform& boundTransform) {
    _transform = renderTransform;
    _worldBound = _adjustedLocalBound;
    _worldBound.transform(boundTransform);
}

// Note that this method is called for models but not for shapes
void ModelMeshPartPayload::updateKey(const render::ItemKey& key) {
    ItemKey::Builder builder(key);
    builder.withTypeShape();

    if (_isBlendShaped || _isSkinned) {
        builder.withDeformed();
    }

    if (topMaterialExists()) {
        auto matKey = _drawMaterials.top().material->getKey();
        if (matKey.isTranslucent()) {
            builder.withTransparent();
        }
    }

    _itemKey = builder.build();
}

void ModelMeshPartPayload::setShapeKey(bool invalidateShapeKey, bool isWireframe, bool useDualQuaternionSkinning) {
    if (invalidateShapeKey) {
        _shapeKey = ShapeKey::Builder::invalid();
        return;
    }

    graphics::MaterialKey drawMaterialKey;
    if (topMaterialExists()) {
        drawMaterialKey = _drawMaterials.top().material->getKey();
    }

    bool isTranslucent = drawMaterialKey.isTranslucent();
    bool hasTangents = drawMaterialKey.isNormalMap() && _hasTangents;
    bool hasLightmap = drawMaterialKey.isLightmapMap();
    bool isUnlit = drawMaterialKey.isUnlit();

    bool isSkinned = _isSkinned;

    if (isWireframe) {
        isTranslucent = hasTangents = hasLightmap = isSkinned = false;
    }

    ShapeKey::Builder builder;
    builder.withMaterial();

    if (isTranslucent) {
        builder.withTranslucent();
    }
    if (hasTangents) {
        builder.withTangents();
    }
    if (hasLightmap) {
        builder.withLightmap();
    }
    if (isUnlit) {
        builder.withUnlit();
    }
    if (isSkinned) {
        builder.withSkinned();
    }
    if (isWireframe) {
        builder.withWireframe();
    }
    if (isSkinned && useDualQuaternionSkinning) {
        builder.withDualQuatSkinned();
    }

    _shapeKey = builder.build();
}

ShapeKey ModelMeshPartPayload::getShapeKey() const {
    return _shapeKey;
}

void ModelMeshPartPayload::bindMesh(gpu::Batch& batch) {
    batch.setIndexBuffer(gpu::UINT32, (_drawMesh->getIndexBuffer()._buffer), 0);
    batch.setInputFormat((_drawMesh->getVertexFormat()));
    if (_blendshapeBuffer) {
        batch.setResourceBuffer(0, _blendshapeBuffer);
    }
    batch.setInputStream(0, _drawMesh->getVertexStream());
}

void ModelMeshPartPayload::bindTransform(gpu::Batch& batch, RenderArgs::RenderMode renderMode) const {
    if (_clusterBuffer) {
        batch.setUniformBuffer(graphics::slot::buffer::Skinning, _clusterBuffer);
    }
    batch.setModelTransform(_transform);
}

void ModelMeshPartPayload::render(RenderArgs* args) {
    PerformanceTimer perfTimer("ModelMeshPartPayload::render");

    if (!args) {
        return;
    }

    gpu::Batch& batch = *(args->_batch);

    bindTransform(batch, args->_renderMode);

    //Bind the index buffer and vertex buffer and Blend shapes if needed
    bindMesh(batch);

    // apply material properties
    if (args->_renderMode != render::Args::RenderMode::SHADOW_RENDER_MODE) {
        RenderPipelines::bindMaterial(!_drawMaterials.empty() ? _drawMaterials.top().material : DEFAULT_MATERIAL, batch, args->_enableTexturing);
        args->_details._materialSwitches++;
    }

    // Draw!
    {
        PerformanceTimer perfTimer("batch.drawIndexed()");
        drawCall(batch);
    }

    const int INDICES_PER_TRIANGLE = 3;
    args->_details._trianglesRendered += _drawPart._numIndices / INDICES_PER_TRIANGLE;
}

void ModelMeshPartPayload::computeAdjustedLocalBound(const std::vector<glm::mat4>& clusterMatrices) {
    _adjustedLocalBound = _localBound;
    if (clusterMatrices.size() > 0) {
        _adjustedLocalBound.transform(clusterMatrices[0]);

        for (int i = 1; i < (int)clusterMatrices.size(); ++i) {
            AABox clusterBound = _localBound;
            clusterBound.transform(clusterMatrices[i]);
            _adjustedLocalBound += clusterBound;
        }
    }
}

void ModelMeshPartPayload::computeAdjustedLocalBound(const std::vector<Model::TransformDualQuaternion>& clusterDualQuaternions) {
    _adjustedLocalBound = _localBound;
    if (clusterDualQuaternions.size() > 0) {
        Transform rootTransform(clusterDualQuaternions[0].getRotation(),
                                clusterDualQuaternions[0].getScale(),
                                clusterDualQuaternions[0].getTranslation());
        _adjustedLocalBound.transform(rootTransform);

        for (int i = 1; i < (int)clusterDualQuaternions.size(); ++i) {
            AABox clusterBound = _localBound;
            Transform transform(clusterDualQuaternions[i].getRotation(),
                                clusterDualQuaternions[i].getScale(),
                                clusterDualQuaternions[i].getTranslation());
            clusterBound.transform(transform);
            _adjustedLocalBound += clusterBound;
        }
    }
}
