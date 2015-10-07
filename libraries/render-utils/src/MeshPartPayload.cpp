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

#include "Model.h"

namespace render {
    template <> const ItemKey payloadGetKey(const MeshPartPayload::Pointer& payload) {
        if (payload) {
            return payload->getKey();
        }
        // Return opaque for lack of a better idea
        return ItemKey::Builder::opaqueShape();
    }
    
    template <> const Item::Bound payloadGetBound(const MeshPartPayload::Pointer& payload) {
        if (payload) {
            return payload->getBound();
        }
        return render::Item::Bound();
    }
    template <> void payloadRender(const MeshPartPayload::Pointer& payload, RenderArgs* args) {
        return payload->render(args);
    }
}

using namespace render;

MeshPartPayload::MeshPartPayload(Model* model, int meshIndex, int partIndex, int shapeIndex) :
    model(model), meshIndex(meshIndex), partIndex(partIndex), _shapeID(shapeIndex)
{
    initCache();
}

void MeshPartPayload::initCache() {
    const std::vector<std::unique_ptr<NetworkMesh>>& networkMeshes = model->_geometry->getMeshes();
    const NetworkMesh& networkMesh = *(networkMeshes.at(meshIndex).get());
    _drawMesh = networkMesh._mesh;

    const FBXGeometry& geometry = model->_geometry->getFBXGeometry();
    const FBXMesh& mesh = geometry.meshes.at(meshIndex);
    _hasColorAttrib = !mesh.colors.isEmpty();
    _isBlendShaped = !mesh.blendshapes.isEmpty();
    _isSkinned = !mesh.clusterIndices.isEmpty();


    _drawPart = _drawMesh->getPartBuffer().get<model::Mesh::Part>(partIndex);

    auto networkMaterial = model->_geometry->getShapeMaterial(_shapeID);
    if (networkMaterial) {
        _drawMaterial = networkMaterial->_material;
    };

}

render::ItemKey MeshPartPayload::getKey() const {
    ItemKey::Builder builder;
    builder.withTypeShape();

    if (!model->isVisible()) {
        builder.withInvisible();
    }

    if (_isBlendShaped || _isSkinned) {
        builder.withDeformed();
    }

    if (_drawMaterial) {
        auto matKey = _drawMaterial->getKey();
        if (matKey.isTransparent() || matKey.isTransparentMap()) {
            builder.withTransparent();
        }
    }

    return builder.build();
}

render::Item::Bound MeshPartPayload::getBound() const {
    if (_isBoundInvalid) {
        model->getPartBounds(meshIndex, partIndex);
        _isBoundInvalid = false;
    }
    return _bound;
}

void MeshPartPayload::drawCall(gpu::Batch& batch) const {
    batch.drawIndexed(gpu::TRIANGLES, _drawPart._numIndices, _drawPart._startIndex);
}

void MeshPartPayload::bindMesh(gpu::Batch& batch) const {
    if (!_isBlendShaped) {
        batch.setIndexBuffer(gpu::UINT32, (_drawMesh->getIndexBuffer()._buffer), 0);
        
        batch.setInputFormat((_drawMesh->getVertexFormat()));
        
        batch.setInputStream(0, _drawMesh->getVertexStream());
    } else {
        batch.setIndexBuffer(gpu::UINT32, (_drawMesh->getIndexBuffer()._buffer), 0);

        batch.setInputFormat((_drawMesh->getVertexFormat()));

        batch.setInputBuffer(0, model->_blendedVertexBuffers[meshIndex], 0, sizeof(glm::vec3));
        batch.setInputBuffer(1, model->_blendedVertexBuffers[meshIndex], _drawMesh->getNumVertices() * sizeof(glm::vec3), sizeof(glm::vec3));
        batch.setInputStream(2, _drawMesh->getVertexStream().makeRangedStream(2));
    }
    
    // TODO: Get rid of that extra call
    if (!_hasColorAttrib) {
        batch._glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    }
}

