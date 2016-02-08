
//
//  RenderPipelines.cpp
//  render-utils/src/
//
//  Created by Zach Pomerantz on 1/28/2016.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <gpu/Context.h>
#include <gpu/StandardShaderLib.h>

#include "TextureCache.h"
#include "render/DrawTask.h"

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

#include "overlay3D_vert.h"
#include "overlay3D_frag.h"

#include "drawOpaqueStencil_frag.h"

using namespace render;

void initStencilPipeline(gpu::PipelinePointer& pipeline) {
    const gpu::int8 STENCIL_OPAQUE = 1;
    auto vs = gpu::StandardShaderLib::getDrawUnitQuadTexcoordVS();
    auto ps = gpu::Shader::createPixel(std::string(drawOpaqueStencil_frag));
    auto program = gpu::Shader::createProgram(vs, ps);
    gpu::Shader::makeProgram((*program));

    auto state = std::make_shared<gpu::State>();
    state->setDepthTest(true, false, gpu::LESS_EQUAL);
    state->setStencilTest(true, 0xFF, gpu::State::StencilTest(STENCIL_OPAQUE, 0xFF, gpu::ALWAYS, gpu::State::STENCIL_OP_REPLACE, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_REPLACE));
    state->setColorWriteMask(0);

    pipeline = gpu::Pipeline::create(program, state);
}

void initOverlay3DPipelines(ShapePlumber& plumber) {
    auto vs = gpu::Shader::createVertex(std::string(overlay3D_vert));
    auto ps = gpu::Shader::createPixel(std::string(overlay3D_frag));
    auto program = gpu::Shader::createProgram(vs, ps);

    auto opaqueState = std::make_shared<gpu::State>();
    opaqueState->setDepthTest(false);
    opaqueState->setBlendFunction(false);

    plumber.addPipeline(ShapeKey::Filter::Builder().withOpaque(), program, opaqueState);
}

void pipelineBatchSetter(const ShapePipeline& pipeline, gpu::Batch& batch) {
    if (pipeline.locations->normalFittingMapUnit > -1) {
        batch.setResourceTexture(pipeline.locations->normalFittingMapUnit,
            DependencyManager::get<TextureCache>()->getNormalFittingTexture());
    }
}

