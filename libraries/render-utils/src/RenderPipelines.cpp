
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

#include <functional>

#include <gpu/Context.h>
#include <gpu/StandardShaderLib.h>

#include "StencilMaskPass.h"
#include "DeferredLightingEffect.h"
#include "TextureCache.h"
#include "render/DrawTask.h"

#include "model_vert.h"
#include "model_normal_map_vert.h"
#include "model_lightmap_vert.h"
#include "model_lightmap_normal_map_vert.h"
#include "skin_model_vert.h"
#include "skin_model_normal_map_vert.h"
#include "skin_model_dq_vert.h"
#include "skin_model_normal_map_dq_vert.h"

#include "model_lightmap_fade_vert.h"
#include "model_lightmap_normal_map_fade_vert.h"
#include "model_translucent_vert.h"
#include "model_translucent_normal_map_vert.h"
#include "skin_model_fade_vert.h"
#include "skin_model_normal_map_fade_vert.h"
#include "skin_model_fade_dq_vert.h"
#include "skin_model_normal_map_fade_dq_vert.h"

#include "simple_vert.h"
#include "simple_textured_frag.h"
#include "simple_textured_unlit_frag.h"
#include "simple_transparent_textured_frag.h"
#include "simple_transparent_textured_unlit_frag.h"

#include "simple_fade_vert.h"
#include "simple_textured_fade_frag.h"
#include "simple_textured_unlit_fade_frag.h"
#include "simple_transparent_textured_fade_frag.h"
#include "simple_transparent_textured_unlit_fade_frag.h"

#include "model_frag.h"
#include "model_unlit_frag.h"
#include "model_normal_map_frag.h"
#include "model_fade_vert.h"
#include "model_normal_map_fade_vert.h"

#include "model_fade_frag.h"
#include "model_unlit_fade_frag.h"
#include "model_normal_map_fade_frag.h"

#include "forward_model_frag.h"
#include "forward_model_unlit_frag.h"
#include "forward_model_normal_map_frag.h"
#include "forward_model_translucent_frag.h"

#include "model_lightmap_frag.h"
#include "model_lightmap_normal_map_frag.h"
#include "model_translucent_frag.h"
#include "model_translucent_unlit_frag.h"
#include "model_translucent_normal_map_frag.h"

#include "model_lightmap_fade_frag.h"
#include "model_lightmap_normal_map_fade_frag.h"
#include "model_translucent_fade_frag.h"
#include "model_translucent_unlit_fade_frag.h"
#include "model_translucent_normal_map_fade_frag.h"

#include "overlay3D_vert.h"
#include "overlay3D_frag.h"
#include "overlay3D_model_frag.h"
#include "overlay3D_model_translucent_frag.h"
#include "overlay3D_translucent_frag.h"
#include "overlay3D_unlit_frag.h"
#include "overlay3D_translucent_unlit_frag.h"
#include "overlay3D_model_unlit_frag.h"
#include "overlay3D_model_translucent_unlit_frag.h"

#include "model_shadow_vert.h"
#include "skin_model_shadow_vert.h"
#include "skin_model_shadow_dq_vert.h"
#include "skin_model_shadow_fade_dq_vert.h"

#include "model_shadow_frag.h"
#include "skin_model_shadow_frag.h"

#include "model_shadow_fade_vert.h"
#include "skin_model_shadow_fade_vert.h"

#include "model_shadow_fade_frag.h"
#include "skin_model_shadow_fade_frag.h"

using namespace render;
using namespace std::placeholders;

void initOverlay3DPipelines(ShapePlumber& plumber, bool depthTest = false);
void initDeferredPipelines(ShapePlumber& plumber, const render::ShapePipeline::BatchSetter& batchSetter, const render::ShapePipeline::ItemSetter& itemSetter);
void initForwardPipelines(ShapePlumber& plumber, const render::ShapePipeline::BatchSetter& batchSetter, const render::ShapePipeline::ItemSetter& itemSetter);
void initZPassPipelines(ShapePlumber& plumber, gpu::StatePointer state);

void addPlumberPipeline(ShapePlumber& plumber,
        const ShapeKey& key, const gpu::ShaderPointer& vertex, const gpu::ShaderPointer& pixel,
        const render::ShapePipeline::BatchSetter& batchSetter, const render::ShapePipeline::ItemSetter& itemSetter);

void batchSetter(const ShapePipeline& pipeline, gpu::Batch& batch, RenderArgs* args);
void lightBatchSetter(const ShapePipeline& pipeline, gpu::Batch& batch, RenderArgs* args);
static bool forceLightBatchSetter{ false };

