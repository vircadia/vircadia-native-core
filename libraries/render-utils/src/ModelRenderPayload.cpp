//
//  ModelRenderPayload.cpp
//  interface/src/renderer
//
//  Created by Sam Gateau on 10/3/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ModelRenderPayload.h"

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
    
    /* template <> const model::MaterialKey& shapeGetMaterialKey(const MeshPartPayload::Pointer& payload) {
     return payload->model->getPartMaterial(payload->meshIndex, payload->partIndex);
     }*/
}

using namespace render;

MeshPartPayload::MeshPartPayload(Model* model, int meshIndex, int partIndex, int shapeIndex) :
    model(model), url(model->getURL()), meshIndex(meshIndex), partIndex(partIndex), _shapeID(shapeIndex)
{
    const std::vector<std::unique_ptr<NetworkMesh>>& networkMeshes = model->_geometry->getMeshes();
    const NetworkMesh& networkMesh = *(networkMeshes.at(meshIndex).get());
    _drawMesh = networkMesh._mesh;
    
    _drawPart = _drawMesh->getPartBuffer().get<model::Mesh::Part>(partIndex);

}


render::ItemKey MeshPartPayload::getKey() const {
    if (!model->isVisible()) {
        return ItemKey::Builder().withInvisible().build();
    }
    auto geometry = model->getGeometry();
    if (!geometry.isNull()) {
        auto drawMaterial = geometry->getShapeMaterial(_shapeID);
        if (drawMaterial) {
            auto matKey = drawMaterial->_material->getKey();
            if (matKey.isTransparent() || matKey.isTransparentMap()) {
                return ItemKey::Builder::transparentShape();
            } else {
                return ItemKey::Builder::opaqueShape();
            }
        }
    }
    
    // Return opaque for lack of a better idea
    return ItemKey::Builder::opaqueShape();
}

render::Item::Bound MeshPartPayload::getBound() const {
    if (_isBoundInvalid) {
        model->getPartBounds(meshIndex, partIndex);
        _isBoundInvalid = false;
    }
    return _bound;
}

void MeshPartPayload::render(RenderArgs* args) const {
    return model->renderPart(args, meshIndex, partIndex, _shapeID, this);
}

void MeshPartPayload::bindMesh(gpu::Batch& batch) const {
    const FBXGeometry& geometry = model->_geometry->getFBXGeometry();
    const std::vector<std::unique_ptr<NetworkMesh>>& networkMeshes = model->_geometry->getMeshes();
    const NetworkMesh& networkMesh = *(networkMeshes.at(meshIndex).get());
    const FBXMesh& mesh = geometry.meshes.at(meshIndex);
  //  auto drawMesh = networkMesh._mesh;
    
    if (mesh.blendshapes.isEmpty()) {
        batch.setIndexBuffer(gpu::UINT32, (_drawMesh->getIndexBuffer()._buffer), 0);
        
        batch.setInputFormat((_drawMesh->getVertexFormat()));
        auto inputStream = _drawMesh->makeBufferStream();
        
        batch.setInputStream(0, inputStream);
    } else {
        batch.setIndexBuffer(gpu::UINT32, (_drawMesh->getIndexBuffer()._buffer), 0);
        batch.setInputFormat((_drawMesh->getVertexFormat()));
        
        batch.setInputBuffer(0, model->_blendedVertexBuffers[meshIndex], 0, sizeof(glm::vec3));
        batch.setInputBuffer(1, model->_blendedVertexBuffers[meshIndex], _drawMesh->getNumVertices() * sizeof(glm::vec3), sizeof(glm::vec3));
        
        auto inputStream = _drawMesh->makeBufferStream().makeRangedStream(2);
        
        batch.setInputStream(2, inputStream);
    }
    
    if (mesh.colors.isEmpty()) {
        batch._glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    }
}