void MeshPartPayload::bindMaterial(gpu::Batch& batch, const ModelRender::Locations* locations) const {
    if (!_drawMaterial) {
        return;
    }

    auto textureCache = DependencyManager::get<TextureCache>();

    batch.setUniformBuffer(ModelRender::MATERIAL_GPU_SLOT, _drawMaterial->getSchemaBuffer());

    auto materialKey = _drawMaterial->getKey();
    auto textureMaps = _drawMaterial->getTextureMaps();
    glm::mat4 texcoordTransform[2];

    // Diffuse
    if (materialKey.isDiffuseMap()) {
        auto diffuseMap = textureMaps[model::MaterialKey::DIFFUSE_MAP];
        if (diffuseMap && diffuseMap->isDefined()) {
            batch.setResourceTexture(ModelRender::DIFFUSE_MAP_SLOT, diffuseMap->getTextureView());

            if (!diffuseMap->getTextureTransform().isIdentity()) {
                diffuseMap->getTextureTransform().getMatrix(texcoordTransform[0]);
            }
        } else {
            batch.setResourceTexture(ModelRender::DIFFUSE_MAP_SLOT, textureCache->getGrayTexture());
        }
    } else {
        batch.setResourceTexture(ModelRender::DIFFUSE_MAP_SLOT, textureCache->getGrayTexture());
    }

    // Normal map
    if (materialKey.isNormalMap()) {
        auto normalMap = textureMaps[model::MaterialKey::NORMAL_MAP];
        if (normalMap && normalMap->isDefined()) {
            batch.setResourceTexture(ModelRender::NORMAL_MAP_SLOT, normalMap->getTextureView());

            // texcoord are assumed to be the same has diffuse
        } else {
            batch.setResourceTexture(ModelRender::NORMAL_MAP_SLOT, textureCache->getBlueTexture());
        }
    } else {
        batch.setResourceTexture(ModelRender::NORMAL_MAP_SLOT, nullptr);
    }

    // TODO: For now gloss map is used as the "specular map in the shading, we ll need to fix that
    if (materialKey.isGlossMap()) {
        auto specularMap = textureMaps[model::MaterialKey::GLOSS_MAP];
        if (specularMap && specularMap->isDefined()) {
            batch.setResourceTexture(ModelRender::SPECULAR_MAP_SLOT, specularMap->getTextureView());

            // texcoord are assumed to be the same has diffuse
        } else {
            batch.setResourceTexture(ModelRender::SPECULAR_MAP_SLOT, textureCache->getBlackTexture());
        }
    } else {
        batch.setResourceTexture(ModelRender::SPECULAR_MAP_SLOT, nullptr);
    }

    // TODO: For now lightmaop is piped into the emissive map unit, we need to fix that and support for real emissive too
    if (materialKey.isLightmapMap()) {
        auto lightmapMap = textureMaps[model::MaterialKey::LIGHTMAP_MAP];

        if (lightmapMap && lightmapMap->isDefined()) {
            batch.setResourceTexture(ModelRender::LIGHTMAP_MAP_SLOT, lightmapMap->getTextureView());

            auto lightmapOffsetScale = lightmapMap->getLightmapOffsetScale();
            batch._glUniform2f(locations->emissiveParams, lightmapOffsetScale.x, lightmapOffsetScale.y);

            if (!lightmapMap->getTextureTransform().isIdentity()) {
                lightmapMap->getTextureTransform().getMatrix(texcoordTransform[1]);
            }
        } else {
            batch.setResourceTexture(ModelRender::LIGHTMAP_MAP_SLOT, textureCache->getGrayTexture());
        }
    } else {
        batch.setResourceTexture(ModelRender::LIGHTMAP_MAP_SLOT, nullptr);
    }

    // Texcoord transforms ?
    if (locations->texcoordMatrices >= 0) {
        batch._glUniformMatrix4fv(locations->texcoordMatrices, 2, false, (const float*)&texcoordTransform);
    }
}

void MeshPartPayload::bindTransform(gpu::Batch& batch, const ModelRender::Locations* locations) const {
    // Still relying on the raw data from the model
    const Model::MeshState& state = model->_meshStates.at(meshIndex);

    Transform transform;
    if (state.clusterBuffer) {
        batch.setUniformBuffer(ModelRender::SKINNING_GPU_SLOT, state.clusterBuffer);
    } else {
        if (model->_cauterizeBones) {
            transform = Transform(state.cauterizedClusterMatrices[0]);
        } else {
            transform = Transform(state.clusterMatrices[0]);
        }
    }
    transform.preTranslate(model->_translation);
    batch.setModelTransform(transform);
}