void initOverlay3DPipelines(ShapePlumber& plumber, bool depthTest) {
    auto vertex = overlay3D_vert::getShader();
    auto vertexModel = model_vert::getShader();
    auto pixel = overlay3D_frag::getShader();
    auto pixelTranslucent = overlay3D_translucent_frag::getShader();
    auto pixelUnlit = overlay3D_unlit_frag::getShader();
    auto pixelTranslucentUnlit = overlay3D_translucent_unlit_frag::getShader();
    auto pixelModel = overlay3D_model_frag::getShader();
    auto pixelModelTranslucent = overlay3D_model_translucent_frag::getShader();
    auto pixelModelUnlit = overlay3D_model_unlit_frag::getShader();
    auto pixelModelTranslucentUnlit = overlay3D_model_translucent_unlit_frag::getShader();

    auto opaqueProgram = gpu::Shader::createProgram(vertex, pixel);
    auto translucentProgram = gpu::Shader::createProgram(vertex, pixelTranslucent);
    auto unlitOpaqueProgram = gpu::Shader::createProgram(vertex, pixelUnlit);
    auto unlitTranslucentProgram = gpu::Shader::createProgram(vertex, pixelTranslucentUnlit);
    auto materialOpaqueProgram = gpu::Shader::createProgram(vertexModel, pixelModel);
    auto materialTranslucentProgram = gpu::Shader::createProgram(vertexModel, pixelModelTranslucent);
    auto materialUnlitOpaqueProgram = gpu::Shader::createProgram(vertexModel, pixelModel);
    auto materialUnlitTranslucentProgram = gpu::Shader::createProgram(vertexModel, pixelModelTranslucent);

    for (int i = 0; i < 8; i++) {
        bool isCulled = (i & 1);
        bool isBiased = (i & 2);
        bool isOpaque = (i & 4);

        auto state = std::make_shared<gpu::State>();
        if (depthTest) {
            state->setDepthTest(true, true, gpu::LESS_EQUAL);
        } else {
            state->setDepthTest(false);
        }
        state->setCullMode(isCulled ? gpu::State::CULL_BACK : gpu::State::CULL_NONE);
        if (isBiased) {
            state->setDepthBias(1.0f);
            state->setDepthBiasSlopeScale(1.0f);
        }
        if (isOpaque) {
            // Soft edges
            state->setBlendFunction(true,
                gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA);
        } else {
            state->setBlendFunction(true,
                gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA,
                gpu::State::FACTOR_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::ONE);
        }

        ShapeKey::Filter::Builder builder;

        isCulled ? builder.withCullFace() : builder.withoutCullFace();
        isBiased ? builder.withDepthBias() : builder.withoutDepthBias();
        isOpaque ? builder.withOpaque() : builder.withTranslucent();

        auto simpleProgram = isOpaque ? opaqueProgram : translucentProgram;
        auto unlitProgram = isOpaque ? unlitOpaqueProgram : unlitTranslucentProgram;
        auto materialProgram = isOpaque ? materialOpaqueProgram : materialTranslucentProgram;
        auto materialUnlitProgram = isOpaque ? materialUnlitOpaqueProgram : materialUnlitTranslucentProgram;

        plumber.addPipeline(builder.withMaterial().build().key(), materialProgram, state, &lightBatchSetter);
        plumber.addPipeline(builder.withMaterial().withUnlit().build().key(), materialUnlitProgram, state, &batchSetter);
        plumber.addPipeline(builder.withoutUnlit().withoutMaterial().build().key(), simpleProgram, state, &lightBatchSetter);
        plumber.addPipeline(builder.withUnlit().withoutMaterial().build().key(), unlitProgram, state, &batchSetter);
    }
}

