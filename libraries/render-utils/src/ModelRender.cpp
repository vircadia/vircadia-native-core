//
//  ModelRender.cpp
//  interface/src/renderer
//
//  Created by Sam Gateau on 10/3/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ModelRender.h"

#include <model-networking/TextureCache.h>

#include <PerfStat.h>

#include "DeferredLightingEffect.h"

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

        // create a new RenderPipeline with the same shader side and the wireframe state
        auto wireframePipeline = gpu::PipelinePointer(gpu::Pipeline::create(program, wireframeState));
        insert(value_type(wireframeKey.getRaw(), RenderPipeline(wireframePipeline, locations)));
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
    locations.skinClusterBufferUnit = program->getBuffers().findLocation("skinClusterBuffer");
    locations.materialBufferUnit = program->getBuffers().findLocation("materialBuffer");
    locations.lightBufferUnit = program->getBuffers().findLocation("lightBuffer");

}


void ModelRender::pickPrograms(gpu::Batch& batch, RenderArgs::RenderMode mode, bool translucent, float alphaThreshold,
    bool hasLightmap, bool hasTangents, bool hasSpecular, bool isSkinned, bool isWireframe, RenderArgs* args,
    Locations*& locations) {

    PerformanceTimer perfTimer("Model::pickPrograms");
    getRenderPipelineLib();

    RenderKey key(mode, translucent, alphaThreshold, hasLightmap, hasTangents, hasSpecular, isSkinned, isWireframe);
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
