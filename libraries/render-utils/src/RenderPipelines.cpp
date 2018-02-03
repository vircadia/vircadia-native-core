
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

#include "model_lightmap_fade_vert.h"
#include "model_lightmap_normal_map_fade_vert.h"
#include "model_translucent_vert.h"
#include "model_translucent_normal_map_vert.h"
#include "skin_model_fade_vert.h"
#include "skin_model_normal_map_fade_vert.h"

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
#include "model_normal_specular_map_frag.h"
#include "model_specular_map_frag.h"

#include "model_fade_vert.h"
#include "model_normal_map_fade_vert.h"

#include "model_fade_frag.h"
#include "model_unlit_fade_frag.h"
#include "model_normal_map_fade_frag.h"
#include "model_normal_specular_map_fade_frag.h"
#include "model_specular_map_fade_frag.h"

#include "forward_model_frag.h"
#include "forward_model_unlit_frag.h"
#include "forward_model_normal_map_frag.h"
#include "forward_model_normal_specular_map_frag.h"
#include "forward_model_specular_map_frag.h"

#include "model_lightmap_frag.h"
#include "model_lightmap_normal_map_frag.h"
#include "model_lightmap_normal_specular_map_frag.h"
#include "model_lightmap_specular_map_frag.h"
#include "model_translucent_frag.h"
#include "model_translucent_unlit_frag.h"
#include "model_translucent_normal_map_frag.h"

#include "model_lightmap_fade_frag.h"
#include "model_lightmap_normal_map_fade_frag.h"
#include "model_lightmap_normal_specular_map_fade_frag.h"
#include "model_lightmap_specular_map_fade_frag.h"
#include "model_translucent_fade_frag.h"
#include "model_translucent_normal_map_fade_frag.h"
#include "model_translucent_unlit_fade_frag.h"

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

