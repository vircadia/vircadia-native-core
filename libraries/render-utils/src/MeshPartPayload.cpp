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

#include "DeferredLightingEffect.h"
#include "EntityItem.h"

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

MeshPartPayload::MeshPartPayload(const std::shared_ptr<const model::Mesh>& mesh, int partIndex, model::MaterialPointer material) {
    updateMeshPart(mesh, partIndex);
    updateMaterial(material);
}

void MeshPartPayload::updateMeshPart(const std::shared_ptr<const model::Mesh>& drawMesh, int partIndex) {
    _drawMesh = drawMesh;
    if (_drawMesh) {
        auto vertexFormat = _drawMesh->getVertexFormat();
        _hasColorAttrib = vertexFormat->hasAttribute(gpu::Stream::COLOR);
        _drawPart = _drawMesh->getPartBuffer().get<model::Mesh::Part>(partIndex);
        _localBound = _drawMesh->evalPartBound(partIndex);
    }
}

void MeshPartPayload::updateTransform(const Transform& transform, const Transform& offsetTransform) {
    _transform = transform;
    Transform::mult(_drawTransform, _transform, offsetTransform);
    _worldBound = _localBound;
    _worldBound.transform(_drawTransform);
}

void MeshPartPayload::updateMaterial(model::MaterialPointer drawMaterial) {
    _drawMaterial = drawMaterial;
}

ItemKey MeshPartPayload::getKey() const {
    ItemKey::Builder builder;
    builder.withTypeShape();

    if (_drawMaterial) {
        auto matKey = _drawMaterial->getKey();
        if (matKey.isTranslucent()) {
            builder.withTransparent();
        }
    }

    return builder.build();
}

Item::Bound MeshPartPayload::getBound() const {
    return _worldBound;
}