void initDeferredPipelines(render::ShapePlumber& plumber) {
    using Key = render::ShapeKey;
    using ShaderPointer = gpu::ShaderPointer;

    auto addPipeline = [&plumber](const Key& key, const ShaderPointer& vertexShader, const ShaderPointer& pixelShader) {
        auto state = std::make_shared<gpu::State>();

        // Cull backface
        state->setCullMode(gpu::State::CULL_BACK);

        // Z test depends on transparency
        state->setDepthTest(true, !key.isTranslucent(), gpu::LESS_EQUAL);

        // Blend if transparent
        state->setBlendFunction(key.isTranslucent(),
            // For transparency, keep the highlight intensity
            gpu::State::ONE, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA,
            gpu::State::FACTOR_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::ONE);

        ShaderPointer program = gpu::Shader::createProgram(vertexShader, pixelShader);
        plumber.addPipeline(key, program, state, &pipelineBatchSetter);

        // Add a wireframe version
        if (!key.isWireFrame()) {
            auto wireFrameKey = Key::Builder(key).withWireframe();
            auto wireFrameState = std::make_shared<gpu::State>(state->getValues());

            wireFrameState->setFillMode(gpu::State::FILL_LINE);

            plumber.addPipeline(wireFrameKey, program, wireFrameState, &pipelineBatchSetter);
        }
    };

    // Vertex shaders
    auto modelVertex = gpu::Shader::createVertex(std::string(model_vert));
    auto modelNormalMapVertex = gpu::Shader::createVertex(std::string(model_normal_map_vert));
    auto modelLightmapVertex = gpu::Shader::createVertex(std::string(model_lightmap_vert));
    auto modelLightmapNormalMapVertex = gpu::Shader::createVertex(std::string(model_lightmap_normal_map_vert));
    auto modelShadowVertex = gpu::Shader::createVertex(std::string(model_shadow_vert));
    auto skinModelVertex = gpu::Shader::createVertex(std::string(skin_model_vert));
    auto skinModelNormalMapVertex = gpu::Shader::createVertex(std::string(skin_model_normal_map_vert));
    auto skinModelShadowVertex = gpu::Shader::createVertex(std::string(skin_model_shadow_vert));

    // Pixel shaders
    auto modelPixel = gpu::Shader::createPixel(std::string(model_frag));
    auto modelNormalMapPixel = gpu::Shader::createPixel(std::string(model_normal_map_frag));
    auto modelSpecularMapPixel = gpu::Shader::createPixel(std::string(model_specular_map_frag));
    auto modelNormalSpecularMapPixel = gpu::Shader::createPixel(std::string(model_normal_specular_map_frag));
    auto modelTranslucentPixel = gpu::Shader::createPixel(std::string(model_translucent_frag));
    auto modelShadowPixel = gpu::Shader::createPixel(std::string(model_shadow_frag));
    auto modelLightmapPixel = gpu::Shader::createPixel(std::string(model_lightmap_frag));
    auto modelLightmapNormalMapPixel = gpu::Shader::createPixel(std::string(model_lightmap_normal_map_frag));
    auto modelLightmapSpecularMapPixel = gpu::Shader::createPixel(std::string(model_lightmap_specular_map_frag));
    auto modelLightmapNormalSpecularMapPixel = gpu::Shader::createPixel(std::string(model_lightmap_normal_specular_map_frag));

    // Fill the pipelineLib
    addPipeline(
        Key::Builder(),
        modelVertex, modelPixel);

    addPipeline(
        Key::Builder().withTangents(),
        modelNormalMapVertex, modelNormalMapPixel);

    addPipeline(
        Key::Builder().withSpecular(),
        modelVertex, modelSpecularMapPixel);

    addPipeline(
        Key::Builder().withTangents().withSpecular(),
        modelNormalMapVertex, modelNormalSpecularMapPixel);


    addPipeline(
        Key::Builder().withTranslucent(),
        modelVertex, modelTranslucentPixel);
    // FIXME Ignore lightmap for translucents meshpart
    addPipeline(
        Key::Builder().withTranslucent().withLightmap(),
        modelVertex, modelTranslucentPixel);

    addPipeline(
        Key::Builder().withTangents().withTranslucent(),
        modelNormalMapVertex, modelTranslucentPixel);

    addPipeline(
        Key::Builder().withSpecular().withTranslucent(),
        modelVertex, modelTranslucentPixel);

    addPipeline(
        Key::Builder().withTangents().withSpecular().withTranslucent(),
        modelNormalMapVertex, modelTranslucentPixel);


    addPipeline(
        Key::Builder().withLightmap(),
        modelLightmapVertex, modelLightmapPixel);

    addPipeline(
        Key::Builder().withLightmap().withTangents(),
        modelLightmapNormalMapVertex, modelLightmapNormalMapPixel);

    addPipeline(
        Key::Builder().withLightmap().withSpecular(),
        modelLightmapVertex, modelLightmapSpecularMapPixel);

    addPipeline(
        Key::Builder().withLightmap().withTangents().withSpecular(),
        modelLightmapNormalMapVertex, modelLightmapNormalSpecularMapPixel);


    addPipeline(
        Key::Builder().withSkinned(),
        skinModelVertex, modelPixel);

    addPipeline(
        Key::Builder().withSkinned().withTangents(),
        skinModelNormalMapVertex, modelNormalMapPixel);

    addPipeline(
        Key::Builder().withSkinned().withSpecular(),
        skinModelVertex, modelSpecularMapPixel);

    addPipeline(
        Key::Builder().withSkinned().withTangents().withSpecular(),
        skinModelNormalMapVertex, modelNormalSpecularMapPixel);


    addPipeline(
        Key::Builder().withSkinned().withTranslucent(),
        skinModelVertex, modelTranslucentPixel);

    addPipeline(
        Key::Builder().withSkinned().withTangents().withTranslucent(),
        skinModelNormalMapVertex, modelTranslucentPixel);

    addPipeline(
        Key::Builder().withSkinned().withSpecular().withTranslucent(),
        skinModelVertex, modelTranslucentPixel);

    addPipeline(
        Key::Builder().withSkinned().withTangents().withSpecular().withTranslucent(),
        skinModelNormalMapVertex, modelTranslucentPixel);


    addPipeline(
        Key::Builder().withDepthOnly(),
        modelShadowVertex, modelShadowPixel);


    addPipeline(
        Key::Builder().withSkinned().withDepthOnly(),
        skinModelShadowVertex, modelShadowPixel);
}