void initOverlay3DPipelines(ShapePlumber& plumber, bool depthTest) {
    auto vertex = gpu::Shader::createVertex(std::string(overlay3D_vert));
    auto vertexModel = gpu::Shader::createVertex(std::string(model_vert));
    auto pixel = gpu::Shader::createPixel(std::string(overlay3D_frag));
    auto pixelTranslucent = gpu::Shader::createPixel(std::string(overlay3D_translucent_frag));
    auto pixelUnlit = gpu::Shader::createPixel(std::string(overlay3D_unlit_frag));
    auto pixelTranslucentUnlit = gpu::Shader::createPixel(std::string(overlay3D_translucent_unlit_frag));
    auto pixelModel = gpu::Shader::createPixel(std::string(overlay3D_model_frag));
    auto pixelModelTranslucent = gpu::Shader::createPixel(std::string(overlay3D_model_translucent_frag));
    auto pixelModelUnlit = gpu::Shader::createPixel(std::string(overlay3D_model_unlit_frag));
    auto pixelModelTranslucentUnlit = gpu::Shader::createPixel(std::string(overlay3D_model_translucent_unlit_frag));

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
    auto simpleVertex = gpu::Shader::createVertex(std::string(simple_vert));
    auto modelVertex = gpu::Shader::createVertex(std::string(model_vert));
    auto modelNormalMapVertex = gpu::Shader::createVertex(std::string(model_normal_map_vert));
    auto modelLightmapVertex = gpu::Shader::createVertex(std::string(model_lightmap_vert));
    auto modelLightmapNormalMapVertex = gpu::Shader::createVertex(std::string(model_lightmap_normal_map_vert));
    auto modelTranslucentVertex = gpu::Shader::createVertex(std::string(model_translucent_vert));
    auto modelTranslucentNormalMapVertex = gpu::Shader::createVertex(std::string(model_translucent_normal_map_vert));
    auto modelShadowVertex = gpu::Shader::createVertex(std::string(model_shadow_vert));
    auto skinModelVertex = gpu::Shader::createVertex(std::string(skin_model_vert));
    auto skinModelNormalMapVertex = gpu::Shader::createVertex(std::string(skin_model_normal_map_vert));
    auto skinModelShadowVertex = gpu::Shader::createVertex(std::string(skin_model_shadow_vert));
    auto modelLightmapFadeVertex = gpu::Shader::createVertex(std::string(model_lightmap_fade_vert));
    auto modelLightmapNormalMapFadeVertex = gpu::Shader::createVertex(std::string(model_lightmap_normal_map_fade_vert));
    auto skinModelFadeVertex = gpu::Shader::createVertex(std::string(skin_model_fade_vert));
    auto skinModelNormalMapFadeVertex = gpu::Shader::createVertex(std::string(skin_model_normal_map_fade_vert));
    auto skinModelTranslucentVertex = skinModelFadeVertex;  // We use the same because it ouputs world position per vertex
    auto skinModelNormalMapTranslucentVertex = skinModelNormalMapFadeVertex;  // We use the same because it ouputs world position per vertex

    auto modelFadeVertex = gpu::Shader::createVertex(std::string(model_fade_vert));
    auto modelNormalMapFadeVertex = gpu::Shader::createVertex(std::string(model_normal_map_fade_vert));
    auto simpleFadeVertex = gpu::Shader::createVertex(std::string(simple_fade_vert));
    auto modelShadowFadeVertex = gpu::Shader::createVertex(std::string(model_shadow_fade_vert));
    auto skinModelShadowFadeVertex = gpu::Shader::createVertex(std::string(skin_model_shadow_fade_vert));

    // Pixel shaders
    auto simplePixel = gpu::Shader::createPixel(std::string(simple_textured_frag));
    auto simpleUnlitPixel = gpu::Shader::createPixel(std::string(simple_textured_unlit_frag));
    auto simpleTranslucentPixel = gpu::Shader::createPixel(std::string(simple_transparent_textured_frag));
    auto simpleTranslucentUnlitPixel = gpu::Shader::createPixel(std::string(simple_transparent_textured_unlit_frag));
    auto modelPixel = gpu::Shader::createPixel(std::string(model_frag));
    auto modelUnlitPixel = gpu::Shader::createPixel(std::string(model_unlit_frag));
    auto modelNormalMapPixel = gpu::Shader::createPixel(std::string(model_normal_map_frag));
    auto modelSpecularMapPixel = gpu::Shader::createPixel(std::string(model_specular_map_frag));
    auto modelNormalSpecularMapPixel = gpu::Shader::createPixel(std::string(model_normal_specular_map_frag));
    auto modelTranslucentPixel = gpu::Shader::createPixel(std::string(model_translucent_frag));
    auto modelTranslucentNormalMapPixel = gpu::Shader::createPixel(std::string(model_translucent_normal_map_frag));
    auto modelTranslucentUnlitPixel = gpu::Shader::createPixel(std::string(model_translucent_unlit_frag));
    auto modelShadowPixel = gpu::Shader::createPixel(std::string(model_shadow_frag));
    auto modelLightmapPixel = gpu::Shader::createPixel(std::string(model_lightmap_frag));
    auto modelLightmapNormalMapPixel = gpu::Shader::createPixel(std::string(model_lightmap_normal_map_frag));
    auto modelLightmapSpecularMapPixel = gpu::Shader::createPixel(std::string(model_lightmap_specular_map_frag));
    auto modelLightmapNormalSpecularMapPixel = gpu::Shader::createPixel(std::string(model_lightmap_normal_specular_map_frag));
    auto modelLightmapFadePixel = gpu::Shader::createPixel(std::string(model_lightmap_fade_frag));
    auto modelLightmapNormalMapFadePixel = gpu::Shader::createPixel(std::string(model_lightmap_normal_map_fade_frag));
    auto modelLightmapSpecularMapFadePixel = gpu::Shader::createPixel(std::string(model_lightmap_specular_map_fade_frag));
    auto modelLightmapNormalSpecularMapFadePixel = gpu::Shader::createPixel(std::string(model_lightmap_normal_specular_map_fade_frag));

    auto modelFadePixel = gpu::Shader::createPixel(std::string(model_fade_frag));
    auto modelUnlitFadePixel = gpu::Shader::createPixel(std::string(model_unlit_fade_frag));
    auto modelNormalMapFadePixel = gpu::Shader::createPixel(std::string(model_normal_map_fade_frag));
    auto modelSpecularMapFadePixel = gpu::Shader::createPixel(std::string(model_specular_map_fade_frag));
    auto modelNormalSpecularMapFadePixel = gpu::Shader::createPixel(std::string(model_normal_specular_map_fade_frag));
    auto modelShadowFadePixel = gpu::Shader::createPixel(std::string(model_shadow_fade_frag));
    auto modelTranslucentFadePixel = gpu::Shader::createPixel(std::string(model_translucent_fade_frag));
    auto modelTranslucentNormalMapFadePixel = gpu::Shader::createPixel(std::string(model_translucent_normal_map_fade_frag));
    auto modelTranslucentUnlitFadePixel = gpu::Shader::createPixel(std::string(model_translucent_unlit_fade_frag));
    auto simpleFadePixel = gpu::Shader::createPixel(std::string(simple_textured_fade_frag));
    auto simpleUnlitFadePixel = gpu::Shader::createPixel(std::string(simple_textured_unlit_fade_frag));
    auto simpleTranslucentFadePixel = gpu::Shader::createPixel(std::string(simple_transparent_textured_fade_frag));
    auto simpleTranslucentUnlitFadePixel = gpu::Shader::createPixel(std::string(simple_transparent_textured_unlit_fade_frag));

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
    addPipeline(
        Key::Builder().withMaterial().withSpecular(),
        modelVertex, modelSpecularMapPixel, nullptr, nullptr);
    addPipeline(
        Key::Builder().withMaterial().withTangents().withSpecular(),
        modelNormalMapVertex, modelNormalSpecularMapPixel, nullptr, nullptr);
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
    addPipeline(
        Key::Builder().withMaterial().withSpecular().withFade(),
        modelFadeVertex, modelSpecularMapFadePixel, batchSetter, itemSetter);
    addPipeline(
        Key::Builder().withMaterial().withTangents().withSpecular().withFade(),
        modelNormalMapFadeVertex, modelNormalSpecularMapFadePixel, batchSetter, itemSetter);

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
        Key::Builder().withMaterial().withTranslucent().withSpecular(),
        modelTranslucentVertex, modelTranslucentPixel, nullptr, nullptr);
    addPipeline(
        Key::Builder().withMaterial().withTranslucent().withTangents().withSpecular(),
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
        Key::Builder().withMaterial().withTranslucent().withSpecular().withFade(),
        modelFadeVertex, modelTranslucentFadePixel, batchSetter, itemSetter);
    addPipeline(
        Key::Builder().withMaterial().withTranslucent().withTangents().withSpecular().withFade(),
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
    addPipeline(
        Key::Builder().withMaterial().withLightmap().withSpecular(),
        modelLightmapVertex, modelLightmapSpecularMapPixel, nullptr, nullptr);
    addPipeline(
        Key::Builder().withMaterial().withLightmap().withTangents().withSpecular(),
        modelLightmapNormalMapVertex, modelLightmapNormalSpecularMapPixel, nullptr, nullptr);
    // Same thing but with Fade on
    addPipeline(
        Key::Builder().withMaterial().withLightmap().withFade(),
        modelLightmapFadeVertex, modelLightmapFadePixel, batchSetter, itemSetter);
    addPipeline(
        Key::Builder().withMaterial().withLightmap().withTangents().withFade(),
        modelLightmapNormalMapFadeVertex, modelLightmapNormalMapFadePixel, batchSetter, itemSetter);
    addPipeline(
        Key::Builder().withMaterial().withLightmap().withSpecular().withFade(),
        modelLightmapFadeVertex, modelLightmapSpecularMapFadePixel, batchSetter, itemSetter);
    addPipeline(
        Key::Builder().withMaterial().withLightmap().withTangents().withSpecular().withFade(),
        modelLightmapNormalMapFadeVertex, modelLightmapNormalSpecularMapFadePixel, batchSetter, itemSetter);

    // Skinned
    addPipeline(
        Key::Builder().withMaterial().withSkinned(),
        skinModelVertex, modelPixel, nullptr, nullptr);
    addPipeline(
        Key::Builder().withMaterial().withSkinned().withTangents(),
        skinModelNormalMapVertex, modelNormalMapPixel, nullptr, nullptr);
    addPipeline(
        Key::Builder().withMaterial().withSkinned().withSpecular(),
        skinModelVertex, modelSpecularMapPixel, nullptr, nullptr);
    addPipeline(
        Key::Builder().withMaterial().withSkinned().withTangents().withSpecular(),
        skinModelNormalMapVertex, modelNormalSpecularMapPixel, nullptr, nullptr);
    // Same thing but with Fade on
    addPipeline(
        Key::Builder().withMaterial().withSkinned().withFade(),
        skinModelFadeVertex, modelFadePixel, batchSetter, itemSetter);
    addPipeline(
        Key::Builder().withMaterial().withSkinned().withTangents().withFade(),
        skinModelNormalMapFadeVertex, modelNormalMapFadePixel, batchSetter, itemSetter);
    addPipeline(
        Key::Builder().withMaterial().withSkinned().withSpecular().withFade(),
        skinModelFadeVertex, modelSpecularMapFadePixel, batchSetter, itemSetter);
    addPipeline(
        Key::Builder().withMaterial().withSkinned().withTangents().withSpecular().withFade(),
        skinModelNormalMapFadeVertex, modelNormalSpecularMapFadePixel, batchSetter, itemSetter);

    // Skinned and Translucent
    addPipeline(
        Key::Builder().withMaterial().withSkinned().withTranslucent(),
        skinModelTranslucentVertex, modelTranslucentPixel, nullptr, nullptr);
    addPipeline(
        Key::Builder().withMaterial().withSkinned().withTranslucent().withTangents(),
        skinModelNormalMapTranslucentVertex, modelTranslucentNormalMapPixel, nullptr, nullptr);
    addPipeline(
        Key::Builder().withMaterial().withSkinned().withTranslucent().withSpecular(),
        skinModelTranslucentVertex, modelTranslucentPixel, nullptr, nullptr);
    addPipeline(
        Key::Builder().withMaterial().withSkinned().withTranslucent().withTangents().withSpecular(),
        skinModelNormalMapTranslucentVertex, modelTranslucentNormalMapPixel, nullptr, nullptr);
    // Same thing but with Fade on
    addPipeline(
        Key::Builder().withMaterial().withSkinned().withTranslucent().withFade(),
        skinModelFadeVertex, modelTranslucentFadePixel, batchSetter, itemSetter);
    addPipeline(
        Key::Builder().withMaterial().withSkinned().withTranslucent().withTangents().withFade(),
        skinModelNormalMapFadeVertex, modelTranslucentNormalMapFadePixel, batchSetter, itemSetter);
    addPipeline(
        Key::Builder().withMaterial().withSkinned().withTranslucent().withSpecular().withFade(),
        skinModelFadeVertex, modelTranslucentFadePixel, batchSetter, itemSetter);
    addPipeline(
        Key::Builder().withMaterial().withSkinned().withTranslucent().withTangents().withSpecular().withFade(),
        skinModelNormalMapFadeVertex, modelTranslucentNormalMapFadePixel, batchSetter, itemSetter);

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
}