void initDeferredPipelines(render::ShapePlumber& plumber, const render::ShapePipeline::BatchSetter& batchSetter, const render::ShapePipeline::ItemSetter& itemSetter) {
    // Vertex shaders
    auto simpleVertex = simple_vert::getShader();
    auto modelVertex = model_vert::getShader();
    auto modelNormalMapVertex = model_normal_map_vert::getShader();
    auto modelLightmapVertex = model_lightmap_vert::getShader();
    auto modelLightmapNormalMapVertex = model_lightmap_normal_map_vert::getShader();
    auto modelTranslucentVertex = model_translucent_vert::getShader();
    auto modelTranslucentNormalMapVertex = model_translucent_normal_map_vert::getShader();
    auto modelShadowVertex = model_shadow_vert::getShader();

    auto modelLightmapFadeVertex = model_lightmap_fade_vert::getShader();
    auto modelLightmapNormalMapFadeVertex = model_lightmap_normal_map_fade_vert::getShader();

    // matrix palette skinned
    auto skinModelVertex = skin_model_vert::getShader();
    auto skinModelNormalMapVertex = skin_model_normal_map_vert::getShader();
    auto skinModelShadowVertex = skin_model_shadow_vert::getShader();
    auto skinModelFadeVertex = skin_model_fade_vert::getShader();
    auto skinModelNormalMapFadeVertex = skin_model_normal_map_fade_vert::getShader();
    auto skinModelTranslucentVertex = skinModelFadeVertex;  // We use the same because it ouputs world position per vertex
    auto skinModelNormalMapTranslucentVertex = skinModelNormalMapFadeVertex;  // We use the same because it ouputs world position per vertex

    // dual quaternion skinned
    auto skinModelDualQuatVertex = skin_model_dq_vert::getShader();
    auto skinModelNormalMapDualQuatVertex = skin_model_normal_map_dq_vert::getShader();
    auto skinModelShadowDualQuatVertex = skin_model_shadow_dq_vert::getShader();
    auto skinModelShadowFadeDualQuatVertex = skin_model_shadow_fade_dq_vert::getShader();
    auto skinModelFadeDualQuatVertex = skin_model_fade_dq_vert::getShader();
    auto skinModelNormalMapFadeDualQuatVertex = skin_model_normal_map_fade_dq_vert::getShader();
    auto skinModelTranslucentDualQuatVertex = skinModelFadeDualQuatVertex;  // We use the same because it ouputs world position per vertex
    auto skinModelNormalMapTranslucentDualQuatVertex = skinModelNormalMapFadeDualQuatVertex;  // We use the same because it ouputs world position per vertex

    auto modelFadeVertex = model_fade_vert::getShader();
    auto modelNormalMapFadeVertex = model_normal_map_fade_vert::getShader();
    auto simpleFadeVertex = simple_fade_vert::getShader();
    auto modelShadowFadeVertex = model_shadow_fade_vert::getShader();
    auto skinModelShadowFadeVertex = skin_model_shadow_fade_vert::getShader();

    // Pixel shaders
    auto simplePixel = simple_textured_frag::getShader();
    auto simpleUnlitPixel = simple_textured_unlit_frag::getShader();
    auto simpleTranslucentPixel = simple_transparent_textured_frag::getShader();
    auto simpleTranslucentUnlitPixel = simple_transparent_textured_unlit_frag::getShader();
    auto modelPixel = model_frag::getShader();
    auto modelUnlitPixel = model_unlit_frag::getShader();
    auto modelNormalMapPixel = model_normal_map_frag::getShader();
    auto modelTranslucentPixel = model_translucent_frag::getShader();
    auto modelTranslucentUnlitPixel = model_translucent_unlit_frag::getShader();
    auto modelTranslucentNormalMapPixel = model_translucent_normal_map_frag::getShader();
    auto modelShadowPixel = model_shadow_frag::getShader();
    auto modelLightmapPixel = model_lightmap_frag::getShader();
    auto modelLightmapNormalMapPixel = model_lightmap_normal_map_frag::getShader();
    auto modelLightmapFadePixel = model_lightmap_fade_frag::getShader();
    auto modelLightmapNormalMapFadePixel = model_lightmap_normal_map_fade_frag::getShader();

    auto modelFadePixel = model_fade_frag::getShader();
    auto modelUnlitFadePixel = model_unlit_fade_frag::getShader();
    auto modelNormalMapFadePixel = model_normal_map_fade_frag::getShader();
    auto modelShadowFadePixel = model_shadow_fade_frag::getShader();
    auto modelTranslucentFadePixel = model_translucent_fade_frag::getShader();
    auto modelTranslucentUnlitFadePixel = model_translucent_unlit_fade_frag::getShader();
    auto modelTranslucentNormalMapFadePixel = model_translucent_normal_map_fade_frag::getShader(); 
    auto simpleFadePixel = simple_textured_fade_frag::getShader();
    auto simpleUnlitFadePixel = simple_textured_unlit_fade_frag::getShader();
    auto simpleTranslucentFadePixel = simple_transparent_textured_fade_frag::getShader();
    auto simpleTranslucentUnlitFadePixel = simple_transparent_textured_unlit_fade_frag::getShader();

    using Key = render::ShapeKey;
    auto addPipeline = std::bind(&addPlumberPipeline, std::ref(plumber), _1, _2, _3, _4, _5);
    // TODO: Refactor this to use a filter
    // Opaques
    addPipeline(
        Key::Builder().withMaterial(),
        modelVertex, modelPixel, nullptr, nullptr);
    addPipeline(
        Key::Builder(),
        simpleVertex, simplePixel, nullptr, nullptr);
    addPipeline(
        Key::Builder().withMaterial().withUnlit(),
        modelVertex, modelUnlitPixel, nullptr, nullptr);
    addPipeline(
        Key::Builder().withUnlit(),
        simpleVertex, simpleUnlitPixel, nullptr, nullptr);
    addPipeline(
        Key::Builder().withMaterial().withTangents(),
        modelNormalMapVertex, modelNormalMapPixel, nullptr, nullptr);

    // Same thing but with Fade on
    addPipeline(
        Key::Builder().withMaterial().withFade(),
        modelFadeVertex, modelFadePixel, batchSetter, itemSetter);
    addPipeline(
        Key::Builder().withFade(),
        simpleFadeVertex, simpleFadePixel, batchSetter, itemSetter);
    addPipeline(
        Key::Builder().withMaterial().withUnlit().withFade(),
        modelFadeVertex, modelUnlitFadePixel, batchSetter, itemSetter);
    addPipeline(
        Key::Builder().withUnlit().withFade(),
        simpleFadeVertex, simpleUnlitFadePixel, batchSetter, itemSetter);
    addPipeline(
        Key::Builder().withMaterial().withTangents().withFade(),
        modelNormalMapFadeVertex, modelNormalMapFadePixel, batchSetter, itemSetter);

    // Translucents
    addPipeline(
        Key::Builder().withMaterial().withTranslucent(),
        modelTranslucentVertex, modelTranslucentPixel, nullptr, nullptr);
    addPipeline(
        Key::Builder().withTranslucent(),
        simpleVertex, simpleTranslucentPixel, nullptr, nullptr);
    addPipeline(
        Key::Builder().withMaterial().withTranslucent().withUnlit(),
        modelVertex, modelTranslucentUnlitPixel, nullptr, nullptr);
    addPipeline(
        Key::Builder().withTranslucent().withUnlit(),
        simpleVertex, simpleTranslucentUnlitPixel, nullptr, nullptr);
    addPipeline(
        Key::Builder().withMaterial().withTranslucent().withTangents(),
        modelTranslucentNormalMapVertex, modelTranslucentNormalMapPixel, nullptr, nullptr);
    addPipeline(
        // FIXME: Ignore lightmap for translucents meshpart
        Key::Builder().withMaterial().withTranslucent().withLightmap(),
        modelTranslucentVertex, modelTranslucentPixel, nullptr, nullptr);
    // Same thing but with Fade on
    addPipeline(
        Key::Builder().withMaterial().withTranslucent().withFade(),
        modelTranslucentVertex, modelTranslucentFadePixel, batchSetter, itemSetter);
    addPipeline(
        Key::Builder().withTranslucent().withFade(),
        simpleFadeVertex, simpleTranslucentFadePixel, batchSetter, itemSetter);
    addPipeline(
        Key::Builder().withMaterial().withTranslucent().withUnlit().withFade(),
        modelFadeVertex, modelTranslucentUnlitFadePixel, batchSetter, itemSetter);
    addPipeline(
        Key::Builder().withTranslucent().withUnlit().withFade(),
        simpleFadeVertex, simpleTranslucentUnlitFadePixel, batchSetter, itemSetter);
    addPipeline(
        Key::Builder().withMaterial().withTranslucent().withTangents().withFade(),
        modelTranslucentNormalMapVertex, modelTranslucentNormalMapFadePixel, batchSetter, itemSetter);
    addPipeline(
        // FIXME: Ignore lightmap for translucents meshpart
        Key::Builder().withMaterial().withTranslucent().withLightmap().withFade(),
        modelFadeVertex, modelTranslucentFadePixel, batchSetter, itemSetter);

    // Lightmapped
    addPipeline(
        Key::Builder().withMaterial().withLightmap(),
        modelLightmapVertex, modelLightmapPixel, nullptr, nullptr);
    addPipeline(
        Key::Builder().withMaterial().withLightmap().withTangents(),
        modelLightmapNormalMapVertex, modelLightmapNormalMapPixel, nullptr, nullptr);
    // Same thing but with Fade on
    addPipeline(
        Key::Builder().withMaterial().withLightmap().withFade(),
        modelLightmapFadeVertex, modelLightmapFadePixel, batchSetter, itemSetter);
    addPipeline(
        Key::Builder().withMaterial().withLightmap().withTangents().withFade(),
        modelLightmapNormalMapFadeVertex, modelLightmapNormalMapFadePixel, batchSetter, itemSetter);

    // matrix palette skinned
    addPipeline(
        Key::Builder().withMaterial().withSkinned(),
        skinModelVertex, modelPixel, nullptr, nullptr);
    addPipeline(
        Key::Builder().withMaterial().withSkinned().withTangents(),
        skinModelNormalMapVertex, modelNormalMapPixel, nullptr, nullptr);
    // Same thing but with Fade on
    addPipeline(
        Key::Builder().withMaterial().withSkinned().withFade(),
        skinModelFadeVertex, modelFadePixel, batchSetter, itemSetter);
    addPipeline(
        Key::Builder().withMaterial().withSkinned().withTangents().withFade(),
        skinModelNormalMapFadeVertex, modelNormalMapFadePixel, batchSetter, itemSetter);

    // matrix palette skinned and translucent
    addPipeline(
        Key::Builder().withMaterial().withSkinned().withTranslucent(),
        skinModelTranslucentVertex, modelTranslucentPixel, nullptr, nullptr);
    addPipeline(
        Key::Builder().withMaterial().withSkinned().withTranslucent().withTangents(),
        skinModelNormalMapTranslucentVertex, modelTranslucentNormalMapPixel, nullptr, nullptr);
    // Same thing but with Fade on
    addPipeline(
        Key::Builder().withMaterial().withSkinned().withTranslucent().withFade(),
        skinModelFadeVertex, modelTranslucentFadePixel, batchSetter, itemSetter);
    addPipeline(
        Key::Builder().withMaterial().withSkinned().withTranslucent().withTangents().withFade(),
        skinModelNormalMapFadeVertex, modelTranslucentNormalMapFadePixel, batchSetter, itemSetter);

    // dual quaternion skinned
    addPipeline(
        Key::Builder().withMaterial().withSkinned().withDualQuatSkinned(),
        skinModelDualQuatVertex, modelPixel, nullptr, nullptr);
    addPipeline(
        Key::Builder().withMaterial().withSkinned().withDualQuatSkinned().withTangents(),
        skinModelNormalMapDualQuatVertex, modelNormalMapPixel, nullptr, nullptr);
    // Same thing but with Fade on
    addPipeline(
        Key::Builder().withMaterial().withSkinned().withDualQuatSkinned().withFade(),
        skinModelFadeDualQuatVertex, modelFadePixel, batchSetter, itemSetter);
    addPipeline(
        Key::Builder().withMaterial().withSkinned().withDualQuatSkinned().withTangents().withFade(),
        skinModelNormalMapFadeDualQuatVertex, modelNormalMapFadePixel, batchSetter, itemSetter);

    // dual quaternion skinned and translucent
    addPipeline(
        Key::Builder().withMaterial().withSkinned().withDualQuatSkinned().withTranslucent(),
        skinModelTranslucentDualQuatVertex, modelTranslucentPixel, nullptr, nullptr);
    addPipeline(
        Key::Builder().withMaterial().withSkinned().withDualQuatSkinned().withTranslucent().withTangents(),
        skinModelNormalMapTranslucentDualQuatVertex, modelTranslucentNormalMapPixel, nullptr, nullptr);
    // Same thing but with Fade on
    addPipeline(
        Key::Builder().withMaterial().withSkinned().withDualQuatSkinned().withTranslucent().withFade(),
        skinModelFadeDualQuatVertex, modelTranslucentFadePixel, batchSetter, itemSetter);
    addPipeline(
        Key::Builder().withMaterial().withSkinned().withDualQuatSkinned().withTranslucent().withTangents().withFade(),
        skinModelNormalMapFadeDualQuatVertex, modelTranslucentNormalMapFadePixel, batchSetter, itemSetter);

    // Depth-only
    addPipeline(
        Key::Builder().withDepthOnly(),
        modelShadowVertex, modelShadowPixel, nullptr, nullptr);
    addPipeline(
        Key::Builder().withSkinned().withDepthOnly(),
        skinModelShadowVertex, modelShadowPixel, nullptr, nullptr);
    // Same thing but with Fade on
    addPipeline(
        Key::Builder().withDepthOnly().withFade(),
        modelShadowFadeVertex, modelShadowFadePixel, batchSetter, itemSetter);
    addPipeline(
        Key::Builder().withSkinned().withDepthOnly().withFade(),
        skinModelShadowFadeVertex, modelShadowFadePixel, batchSetter, itemSetter);

    // Now repeat for dual quaternion
    // Depth-only
    addPipeline(
        Key::Builder().withSkinned().withDualQuatSkinned().withDepthOnly(),
        skinModelShadowDualQuatVertex, modelShadowPixel, nullptr, nullptr);
    // Same thing but with Fade on
    addPipeline(
        Key::Builder().withSkinned().withDualQuatSkinned().withDepthOnly().withFade(),
        skinModelShadowFadeDualQuatVertex, modelShadowFadePixel, batchSetter, itemSetter);
}