void MeshPartPayload::drawCall(gpu::Batch& batch) const {
    batch.drawIndexed(gpu::TRIANGLES, _drawPart._numIndices, _drawPart._startIndex);
}
/*
void Model::renderPart(RenderArgs* args, int meshIndex, int partIndex, int shapeID) {
    //   PROFILE_RANGE(__FUNCTION__);
    PerformanceTimer perfTimer("Model::renderPart");
    if (!_readyWhenAdded) {
        return; // bail asap
    }
    
    auto textureCache = DependencyManager::get<TextureCache>();
    
    gpu::Batch& batch = *(args->_batch);
    auto mode = args->_renderMode;
    
    
    // Capture the view matrix once for the rendering of this model
    if (_transforms.empty()) {
        _transforms.push_back(Transform());
    }
    
    auto alphaThreshold = args->_alphaThreshold; //translucent ? TRANSPARENT_ALPHA_THRESHOLD : OPAQUE_ALPHA_THRESHOLD; // FIX ME
    
    const FBXGeometry& geometry = _geometry->getFBXGeometry();
    const std::vector<std::unique_ptr<NetworkMesh>>& networkMeshes = _geometry->getMeshes();
    
    auto networkMaterial = _geometry->getShapeMaterial(shapeID);
    if (!networkMaterial) {
        return;
    };
    auto material = networkMaterial->_material;
    if (!material) {
        return;
    }
    
    // TODO: Not yet
    // auto drawMesh = _geometry->getShapeMesh(shapeID);
    // auto drawPart = _geometry->getShapePart(shapeID);
    
    // guard against partially loaded meshes
    if (meshIndex >= (int)networkMeshes.size() || meshIndex >= (int)geometry.meshes.size() || meshIndex >= (int)_meshStates.size() ) {
        return;
    }
    
    updateClusterMatrices();
    
    const NetworkMesh& networkMesh = *(networkMeshes.at(meshIndex).get());
    const FBXMesh& mesh = geometry.meshes.at(meshIndex);
    const MeshState& state = _meshStates.at(meshIndex);
    
    auto drawMesh = networkMesh._mesh;
    
    
    auto drawMaterialKey = material->getKey();
    bool translucentMesh = drawMaterialKey.isTransparent() || drawMaterialKey.isTransparentMap();
    
    bool hasTangents = drawMaterialKey.isNormalMap() && !mesh.tangents.isEmpty();
    bool hasSpecular = drawMaterialKey.isGlossMap(); // !drawMaterial->specularTextureName.isEmpty(); //mesh.hasSpecularTexture();
    bool hasLightmap = drawMaterialKey.isLightmapMap(); // !drawMaterial->emissiveTextureName.isEmpty(); //mesh.hasEmissiveTexture();
    bool isSkinned = state.clusterMatrices.size() > 1;
    bool wireframe = isWireframe();
    
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
    
    Locations* locations = nullptr;
    pickPrograms(batch, mode, translucentMesh, alphaThreshold, hasLightmap, hasTangents, hasSpecular, isSkinned, wireframe,
                 args, locations);
    
    // if our index is ever out of range for either meshes or networkMeshes, then skip it, and set our _meshGroupsKnown
    // to false to rebuild out mesh groups.
    if (meshIndex < 0 || meshIndex >= (int)networkMeshes.size() || meshIndex > geometry.meshes.size()) {
        _meshGroupsKnown = false; // regenerate these lists next time around.
        _readyWhenAdded = false; // in case any of our users are using scenes
        invalidCalculatedMeshBoxes(); // if we have to reload, we need to assume our mesh boxes are all invalid
        return; // FIXME!
    }
    
    int vertexCount = mesh.vertices.size();
    if (vertexCount == 0) {
        // sanity check
        return; // FIXME!
    }
    
    // Transform stage
    if (_transforms.empty()) {
        _transforms.push_back(Transform());
    }
    
    if (isSkinned) {
        const float* bones;
        if (_cauterizeBones) {
            bones = (const float*)state.cauterizedClusterMatrices.constData();
        } else {
            bones = (const float*)state.clusterMatrices.constData();
        }
        batch._glUniformMatrix4fv(locations->clusterMatrices, state.clusterMatrices.size(), false, bones);
        _transforms[0] = Transform();
        _transforms[0].preTranslate(_translation);
    } else {
        if (_cauterizeBones) {
            _transforms[0] = Transform(state.cauterizedClusterMatrices[0]);
        } else {
            _transforms[0] = Transform(state.clusterMatrices[0]);
        }
        _transforms[0].preTranslate(_translation);
    }
    batch.setModelTransform(_transforms[0]);
    
    auto drawPart = drawMesh->getPartBuffer().get<model::Mesh::Part>(partIndex);
    
    if (mesh.blendshapes.isEmpty()) {
        batch.setIndexBuffer(gpu::UINT32, (drawMesh->getIndexBuffer()._buffer), 0);
        
        batch.setInputFormat((drawMesh->getVertexFormat()));
        auto inputStream = drawMesh->makeBufferStream();
        
        batch.setInputStream(0, inputStream);
    } else {
        batch.setIndexBuffer(gpu::UINT32, (drawMesh->getIndexBuffer()._buffer), 0);
        batch.setInputFormat((drawMesh->getVertexFormat()));
        
        batch.setInputBuffer(0, _blendedVertexBuffers[meshIndex], 0, sizeof(glm::vec3));
        batch.setInputBuffer(1, _blendedVertexBuffers[meshIndex], vertexCount * sizeof(glm::vec3), sizeof(glm::vec3));
        
        auto inputStream = drawMesh->makeBufferStream().makeRangedStream(2);
        
        batch.setInputStream(2, inputStream);
    }
    
    if (mesh.colors.isEmpty()) {
        batch._glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    }
    
    // guard against partially loaded meshes
    if (partIndex >= mesh.parts.size()) {
        return;
    }
    
    
#ifdef WANT_DEBUG
    if (material == nullptr) {
        qCDebug(renderutils) << "WARNING: material == nullptr!!!";
    }
#endif
    
    {
        
        // apply material properties
        if (mode != RenderArgs::SHADOW_RENDER_MODE) {
#ifdef WANT_DEBUG
            qCDebug(renderutils) << "Material Changed ---------------------------------------------";
            qCDebug(renderutils) << "part INDEX:" << partIndex;
            qCDebug(renderutils) << "NEW part.materialID:" << part.materialID;
#endif //def WANT_DEBUG
            
            if (locations->materialBufferUnit >= 0) {
                batch.setUniformBuffer(locations->materialBufferUnit, material->getSchemaBuffer());
            }
            
            auto materialKey = material->getKey();
            auto textureMaps = material->getTextureMaps();
            glm::mat4 texcoordTransform[2];
            
            // Diffuse
            if (materialKey.isDiffuseMap()) {
                auto diffuseMap = textureMaps[model::MaterialKey::DIFFUSE_MAP];
                if (diffuseMap && diffuseMap->isDefined()) {
                    batch.setResourceTexture(DIFFUSE_MAP_SLOT, diffuseMap->getTextureView());
                    
                    if (!diffuseMap->getTextureTransform().isIdentity()) {
                        diffuseMap->getTextureTransform().getMatrix(texcoordTransform[0]);
                    }
                } else {
                    batch.setResourceTexture(DIFFUSE_MAP_SLOT, textureCache->getGrayTexture());
                }
            } else {
                batch.setResourceTexture(DIFFUSE_MAP_SLOT, textureCache->getGrayTexture());
            }
            
            // Normal map
            if ((locations->normalTextureUnit >= 0) && hasTangents) {
                auto normalMap = textureMaps[model::MaterialKey::NORMAL_MAP];
                if (normalMap && normalMap->isDefined()) {
                    batch.setResourceTexture(NORMAL_MAP_SLOT, normalMap->getTextureView());
                    
                    // texcoord are assumed to be the same has diffuse
                } else {
                    batch.setResourceTexture(NORMAL_MAP_SLOT, textureCache->getBlueTexture());
                }
            } else {
                batch.setResourceTexture(NORMAL_MAP_SLOT, nullptr);
            }
            
            // TODO: For now gloss map is used as the "specular map in the shading, we ll need to fix that
            if ((locations->specularTextureUnit >= 0) && materialKey.isGlossMap()) {
                auto specularMap = textureMaps[model::MaterialKey::GLOSS_MAP];
                if (specularMap && specularMap->isDefined()) {
                    batch.setResourceTexture(SPECULAR_MAP_SLOT, specularMap->getTextureView());
                    
                    // texcoord are assumed to be the same has diffuse
                } else {
                    batch.setResourceTexture(SPECULAR_MAP_SLOT, textureCache->getBlackTexture());
                }
            } else {
                batch.setResourceTexture(SPECULAR_MAP_SLOT, nullptr);
            }
            
            // TODO: For now lightmaop is piped into the emissive map unit, we need to fix that and support for real emissive too
            if ((locations->emissiveTextureUnit >= 0) && materialKey.isLightmapMap()) {
                auto lightmapMap = textureMaps[model::MaterialKey::LIGHTMAP_MAP];
                
                if (lightmapMap && lightmapMap->isDefined()) {
                    batch.setResourceTexture(LIGHTMAP_MAP_SLOT, lightmapMap->getTextureView());
                    
                    auto lightmapOffsetScale = lightmapMap->getLightmapOffsetScale();
                    batch._glUniform2f(locations->emissiveParams, lightmapOffsetScale.x, lightmapOffsetScale.y);
                    
                    if (!lightmapMap->getTextureTransform().isIdentity()) {
                        lightmapMap->getTextureTransform().getMatrix(texcoordTransform[1]);
                    }
                }
                else {
                    batch.setResourceTexture(LIGHTMAP_MAP_SLOT, textureCache->getGrayTexture());
                }
            } else {
                batch.setResourceTexture(LIGHTMAP_MAP_SLOT, nullptr);
            }
            
            // Texcoord transforms ?
            if (locations->texcoordMatrices >= 0) {
                batch._glUniformMatrix4fv(locations->texcoordMatrices, 2, false, (const float*)&texcoordTransform);
            }
            
            // TODO: We should be able to do that just in the renderTransparentJob
            if (translucentMesh && locations->lightBufferUnit >= 0) {
                PerformanceTimer perfTimer("DLE->setupTransparent()");
                
                DependencyManager::get<DeferredLightingEffect>()->setupTransparent(args, locations->lightBufferUnit);
            }
            
            
            if (args) {
                args->_details._materialSwitches++;
            }
        }
    }

    if (args) {
        const int INDICES_PER_TRIANGLE = 3;
        args->_details._trianglesRendered += drawPart._numIndices / INDICES_PER_TRIANGLE;
    }
}
*/