void initForwardPipelines(ShapePlumber& plumber, const render::ShapePipeline::BatchSetter& batchSetter, const render::ShapePipeline::ItemSetter& itemSetter) {
    // Vertex shaders
    auto modelVertex = gpu::Shader::createVertex(std::string(model_vert));
    auto modelNormalMapVertex = gpu::Shader::createVertex(std::string(model_normal_map_vert));
    auto skinModelVertex = gpu::Shader::createVertex(std::string(skin_model_vert));
    auto skinModelNormalMapVertex = gpu::Shader::createVertex(std::string(skin_model_normal_map_vert));
    auto skinModelNormalMapFadeVertex = gpu::Shader::createVertex(std::string(skin_model_normal_map_fade_vert));

    // Pixel shaders
    auto modelPixel = gpu::Shader::createPixel(std::string(forward_model_frag));
    auto modelUnlitPixel = gpu::Shader::createPixel(std::string(forward_model_unlit_frag));
    auto modelNormalMapPixel = gpu::Shader::createPixel(std::string(forward_model_normal_map_frag));
    auto modelSpecularMapPixel = gpu::Shader::createPixel(std::string(forward_model_specular_map_frag));
    auto modelNormalSpecularMapPixel = gpu::Shader::createPixel(std::string(forward_model_normal_specular_map_frag));
    auto modelNormalMapFadePixel = gpu::Shader::createPixel(std::string(model_normal_map_fade_frag));

    using Key = render::ShapeKey;
    auto addPipeline = std::bind(&addPlumberPipeline, std::ref(plumber), _1, _2, _3, _4, _5);
    // Opaques
    addPipeline(
        Key::Builder().withMaterial(),
        modelVertex, modelPixel, nullptr, nullptr);
    addPipeline(
        Key::Builder().withMaterial().withUnlit(),
        modelVertex, modelUnlitPixel, nullptr, nullptr);
    addPipeline(
        Key::Builder().withMaterial().withTangents(),
        modelNormalMapVertex, modelNormalMapPixel, nullptr, nullptr);
    addPipeline(
        Key::Builder().withMaterial().withSpecular(),
        modelVertex, modelSpecularMapPixel, nullptr, nullptr);
    addPipeline(
        Key::Builder().withMaterial().withTangents().withSpecular(),
        modelNormalMapVertex, modelNormalSpecularMapPixel, nullptr, nullptr);
    // Skinned
    addPipeline(
        Key::Builder().withMaterial().withSkinned(),
        skinModelVertex, modelPixel, nullptr, nullptr);
    addPipeline(
        Key::Builder().withMaterial().withSkinned().withTangents(),
        skinModelNormalMapVertex, modelNormalMapPixel, nullptr, nullptr);
    addPipeline(
        Key::Builder().withMaterial().withSkinned().withSpecular(),
        skinModelVertex, modelSpecularMapPixel, nullptr, nullptr);
    addPipeline(
        Key::Builder().withMaterial().withSkinned().withTangents().withSpecular(),
        skinModelNormalMapVertex, modelNormalSpecularMapPixel, nullptr, nullptr);
    addPipeline(
            Key::Builder().withMaterial().withSkinned().withTangents().withFade(),
            skinModelNormalMapFadeVertex, modelNormalMapFadePixel, batchSetter, itemSetter, nullptr, nullptr);

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

        auto baseBatchSetter = key.isTranslucent() ? &lightBatchSetter : &batchSetter;
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
    auto modelVertex = gpu::Shader::createVertex(std::string(model_shadow_vert));
    auto modelPixel = gpu::Shader::createPixel(std::string(model_shadow_frag));
    gpu::ShaderPointer modelProgram = gpu::Shader::createProgram(modelVertex, modelPixel);
    shapePlumber.addPipeline(
        ShapeKey::Filter::Builder().withoutSkinned().withoutFade(),
        modelProgram, state);

    auto skinVertex = gpu::Shader::createVertex(std::string(skin_model_shadow_vert));
    auto skinPixel = gpu::Shader::createPixel(std::string(skin_model_shadow_frag));
    gpu::ShaderPointer skinProgram = gpu::Shader::createProgram(skinVertex, skinPixel);
    shapePlumber.addPipeline(
        ShapeKey::Filter::Builder().withSkinned().withoutFade(),
        skinProgram, state);

    auto modelFadeVertex = gpu::Shader::createVertex(std::string(model_shadow_fade_vert));
    auto modelFadePixel = gpu::Shader::createPixel(std::string(model_shadow_fade_frag));
    gpu::ShaderPointer modelFadeProgram = gpu::Shader::createProgram(modelFadeVertex, modelFadePixel);
    shapePlumber.addPipeline(
        ShapeKey::Filter::Builder().withoutSkinned().withFade(),
        modelFadeProgram, state);

    auto skinFadeVertex = gpu::Shader::createVertex(std::string(skin_model_shadow_fade_vert));
    auto skinFadePixel = gpu::Shader::createPixel(std::string(skin_model_shadow_fade_frag));
    gpu::ShaderPointer skinFadeProgram = gpu::Shader::createProgram(skinFadeVertex, skinFadePixel);
    shapePlumber.addPipeline(
        ShapeKey::Filter::Builder().withSkinned().withFade(),
        skinFadeProgram, state);
}