void initForwardPipelines(ShapePlumber& plumber, const render::ShapePipeline::BatchSetter& batchSetter, const render::ShapePipeline::ItemSetter& itemSetter) {
    // Vertex shaders
    auto modelVertex = model_vert::getShader();
    auto modelNormalMapVertex = model_normal_map_vert::getShader();
    auto skinModelVertex = skin_model_vert::getShader();
    auto skinModelNormalMapVertex = skin_model_normal_map_vert::getShader();

    auto skinModelDualQuatVertex = skin_model_dq_vert::getShader();
    auto skinModelNormalMapDualQuatVertex = skin_model_normal_map_dq_vert::getShader();

    // Pixel shaders
    auto modelPixel = forward_model_frag::getShader();
    auto modelUnlitPixel = forward_model_unlit_frag::getShader();
    auto modelNormalMapPixel = forward_model_normal_map_frag::getShader();
    auto modelTranslucentPixel = forward_model_translucent_frag::getShader();

    using Key = render::ShapeKey;
    auto addPipelineBind = std::bind(&addPlumberPipeline, std::ref(plumber), _1, _2, _3, _4, _5);

    // Disable fade on the forward pipeline, all shaders get added twice, once with the fade key and once without
    auto addPipeline = [&](const ShapeKey& key, const gpu::ShaderPointer& vertex, const gpu::ShaderPointer& pixel) {
        addPipelineBind(key, vertex, pixel, nullptr, nullptr);
        addPipelineBind(Key::Builder(key).withFade(), vertex, pixel, nullptr, nullptr);
    };

    // Forward pipelines need the lightBatchSetter for opaques and transparents
  //  forceLightBatchSetter = true;
    forceLightBatchSetter = false;

    // Opaques
    addPipeline(Key::Builder().withMaterial(), modelVertex, modelPixel);
    addPipeline(Key::Builder().withMaterial().withUnlit(), modelVertex, modelUnlitPixel);
    addPipeline(Key::Builder().withMaterial().withTangents(), modelNormalMapVertex, modelNormalMapPixel);
 
    // Skinned Opaques
    addPipeline(Key::Builder().withMaterial().withSkinned(), skinModelVertex, modelPixel);
    addPipeline(Key::Builder().withMaterial().withSkinned().withTangents(), skinModelNormalMapVertex, modelNormalMapPixel);
    addPipeline(Key::Builder().withMaterial().withSkinned().withDualQuatSkinned(), skinModelDualQuatVertex, modelPixel);
    addPipeline(Key::Builder().withMaterial().withSkinned().withTangents().withDualQuatSkinned(), skinModelNormalMapDualQuatVertex, modelNormalMapPixel);

    // Translucents
    addPipeline(Key::Builder().withMaterial().withTranslucent(), modelVertex, modelTranslucentPixel);
    addPipeline(Key::Builder().withMaterial().withTranslucent().withTangents(), modelNormalMapVertex, modelTranslucentPixel);

    // Skinned Translucents
    addPipeline(Key::Builder().withMaterial().withSkinned().withTranslucent(), skinModelVertex, modelTranslucentPixel);
    addPipeline(Key::Builder().withMaterial().withSkinned().withTranslucent().withTangents(), skinModelNormalMapVertex, modelTranslucentPixel);
    addPipeline(Key::Builder().withMaterial().withSkinned().withTranslucent().withDualQuatSkinned(), skinModelDualQuatVertex, modelTranslucentPixel);
    addPipeline(Key::Builder().withMaterial().withSkinned().withTranslucent().withTangents().withDualQuatSkinned(), skinModelNormalMapDualQuatVertex, modelTranslucentPixel);

    forceLightBatchSetter = false;
}

