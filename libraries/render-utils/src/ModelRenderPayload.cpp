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

#include "DeferredLightingEffect.h"

#include "Model.h"

#include "model_vert.h"
#include "model_shadow_vert.h"
#include "model_normal_map_vert.h"
#include "model_lightmap_vert.h"
#include "model_lightmap_normal_map_vert.h"
#include "skin_model_vert.h"
#include "skin_model_shadow_vert.h"
#include "skin_model_normal_map_vert.h"

#include "model_frag.h"
#include "model_shadow_frag.h"
#include "model_normal_map_frag.h"
#include "model_normal_specular_map_frag.h"
#include "model_specular_map_frag.h"
#include "model_lightmap_frag.h"
#include "model_lightmap_normal_map_frag.h"
#include "model_lightmap_normal_specular_map_frag.h"
#include "model_lightmap_specular_map_frag.h"
#include "model_translucent_frag.h"

ModelRender::RenderPipelineLib ModelRender::_renderPipelineLib;

const ModelRender::RenderPipelineLib& ModelRender::getRenderPipelineLib() {
    if (_renderPipelineLib.empty()) {
        // Vertex shaders
        auto modelVertex = gpu::ShaderPointer(gpu::Shader::createVertex(std::string(model_vert)));
        auto modelNormalMapVertex = gpu::ShaderPointer(gpu::Shader::createVertex(std::string(model_normal_map_vert)));
        auto modelLightmapVertex = gpu::ShaderPointer(gpu::Shader::createVertex(std::string(model_lightmap_vert)));
        auto modelLightmapNormalMapVertex = gpu::ShaderPointer(gpu::Shader::createVertex(std::string(model_lightmap_normal_map_vert)));
        auto modelShadowVertex = gpu::ShaderPointer(gpu::Shader::createVertex(std::string(model_shadow_vert)));
        auto skinModelVertex = gpu::ShaderPointer(gpu::Shader::createVertex(std::string(skin_model_vert)));
        auto skinModelNormalMapVertex = gpu::ShaderPointer(gpu::Shader::createVertex(std::string(skin_model_normal_map_vert)));
        auto skinModelShadowVertex = gpu::ShaderPointer(gpu::Shader::createVertex(std::string(skin_model_shadow_vert)));

        // Pixel shaders
        auto modelPixel = gpu::ShaderPointer(gpu::Shader::createPixel(std::string(model_frag)));
        auto modelNormalMapPixel = gpu::ShaderPointer(gpu::Shader::createPixel(std::string(model_normal_map_frag)));
        auto modelSpecularMapPixel = gpu::ShaderPointer(gpu::Shader::createPixel(std::string(model_specular_map_frag)));
        auto modelNormalSpecularMapPixel = gpu::ShaderPointer(gpu::Shader::createPixel(std::string(model_normal_specular_map_frag)));
        auto modelTranslucentPixel = gpu::ShaderPointer(gpu::Shader::createPixel(std::string(model_translucent_frag)));
        auto modelShadowPixel = gpu::ShaderPointer(gpu::Shader::createPixel(std::string(model_shadow_frag)));
        auto modelLightmapPixel = gpu::ShaderPointer(gpu::Shader::createPixel(std::string(model_lightmap_frag)));
        auto modelLightmapNormalMapPixel = gpu::ShaderPointer(gpu::Shader::createPixel(std::string(model_lightmap_normal_map_frag)));
        auto modelLightmapSpecularMapPixel = gpu::ShaderPointer(gpu::Shader::createPixel(std::string(model_lightmap_specular_map_frag)));
        auto modelLightmapNormalSpecularMapPixel = gpu::ShaderPointer(gpu::Shader::createPixel(std::string(model_lightmap_normal_specular_map_frag)));

        // Fill the renderPipelineLib

        _renderPipelineLib.addRenderPipeline(
            RenderKey(0),
            modelVertex, modelPixel);

        _renderPipelineLib.addRenderPipeline(
            RenderKey(RenderKey::HAS_TANGENTS),
            modelNormalMapVertex, modelNormalMapPixel);

        _renderPipelineLib.addRenderPipeline(
            RenderKey(RenderKey::HAS_SPECULAR),
            modelVertex, modelSpecularMapPixel);

        _renderPipelineLib.addRenderPipeline(
            RenderKey(RenderKey::HAS_TANGENTS | RenderKey::HAS_SPECULAR),
            modelNormalMapVertex, modelNormalSpecularMapPixel);


        _renderPipelineLib.addRenderPipeline(
            RenderKey(RenderKey::IS_TRANSLUCENT),
            modelVertex, modelTranslucentPixel);
        // FIXME Ignore lightmap for translucents meshpart
        _renderPipelineLib.addRenderPipeline(
            RenderKey(RenderKey::IS_TRANSLUCENT | RenderKey::HAS_LIGHTMAP),
            modelVertex, modelTranslucentPixel);

        _renderPipelineLib.addRenderPipeline(
            RenderKey(RenderKey::HAS_TANGENTS | RenderKey::IS_TRANSLUCENT),
            modelNormalMapVertex, modelTranslucentPixel);

        _renderPipelineLib.addRenderPipeline(
            RenderKey(RenderKey::HAS_SPECULAR | RenderKey::IS_TRANSLUCENT),
            modelVertex, modelTranslucentPixel);

        _renderPipelineLib.addRenderPipeline(
            RenderKey(RenderKey::HAS_TANGENTS | RenderKey::HAS_SPECULAR | RenderKey::IS_TRANSLUCENT),
            modelNormalMapVertex, modelTranslucentPixel);


        _renderPipelineLib.addRenderPipeline(
            RenderKey(RenderKey::HAS_LIGHTMAP),
            modelLightmapVertex, modelLightmapPixel);
        _renderPipelineLib.addRenderPipeline(
            RenderKey(RenderKey::HAS_LIGHTMAP | RenderKey::HAS_TANGENTS),
            modelLightmapNormalMapVertex, modelLightmapNormalMapPixel);

        _renderPipelineLib.addRenderPipeline(
            RenderKey(RenderKey::HAS_LIGHTMAP | RenderKey::HAS_SPECULAR),
            modelLightmapVertex, modelLightmapSpecularMapPixel);

        _renderPipelineLib.addRenderPipeline(
            RenderKey(RenderKey::HAS_LIGHTMAP | RenderKey::HAS_TANGENTS | RenderKey::HAS_SPECULAR),
            modelLightmapNormalMapVertex, modelLightmapNormalSpecularMapPixel);


        _renderPipelineLib.addRenderPipeline(
            RenderKey(RenderKey::IS_SKINNED),
            skinModelVertex, modelPixel);

        _renderPipelineLib.addRenderPipeline(
            RenderKey(RenderKey::IS_SKINNED | RenderKey::HAS_TANGENTS),
            skinModelNormalMapVertex, modelNormalMapPixel);

        _renderPipelineLib.addRenderPipeline(
            RenderKey(RenderKey::IS_SKINNED | RenderKey::HAS_SPECULAR),
            skinModelVertex, modelSpecularMapPixel);

        _renderPipelineLib.addRenderPipeline(
            RenderKey(RenderKey::IS_SKINNED | RenderKey::HAS_TANGENTS | RenderKey::HAS_SPECULAR),
            skinModelNormalMapVertex, modelNormalSpecularMapPixel);


        _renderPipelineLib.addRenderPipeline(
            RenderKey(RenderKey::IS_SKINNED | RenderKey::IS_TRANSLUCENT),
            skinModelVertex, modelTranslucentPixel);

        _renderPipelineLib.addRenderPipeline(
            RenderKey(RenderKey::IS_SKINNED | RenderKey::HAS_TANGENTS | RenderKey::IS_TRANSLUCENT),
            skinModelNormalMapVertex, modelTranslucentPixel);

        _renderPipelineLib.addRenderPipeline(
            RenderKey(RenderKey::IS_SKINNED | RenderKey::HAS_SPECULAR | RenderKey::IS_TRANSLUCENT),
            skinModelVertex, modelTranslucentPixel);

        _renderPipelineLib.addRenderPipeline(
            RenderKey(RenderKey::IS_SKINNED | RenderKey::HAS_TANGENTS | RenderKey::HAS_SPECULAR | RenderKey::IS_TRANSLUCENT),
            skinModelNormalMapVertex, modelTranslucentPixel);


        _renderPipelineLib.addRenderPipeline(
            RenderKey(RenderKey::IS_DEPTH_ONLY | RenderKey::IS_SHADOW),
            modelShadowVertex, modelShadowPixel);


        _renderPipelineLib.addRenderPipeline(
            RenderKey(RenderKey::IS_SKINNED | RenderKey::IS_DEPTH_ONLY | RenderKey::IS_SHADOW),
            skinModelShadowVertex, modelShadowPixel);
    }

    return _renderPipelineLib;
}