ShapeKey MeshPartPayload::getShapeKey() const {
    model::MaterialKey drawMaterialKey;
    if (_drawMaterial) {
        drawMaterialKey = _drawMaterial->getKey();
    }

    ShapeKey::Builder builder;
    builder.withMaterial();

    if (drawMaterialKey.isTranslucent()) {
        builder.withTranslucent();
    }
    if (drawMaterialKey.isNormalMap()) {
        builder.withTangents();
    }
    if (drawMaterialKey.isMetallicMap()) {
        builder.withSpecular();
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

    // TODO: Get rid of that extra call
    if (!_hasColorAttrib) {
        batch._glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    }
}

void MeshPartPayload::bindMaterial(gpu::Batch& batch, const ShapePipeline::LocationsPointer locations, bool enableTextures) const {
    if (!_drawMaterial) {
        return;
    }

    auto textureCache = DependencyManager::get<TextureCache>();

    batch.setUniformBuffer(ShapePipeline::Slot::BUFFER::MATERIAL, _drawMaterial->getSchemaBuffer());
    batch.setUniformBuffer(ShapePipeline::Slot::BUFFER::TEXMAPARRAY, _drawMaterial->getTexMapArrayBuffer());

    const auto& materialKey = _drawMaterial->getKey();
    const auto& textureMaps = _drawMaterial->getTextureMaps();

    int numUnlit = 0;
    if (materialKey.isUnlit()) {
        numUnlit++;
    }

    if (!enableTextures) {
        batch.setResourceTexture(ShapePipeline::Slot::ALBEDO, textureCache->getWhiteTexture());
        batch.setResourceTexture(ShapePipeline::Slot::MAP::ROUGHNESS, textureCache->getWhiteTexture());
        batch.setResourceTexture(ShapePipeline::Slot::MAP::NORMAL, textureCache->getBlueTexture());
        batch.setResourceTexture(ShapePipeline::Slot::MAP::METALLIC, textureCache->getBlackTexture());
        batch.setResourceTexture(ShapePipeline::Slot::MAP::OCCLUSION, textureCache->getWhiteTexture());
        batch.setResourceTexture(ShapePipeline::Slot::MAP::SCATTERING, textureCache->getWhiteTexture());
        batch.setResourceTexture(ShapePipeline::Slot::MAP::EMISSIVE_LIGHTMAP, textureCache->getBlackTexture());
        return;
    }

    // Albedo
    if (materialKey.isAlbedoMap()) {
        auto itr = textureMaps.find(model::MaterialKey::ALBEDO_MAP);
        if (itr != textureMaps.end() && itr->second->isDefined()) {
            batch.setResourceTexture(ShapePipeline::Slot::ALBEDO, itr->second->getTextureView());
        } else {
            batch.setResourceTexture(ShapePipeline::Slot::ALBEDO, textureCache->getGrayTexture());
        }
    }

    // Roughness map
    if (materialKey.isRoughnessMap()) {
        auto itr = textureMaps.find(model::MaterialKey::ROUGHNESS_MAP);
        if (itr != textureMaps.end() && itr->second->isDefined()) {
            batch.setResourceTexture(ShapePipeline::Slot::MAP::ROUGHNESS, itr->second->getTextureView());

            // texcoord are assumed to be the same has albedo
        } else {
            batch.setResourceTexture(ShapePipeline::Slot::MAP::ROUGHNESS, textureCache->getWhiteTexture());
        }
    }

    // Normal map
    if (materialKey.isNormalMap()) {
        auto itr = textureMaps.find(model::MaterialKey::NORMAL_MAP);
        if (itr != textureMaps.end() && itr->second->isDefined()) {
            batch.setResourceTexture(ShapePipeline::Slot::MAP::NORMAL, itr->second->getTextureView());

            // texcoord are assumed to be the same has albedo
        } else {
            batch.setResourceTexture(ShapePipeline::Slot::MAP::NORMAL, textureCache->getBlueTexture());
        }
    }

    // Metallic map
    if (materialKey.isMetallicMap()) {
        auto itr = textureMaps.find(model::MaterialKey::METALLIC_MAP);
        if (itr != textureMaps.end() && itr->second->isDefined()) {
            batch.setResourceTexture(ShapePipeline::Slot::MAP::METALLIC, itr->second->getTextureView());

            // texcoord are assumed to be the same has albedo
        } else {
            batch.setResourceTexture(ShapePipeline::Slot::MAP::METALLIC, textureCache->getBlackTexture());
        }
    }

    // Occlusion map
    if (materialKey.isOcclusionMap()) {
        auto itr = textureMaps.find(model::MaterialKey::OCCLUSION_MAP);
        if (itr != textureMaps.end() && itr->second->isDefined()) {
            batch.setResourceTexture(ShapePipeline::Slot::MAP::OCCLUSION, itr->second->getTextureView());

            // texcoord are assumed to be the same has albedo
        } else {
            batch.setResourceTexture(ShapePipeline::Slot::MAP::OCCLUSION, textureCache->getWhiteTexture());
        }
    }

    // Scattering map
    if (materialKey.isScatteringMap()) {
        auto itr = textureMaps.find(model::MaterialKey::SCATTERING_MAP);
        if (itr != textureMaps.end() && itr->second->isDefined()) {
            batch.setResourceTexture(ShapePipeline::Slot::MAP::SCATTERING, itr->second->getTextureView());

            // texcoord are assumed to be the same has albedo
        } else {
            batch.setResourceTexture(ShapePipeline::Slot::MAP::SCATTERING, textureCache->getWhiteTexture());
        }
    }

    // Emissive / Lightmap
    if (materialKey.isLightmapMap()) {
        auto itr = textureMaps.find(model::MaterialKey::LIGHTMAP_MAP);

        if (itr != textureMaps.end() && itr->second->isDefined()) {
            batch.setResourceTexture(ShapePipeline::Slot::MAP::EMISSIVE_LIGHTMAP, itr->second->getTextureView());
        } else {
            batch.setResourceTexture(ShapePipeline::Slot::MAP::EMISSIVE_LIGHTMAP, textureCache->getGrayTexture());
        }
    } else if (materialKey.isEmissiveMap()) {
        auto itr = textureMaps.find(model::MaterialKey::EMISSIVE_MAP);

        if (itr != textureMaps.end() && itr->second->isDefined()) {
            batch.setResourceTexture(ShapePipeline::Slot::MAP::EMISSIVE_LIGHTMAP, itr->second->getTextureView());
        } else {
            batch.setResourceTexture(ShapePipeline::Slot::MAP::EMISSIVE_LIGHTMAP, textureCache->getBlackTexture());
        }
    }
}

void MeshPartPayload::bindTransform(gpu::Batch& batch, const ShapePipeline::LocationsPointer locations, RenderArgs::RenderMode renderMode) const {
    batch.setModelTransform(_drawTransform);
}


void MeshPartPayload::render(RenderArgs* args) {
    PerformanceTimer perfTimer("MeshPartPayload::render");

    gpu::Batch& batch = *(args->_batch);

    auto locations = args->_shapePipeline->locations;
    assert(locations);

    // Bind the model transform and the skinCLusterMatrices if needed
    bindTransform(batch, locations, args->_renderMode);

    //Bind the index buffer and vertex buffer and Blend shapes if needed
    bindMesh(batch);

    // apply material properties
    bindMaterial(batch, locations, args->_enableTexturing);

    if (args) {
        args->_details._materialSwitches++;
    }

    // Draw!
    {
        PerformanceTimer perfTimer("batch.drawIndexed()");
        drawCall(batch);
    }

    if (args) {
        const int INDICES_PER_TRIANGLE = 3;
        args->_details._trianglesRendered += _drawPart._numIndices / INDICES_PER_TRIANGLE;
    }
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
template <> int payloadGetLayer(const ModelMeshPartPayload::Pointer& payload) {
    if (payload) {
        return payload->getLayer();
    }
    return 0;
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

ModelMeshPartPayload::ModelMeshPartPayload(ModelPointer model, int _meshIndex, int partIndex, int shapeIndex, const Transform& transform, const Transform& offsetTransform) :
    _meshIndex(_meshIndex),
    _shapeID(shapeIndex) {

    assert(model && model->isLoaded());
    _model = model;
    auto& modelMesh = model->getGeometry()->getMeshes().at(_meshIndex);

    updateMeshPart(modelMesh, partIndex);

    updateTransform(transform, offsetTransform);
    initCache();
}

void ModelMeshPartPayload::initCache() {
    ModelPointer model = _model.lock();
    assert(model && model->isLoaded());

    if (_drawMesh) {
        auto vertexFormat = _drawMesh->getVertexFormat();
        _hasColorAttrib = vertexFormat->hasAttribute(gpu::Stream::COLOR);
        _isSkinned = vertexFormat->hasAttribute(gpu::Stream::SKIN_CLUSTER_WEIGHT) && vertexFormat->hasAttribute(gpu::Stream::SKIN_CLUSTER_INDEX);

        const FBXGeometry& geometry = model->getFBXGeometry();
        const FBXMesh& mesh = geometry.meshes.at(_meshIndex);

        _isBlendShaped = !mesh.blendshapes.isEmpty();
    }

    auto networkMaterial = model->getGeometry()->getShapeMaterial(_shapeID);
    if (networkMaterial) {
        _drawMaterial = networkMaterial;
    }
}

void ModelMeshPartPayload::notifyLocationChanged() {

}

void ModelMeshPartPayload::updateTransformForSkinnedMesh(const Transform& renderTransform, const Transform& boundTransform,
        const gpu::BufferPointer& buffer) {
    _transform = renderTransform;
    _worldBound = _adjustedLocalBound;
    _worldBound.transform(boundTransform);
    _clusterBuffer = buffer;
}

ItemKey ModelMeshPartPayload::getKey() const {
    ItemKey::Builder builder;
    builder.withTypeShape();

    ModelPointer model = _model.lock();
    if (model) {
        if (!model->isVisible()) {
            builder.withInvisible();
        }

        if (model->isLayeredInFront()) {
            builder.withLayered();
        }

        if (_isBlendShaped || _isSkinned) {
            builder.withDeformed();
        }

        if (_drawMaterial) {
            auto matKey = _drawMaterial->getKey();
            if (matKey.isTranslucent()) {
                builder.withTransparent();
            }
        }
    }
    return builder.build();
}

int ModelMeshPartPayload::getLayer() const {
    // MAgic number while we are defining the layering mechanism:
    const int LAYER_3D_FRONT = 1;
    const int LAYER_3D = 0;
    ModelPointer model = _model.lock();
    if (model && model->isLayeredInFront()) {
        return LAYER_3D_FRONT;
    } else {
        return LAYER_3D;
    }
}

ShapeKey ModelMeshPartPayload::getShapeKey() const {

    // guard against partially loaded meshes
    ModelPointer model = _model.lock();
    if (!model || !model->isLoaded() || !model->getGeometry()) {
        return ShapeKey::Builder::invalid();
    }

    const FBXGeometry& geometry = model->getFBXGeometry();
    const auto& networkMeshes = model->getGeometry()->getMeshes();

    // guard against partially loaded meshes
    if (_meshIndex >= (int)networkMeshes.size() || _meshIndex >= (int)geometry.meshes.size() || _meshIndex >= (int)model->_meshStates.size()) {
        return ShapeKey::Builder::invalid();
    }

    const FBXMesh& mesh = geometry.meshes.at(_meshIndex);

    // if our index is ever out of range for either meshes or networkMeshes, then skip it, and set our _meshGroupsKnown
    // to false to rebuild out mesh groups.
    if (_meshIndex < 0 || _meshIndex >= (int)networkMeshes.size() || _meshIndex > geometry.meshes.size()) {
        model->_needsFixupInScene = true; // trigger remove/add cycle
        model->invalidCalculatedMeshBoxes(); // if we have to reload, we need to assume our mesh boxes are all invalid
        return ShapeKey::Builder::invalid();
    }


    int vertexCount = mesh.vertices.size();
    if (vertexCount == 0) {
        // sanity check
        return ShapeKey::Builder::invalid(); // FIXME
    }


    model::MaterialKey drawMaterialKey;
    if (_drawMaterial) {
        drawMaterialKey = _drawMaterial->getKey();
    }

    bool isTranslucent = drawMaterialKey.isTranslucent();
    bool hasTangents = drawMaterialKey.isNormalMap() && !mesh.tangents.isEmpty();
    bool hasSpecular = drawMaterialKey.isMetallicMap();
    bool hasLightmap = drawMaterialKey.isLightmapMap();
    bool isUnlit = drawMaterialKey.isUnlit();

    bool isSkinned = _isSkinned;
    bool wireframe = model->isWireframe();

    if (wireframe) {
        isTranslucent = hasTangents = hasSpecular = hasLightmap = isSkinned = false;
    }

    ShapeKey::Builder builder;
    builder.withMaterial();

    if (isTranslucent) {
        builder.withTranslucent();
    }
    if (hasTangents) {
        builder.withTangents();
    }
    if (hasSpecular) {
        builder.withSpecular();
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
    if (wireframe) {
        builder.withWireframe();
    }
    return builder.build();
}

void ModelMeshPartPayload::bindMesh(gpu::Batch& batch) {
    if (!_isBlendShaped) {
        batch.setIndexBuffer(gpu::UINT32, (_drawMesh->getIndexBuffer()._buffer), 0);
        batch.setInputFormat((_drawMesh->getVertexFormat()));
        batch.setInputStream(0, _drawMesh->getVertexStream());
    } else {
        batch.setIndexBuffer(gpu::UINT32, (_drawMesh->getIndexBuffer()._buffer), 0);
        batch.setInputFormat((_drawMesh->getVertexFormat()));

        ModelPointer model = _model.lock();
        if (model) {
            batch.setInputBuffer(0, model->_blendedVertexBuffers[_meshIndex], 0, sizeof(glm::vec3));
            batch.setInputBuffer(1, model->_blendedVertexBuffers[_meshIndex], _drawMesh->getNumVertices() * sizeof(glm::vec3), sizeof(glm::vec3));
            batch.setInputStream(2, _drawMesh->getVertexStream().makeRangedStream(2));
        } else {
            batch.setIndexBuffer(gpu::UINT32, (_drawMesh->getIndexBuffer()._buffer), 0);
            batch.setInputFormat((_drawMesh->getVertexFormat()));
            batch.setInputStream(0, _drawMesh->getVertexStream());
        }
    }

    // TODO: Get rid of that extra call
    if (!_hasColorAttrib) {
        batch._glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    }
}

void ModelMeshPartPayload::bindTransform(gpu::Batch& batch, const ShapePipeline::LocationsPointer locations, RenderArgs::RenderMode renderMode) const {
    // Still relying on the raw data from the model
    if (_clusterBuffer) {
        batch.setUniformBuffer(ShapePipeline::Slot::BUFFER::SKINNING, _clusterBuffer);
    }
    batch.setModelTransform(_transform);
}

void ModelMeshPartPayload::render(RenderArgs* args) {
    PerformanceTimer perfTimer("ModelMeshPartPayload::render");

    ModelPointer model = _model.lock();
    if (!model || !model->isAddedToScene() || !model->isVisible()) {
        return; // bail asap
    }

    if (_state == WAITING_TO_START) {
        if (model->isLoaded()) {
            _state = STARTED;
            model->setRenderItemsNeedUpdate();
        } else {
            return;
        }
    }

    if (_materialNeedsUpdate && model->getGeometry()->areTexturesLoaded()) {
        model->setRenderItemsNeedUpdate();
        _materialNeedsUpdate = false;
    }

    if (!args) {
        return;
    }
    if (!getShapeKey().isValid()) {
        return;
    }

    gpu::Batch& batch = *(args->_batch);
    auto locations =  args->_shapePipeline->locations;
    assert(locations);

    bindTransform(batch, locations, args->_renderMode);

    //Bind the index buffer and vertex buffer and Blend shapes if needed
    bindMesh(batch);

    // apply material properties
    bindMaterial(batch, locations, args->_enableTexturing);

    args->_details._materialSwitches++;

    // Draw!
    {
        PerformanceTimer perfTimer("batch.drawIndexed()");
        drawCall(batch);
    }

    const int INDICES_PER_TRIANGLE = 3;
    args->_details._trianglesRendered += _drawPart._numIndices / INDICES_PER_TRIANGLE;
}

void ModelMeshPartPayload::computeAdjustedLocalBound(const QVector<glm::mat4>& clusterMatrices) {
    _adjustedLocalBound = _localBound;
    if (clusterMatrices.size() > 0) {
        _adjustedLocalBound.transform(clusterMatrices[0]);
        for (int i = 1; i < clusterMatrices.size(); ++i) {
            AABox clusterBound = _localBound;
            clusterBound.transform(clusterMatrices[i]);
            _adjustedLocalBound += clusterBound;
        }
    }
}