void addPlumberPipeline(ShapePlumber& plumber,
        const ShapeKey& key, const gpu::ShaderPointer& vertex, const gpu::ShaderPointer& pixel,
        const render::ShapePipeline::BatchSetter& extraBatchSetter, const render::ShapePipeline::ItemSetter& itemSetter) {
    // These key-values' pipelines are added by this functor in addition to the key passed
    assert(!key.isWireframe());
    assert(!key.isDepthBiased());
    assert(key.isCullFace());

    gpu::ShaderPointer program = gpu::Shader::createProgram(vertex, pixel);

    for (int i = 0; i < 8; i++) {
        bool isCulled = (i & 1);
        bool isBiased = (i & 2);
        bool isWireframed = (i & 4);

        auto state = std::make_shared<gpu::State>();
        PrepareStencil::testMaskDrawShape(*state);

        // Depth test depends on transparency
        state->setDepthTest(true, !key.isTranslucent(), gpu::LESS_EQUAL);
        state->setBlendFunction(key.isTranslucent(),
                gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA,
                gpu::State::FACTOR_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::ONE);

        ShapeKey::Builder builder(key);
        if (!isCulled) {
            builder.withoutCullFace();
        }
        state->setCullMode(isCulled ? gpu::State::CULL_BACK : gpu::State::CULL_NONE);
        if (isWireframed) {
            builder.withWireframe();
            state->setFillMode(gpu::State::FILL_LINE);
        }
        if (isBiased) {
            builder.withDepthBias();
            state->setDepthBias(1.0f);
            state->setDepthBiasSlopeScale(1.0f);
        }

        auto baseBatchSetter = (forceLightBatchSetter || key.isTranslucent()) ? &lightBatchSetter : &batchSetter;
        render::ShapePipeline::BatchSetter finalBatchSetter;
        if (extraBatchSetter) {
            finalBatchSetter = [baseBatchSetter, extraBatchSetter](const ShapePipeline& pipeline, gpu::Batch& batch, render::Args* args) {
                baseBatchSetter(pipeline, batch, args);
                extraBatchSetter(pipeline, batch, args);
            };
        }
        else {
            finalBatchSetter = baseBatchSetter;
        }
        plumber.addPipeline(builder.build(), program, state, finalBatchSetter, itemSetter);
    }
}