void ModelRender::RenderPipelineLib::addRenderPipeline(ModelRender::RenderKey key,
    gpu::ShaderPointer& vertexShader,
    gpu::ShaderPointer& pixelShader) {

    gpu::Shader::BindingSet slotBindings;
    
    slotBindings.insert(gpu::Shader::Binding(std::string("skinClusterBuffer"), ModelRender::SKINNING_GPU_SLOT));
    slotBindings.insert(gpu::Shader::Binding(std::string("materialBuffer"), ModelRender::MATERIAL_GPU_SLOT));
    slotBindings.insert(gpu::Shader::Binding(std::string("diffuseMap"), ModelRender::DIFFUSE_MAP_SLOT));
    slotBindings.insert(gpu::Shader::Binding(std::string("normalMap"), ModelRender::NORMAL_MAP_SLOT));
    slotBindings.insert(gpu::Shader::Binding(std::string("specularMap"), ModelRender::SPECULAR_MAP_SLOT));
    slotBindings.insert(gpu::Shader::Binding(std::string("emissiveMap"), ModelRender::LIGHTMAP_MAP_SLOT));
    slotBindings.insert(gpu::Shader::Binding(std::string("lightBuffer"), ModelRender::LIGHT_BUFFER_SLOT));
    slotBindings.insert(gpu::Shader::Binding(std::string("normalFittingMap"), DeferredLightingEffect::NORMAL_FITTING_MAP_SLOT));

    gpu::ShaderPointer program = gpu::ShaderPointer(gpu::Shader::createProgram(vertexShader, pixelShader));
    gpu::Shader::makeProgram(*program, slotBindings);


    auto locations = std::make_shared<Locations>();
    initLocations(program, *locations);


    auto state = std::make_shared<gpu::State>();

    // Backface on shadow
    if (key.isShadow()) {
        state->setCullMode(gpu::State::CULL_FRONT);
        state->setDepthBias(1.0f);
        state->setDepthBiasSlopeScale(4.0f);
    } else {
        state->setCullMode(gpu::State::CULL_BACK);
    }

    // Z test depends if transparent or not
    state->setDepthTest(true, !key.isTranslucent(), gpu::LESS_EQUAL);

    // Blend on transparent
    state->setBlendFunction(key.isTranslucent(),
        gpu::State::ONE, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA, // For transparent only, this keep the highlight intensity
        gpu::State::FACTOR_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::ONE);

    // Good to go add the brand new pipeline
    auto pipeline = gpu::PipelinePointer(gpu::Pipeline::create(program, state));
    insert(value_type(key.getRaw(), RenderPipeline(pipeline, locations)));


    if (!key.isWireFrame()) {

        RenderKey wireframeKey(key.getRaw() | RenderKey::IS_WIREFRAME);
        auto wireframeState = std::make_shared<gpu::State>(state->getValues());

        wireframeState->setFillMode(gpu::State::FILL_LINE);

        // create a new RenderPipeline with the same shader side and the mirrorState
        auto wireframePipeline = gpu::PipelinePointer(gpu::Pipeline::create(program, wireframeState));
        insert(value_type(wireframeKey.getRaw(), RenderPipeline(wireframePipeline, locations)));
    }

    // If not a shadow pass, create the mirror version from the same state, just change the FrontFace
    if (!key.isShadow()) {

        RenderKey mirrorKey(key.getRaw() | RenderKey::IS_MIRROR);
        auto mirrorState = std::make_shared<gpu::State>(state->getValues());

        // create a new RenderPipeline with the same shader side and the mirrorState
        auto mirrorPipeline = gpu::PipelinePointer(gpu::Pipeline::create(program, mirrorState));
        insert(value_type(mirrorKey.getRaw(), RenderPipeline(mirrorPipeline, locations)));

        if (!key.isWireFrame()) {
            RenderKey wireframeKey(key.getRaw() | RenderKey::IS_MIRROR | RenderKey::IS_WIREFRAME);
            auto wireframeState = std::make_shared<gpu::State>(state->getValues());

            wireframeState->setFillMode(gpu::State::FILL_LINE);

            // create a new RenderPipeline with the same shader side and the mirrorState
            auto wireframePipeline = gpu::PipelinePointer(gpu::Pipeline::create(program, wireframeState));
            insert(value_type(wireframeKey.getRaw(), RenderPipeline(wireframePipeline, locations)));
        }
    }
}