void MeshPartPayload::render(RenderArgs* args) const {
    PerformanceTimer perfTimer("MeshPartPayload::render");
    if (!model->_readyWhenAdded) {
        return; // bail asap
    }

    gpu::Batch& batch = *(args->_batch);
    auto mode = args->_renderMode;
    
    auto alphaThreshold = args->_alphaThreshold; //translucent ? TRANSPARENT_ALPHA_THRESHOLD : OPAQUE_ALPHA_THRESHOLD; // FIX ME
    
    const FBXGeometry& geometry = model->_geometry->getFBXGeometry();
    const std::vector<std::unique_ptr<NetworkMesh>>& networkMeshes = model->_geometry->getMeshes();
    
    // guard against partially loaded meshes
    if (meshIndex >= (int)networkMeshes.size() || meshIndex >= (int)geometry.meshes.size() || meshIndex >= (int)model->_meshStates.size() ) {
        return;
    }
    
    // Back to model to update the cluster matrices right now
    model->updateClusterMatrices();
    
    const FBXMesh& mesh = geometry.meshes.at(meshIndex);
    
    // if our index is ever out of range for either meshes or networkMeshes, then skip it, and set our _meshGroupsKnown
    // to false to rebuild out mesh groups.
    if (meshIndex < 0 || meshIndex >= (int)networkMeshes.size() || meshIndex > geometry.meshes.size()) {
        model->_meshGroupsKnown = false; // regenerate these lists next time around.
        model->_readyWhenAdded = false; // in case any of our users are using scenes
        model->invalidCalculatedMeshBoxes(); // if we have to reload, we need to assume our mesh boxes are all invalid
        return; // FIXME!
    }
    
    
    int vertexCount = mesh.vertices.size();
    if (vertexCount == 0) {
        // sanity check
        return; // FIXME!
    }
    
    
    // guard against partially loaded meshes
    if (partIndex >= mesh.parts.size()) {
        return;
    }
    
    model::MaterialKey drawMaterialKey;
    if (_drawMaterial) {
        drawMaterialKey = _drawMaterial->getKey();
    }
    bool translucentMesh = drawMaterialKey.isTransparent() || drawMaterialKey.isTransparentMap();
    
    bool hasTangents = drawMaterialKey.isNormalMap() && !mesh.tangents.isEmpty();
    bool hasSpecular = drawMaterialKey.isGlossMap();
    bool hasLightmap = drawMaterialKey.isLightmapMap();
    bool isSkinned = _isSkinned;
    bool wireframe = model->isWireframe();
    
    // render the part bounding box
#ifdef DEBUG_BOUNDING_PARTS
    {
        AABox partBounds = getPartBounds(meshIndex, partIndex);
        bool inView = args->_viewFrustum->boxInFrustum(partBounds) != ViewFrustum::OUTSIDE;
        
        glm::vec4 cubeColor;
        if (isSkinned) {
            cubeColor = glm::vec4(0.0f, 1.0f, 1.0f, 1.0f);
        } else if (inView) {
            cubeColor = glm::vec4(1.0f, 0.0f, 1.0f, 1.0f);
        } else {
            cubeColor = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
        }
        
        Transform transform;
        transform.setTranslation(partBounds.calcCenter());
        transform.setScale(partBounds.getDimensions());
        batch.setModelTransform(transform);
        DependencyManager::get<DeferredLightingEffect>()->renderWireCube(batch, 1.0f, cubeColor);
    }
#endif //def DEBUG_BOUNDING_PARTS
    
    if (wireframe) {
        translucentMesh = hasTangents = hasSpecular = hasLightmap = isSkinned = false;
    }
    
    ModelRender::Locations* locations = nullptr;
    ModelRender::pickPrograms(batch, mode, translucentMesh, alphaThreshold, hasLightmap, hasTangents, hasSpecular, isSkinned, wireframe,
                              args, locations);
    

    // Bind the model transform and the skinCLusterMatrices if needed
    bindTransform(batch, locations);
    
    //Bind the index buffer and vertex buffer and Blend shapes if needed
    bindMesh(batch);
    
    // apply material properties
    bindMaterial(batch, locations);
        
        
    // TODO: We should be able to do that just in the renderTransparentJob
    if (translucentMesh && locations->lightBufferUnit >= 0) {
        PerformanceTimer perfTimer("DLE->setupTransparent()");
            
        DependencyManager::get<DeferredLightingEffect>()->setupTransparent(args, locations->lightBufferUnit);
    }
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