void batchSetter(const ShapePipeline& pipeline, gpu::Batch& batch, RenderArgs* args) {
    // Set a default albedo map
    batch.setResourceTexture(render::ShapePipeline::Slot::MAP::ALBEDO,
        DependencyManager::get<TextureCache>()->getWhiteTexture());

    // Set a default material
    if (pipeline.locations->materialBufferUnit >= 0) {
        // Create a default schema
        static bool isMaterialSet = false;
        static graphics::Material material;
        if (!isMaterialSet) {
            material.setAlbedo(vec3(1.0f));
            material.setOpacity(1.0f);
            material.setMetallic(0.1f);
            material.setRoughness(0.9f);
            isMaterialSet = true;
        }

        // Set a default schema
        batch.setUniformBuffer(ShapePipeline::Slot::BUFFER::MATERIAL, material.getSchemaBuffer());
    }
}

void lightBatchSetter(const ShapePipeline& pipeline, gpu::Batch& batch, RenderArgs* args) {
    // Set the batch
    batchSetter(pipeline, batch, args);

    // Set the light
    if (pipeline.locations->keyLightBufferUnit >= 0) {
        DependencyManager::get<DeferredLightingEffect>()->setupKeyLightBatch(args, batch,
            pipeline.locations->keyLightBufferUnit,
            pipeline.locations->lightAmbientBufferUnit,
            pipeline.locations->lightAmbientMapUnit);
    }
}