void ModelRender::RenderPipelineLib::initLocations(gpu::ShaderPointer& program, ModelRender::Locations& locations) {
    locations.alphaThreshold = program->getUniforms().findLocation("alphaThreshold");
    locations.texcoordMatrices = program->getUniforms().findLocation("texcoordMatrices");
    locations.emissiveParams = program->getUniforms().findLocation("emissiveParams");
    locations.glowIntensity = program->getUniforms().findLocation("glowIntensity");
    locations.normalFittingMapUnit = program->getTextures().findLocation("normalFittingMap");
    locations.diffuseTextureUnit = program->getTextures().findLocation("diffuseMap");
    locations.normalTextureUnit = program->getTextures().findLocation("normalMap");
    locations.specularTextureUnit = program->getTextures().findLocation("specularMap");
    locations.emissiveTextureUnit = program->getTextures().findLocation("emissiveMap");
    locations.materialBufferUnit = program->getBuffers().findLocation("materialBuffer");
    locations.lightBufferUnit = program->getBuffers().findLocation("lightBuffer");
    locations.skinClusterBufferUnit = program->getBuffers().findLocation("skinClusterBuffer");
}


void ModelRender::pickPrograms(gpu::Batch& batch, RenderArgs::RenderMode mode, bool translucent, float alphaThreshold,
    bool hasLightmap, bool hasTangents, bool hasSpecular, bool isSkinned, bool isWireframe, RenderArgs* args,
    Locations*& locations) {

 //   PerformanceTimer perfTimer("Model::pickPrograms");
    getRenderPipelineLib();

    RenderKey key(mode, translucent, alphaThreshold, hasLightmap, hasTangents, hasSpecular, isSkinned, isWireframe);
    if (mode == RenderArgs::MIRROR_RENDER_MODE) {
        key = RenderKey(key.getRaw() | RenderKey::IS_MIRROR);
    }
    auto pipeline = _renderPipelineLib.find(key.getRaw());
    if (pipeline == _renderPipelineLib.end()) {
        qDebug() << "No good, couldn't find a pipeline from the key ?" << key.getRaw();
        locations = 0;
        return;
    }

    gpu::ShaderPointer program = (*pipeline).second._pipeline->getProgram();
    locations = (*pipeline).second._locations.get();


    // Setup the One pipeline
    batch.setPipeline((*pipeline).second._pipeline);

    if ((locations->alphaThreshold > -1) && (mode != RenderArgs::SHADOW_RENDER_MODE)) {
        batch._glUniform1f(locations->alphaThreshold, alphaThreshold);
    }

    if ((locations->glowIntensity > -1) && (mode != RenderArgs::SHADOW_RENDER_MODE)) {
        const float DEFAULT_GLOW_INTENSITY = 1.0f; // FIXME - glow is removed
        batch._glUniform1f(locations->glowIntensity, DEFAULT_GLOW_INTENSITY);
    }

    if ((locations->normalFittingMapUnit > -1)) {
        batch.setResourceTexture(locations->normalFittingMapUnit,
            DependencyManager::get<TextureCache>()->getNormalFittingTexture());
    }
}

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
    model(model), url(model->getURL()), meshIndex(meshIndex), partIndex(partIndex), _shapeID(shapeIndex)
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

void MeshPartPayload::render(RenderArgs* args) const {
    return model->renderPart(args, meshIndex, partIndex, _shapeID, this);
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