void initZPassPipelines(ShapePlumber& shapePlumber, gpu::StatePointer state) {
    auto modelVertex = model_shadow_vert::getShader();
    auto modelPixel = model_shadow_frag::getShader();
    gpu::ShaderPointer modelProgram = gpu::Shader::createProgram(modelVertex, modelPixel);
    shapePlumber.addPipeline(
        ShapeKey::Filter::Builder().withoutSkinned().withoutFade(),
        modelProgram, state);

    auto skinVertex = skin_model_shadow_vert::getShader();
    auto skinPixel = skin_model_shadow_frag::getShader();
    gpu::ShaderPointer skinProgram = gpu::Shader::createProgram(skinVertex, skinPixel);
    shapePlumber.addPipeline(
        ShapeKey::Filter::Builder().withSkinned().withoutDualQuatSkinned().withoutFade(),
        skinProgram, state);

    auto modelFadeVertex = model_shadow_fade_vert::getShader();
    auto modelFadePixel = model_shadow_fade_frag::getShader();
    gpu::ShaderPointer modelFadeProgram = gpu::Shader::createProgram(modelFadeVertex, modelFadePixel);
    shapePlumber.addPipeline(
        ShapeKey::Filter::Builder().withoutSkinned().withFade(),
        modelFadeProgram, state);

    auto skinFadeVertex = skin_model_shadow_fade_vert::getShader();
    auto skinFadePixel = skin_model_shadow_fade_frag::getShader();
    gpu::ShaderPointer skinFadeProgram = gpu::Shader::createProgram(skinFadeVertex, skinFadePixel);
    shapePlumber.addPipeline(
        ShapeKey::Filter::Builder().withSkinned().withoutDualQuatSkinned().withFade(),
        skinFadeProgram, state);

    //Added for dual quaternions
    auto skinModelShadowDualQuatVertex = skin_model_shadow_dq_vert::getShader();
    gpu::ShaderPointer skinModelShadowDualQuatProgram = gpu::Shader::createProgram(skinModelShadowDualQuatVertex, skinPixel);
    shapePlumber.addPipeline(
        ShapeKey::Filter::Builder().withSkinned().withDualQuatSkinned().withoutFade(),
        skinModelShadowDualQuatProgram, state);

    auto skinModelShadowFadeDualQuatVertex = skin_model_shadow_fade_dq_vert::getShader();
    gpu::ShaderPointer skinModelShadowFadeDualQuatProgram = gpu::Shader::createProgram(skinModelShadowFadeDualQuatVertex, skinFadePixel);
    shapePlumber.addPipeline(
        ShapeKey::Filter::Builder().withSkinned().withDualQuatSkinned().withFade(),
        skinModelShadowFadeDualQuatProgram, state);
}

#include "RenderPipelines.h"
#include <model-networking/TextureCache.h>

// FIXME find a better way to setup the default textures
void RenderPipelines::bindMaterial(const graphics::MaterialPointer& material, gpu::Batch& batch, bool enableTextures) {
    if (!material) {
        return;
    }

    auto textureCache = DependencyManager::get<TextureCache>();

    batch.setUniformBuffer(ShapePipeline::Slot::BUFFER::MATERIAL, material->getSchemaBuffer());
    batch.setUniformBuffer(ShapePipeline::Slot::BUFFER::TEXMAPARRAY, material->getTexMapArrayBuffer());

    const auto& materialKey = material->getKey();
    const auto& textureMaps = material->getTextureMaps();

    int numUnlit = 0;
    if (materialKey.isUnlit()) {
        numUnlit++;
    }

    const auto& drawMaterialTextures = material->getTextureTable();

    // Albedo
    if (materialKey.isAlbedoMap()) {
        auto itr = textureMaps.find(graphics::MaterialKey::ALBEDO_MAP);
        if (enableTextures && itr != textureMaps.end() && itr->second->isDefined()) {
            drawMaterialTextures->setTexture(ShapePipeline::Slot::ALBEDO, itr->second->getTextureView());
        } else {
            drawMaterialTextures->setTexture(ShapePipeline::Slot::ALBEDO, textureCache->getWhiteTexture());
        }
    }

    // Roughness map
    if (materialKey.isRoughnessMap()) {
        auto itr = textureMaps.find(graphics::MaterialKey::ROUGHNESS_MAP);
        if (enableTextures && itr != textureMaps.end() && itr->second->isDefined()) {
            drawMaterialTextures->setTexture(ShapePipeline::Slot::ROUGHNESS, itr->second->getTextureView());
        } else {
            drawMaterialTextures->setTexture(ShapePipeline::Slot::ROUGHNESS, textureCache->getWhiteTexture());
        }
    }

    // Normal map
    if (materialKey.isNormalMap()) {
        auto itr = textureMaps.find(graphics::MaterialKey::NORMAL_MAP);
        if (enableTextures && itr != textureMaps.end() && itr->second->isDefined()) {
            drawMaterialTextures->setTexture(ShapePipeline::Slot::NORMAL, itr->second->getTextureView());
        } else {
            drawMaterialTextures->setTexture(ShapePipeline::Slot::NORMAL, textureCache->getBlueTexture());
        }
    }

    // Metallic map
    if (materialKey.isMetallicMap()) {
        auto itr = textureMaps.find(graphics::MaterialKey::METALLIC_MAP);
        if (enableTextures && itr != textureMaps.end() && itr->second->isDefined()) {
            drawMaterialTextures->setTexture(ShapePipeline::Slot::METALLIC, itr->second->getTextureView());
        } else {
            drawMaterialTextures->setTexture(ShapePipeline::Slot::METALLIC, textureCache->getBlackTexture());
        }
    }

    // Occlusion map
    if (materialKey.isOcclusionMap()) {
        auto itr = textureMaps.find(graphics::MaterialKey::OCCLUSION_MAP);
        if (enableTextures && itr != textureMaps.end() && itr->second->isDefined()) {
            drawMaterialTextures->setTexture(ShapePipeline::Slot::OCCLUSION, itr->second->getTextureView());
        } else {
            drawMaterialTextures->setTexture(ShapePipeline::Slot::OCCLUSION, textureCache->getWhiteTexture());
        }
    }

    // Scattering map
    if (materialKey.isScatteringMap()) {
        auto itr = textureMaps.find(graphics::MaterialKey::SCATTERING_MAP);
        if (enableTextures && itr != textureMaps.end() && itr->second->isDefined()) {
            drawMaterialTextures->setTexture(ShapePipeline::Slot::SCATTERING, itr->second->getTextureView());
        } else {
            drawMaterialTextures->setTexture(ShapePipeline::Slot::SCATTERING, textureCache->getWhiteTexture());
        }
    }

    // Emissive / Lightmap
    if (materialKey.isLightmapMap()) {
        auto itr = textureMaps.find(graphics::MaterialKey::LIGHTMAP_MAP);

        if (enableTextures && itr != textureMaps.end() && itr->second->isDefined()) {
            drawMaterialTextures->setTexture(ShapePipeline::Slot::EMISSIVE_LIGHTMAP, itr->second->getTextureView());
        } else {
            drawMaterialTextures->setTexture(ShapePipeline::Slot::EMISSIVE_LIGHTMAP, textureCache->getGrayTexture());
        }
    } else if (materialKey.isEmissiveMap()) {
        auto itr = textureMaps.find(graphics::MaterialKey::EMISSIVE_MAP);
        if (enableTextures && itr != textureMaps.end() && itr->second->isDefined()) {
            drawMaterialTextures->setTexture(ShapePipeline::Slot::EMISSIVE_LIGHTMAP, itr->second->getTextureView());
        } else {
            drawMaterialTextures->setTexture(ShapePipeline::Slot::EMISSIVE_LIGHTMAP, textureCache->getBlackTexture());
        }
    }

    batch.setResourceTextureTable(material->getTextureTable());
}
