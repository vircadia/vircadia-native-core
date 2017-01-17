
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

#include "DeferredLightingEffect.h"
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
#include "model_unlit_frag.h"
#include "model_shadow_frag.h"
#include "model_normal_map_frag.h"
#include "model_normal_specular_map_frag.h"
#include "model_specular_map_frag.h"
#include "model_lightmap_frag.h"
#include "model_lightmap_normal_map_frag.h"
#include "model_lightmap_normal_specular_map_frag.h"
#include "model_lightmap_specular_map_frag.h"
#include "model_translucent_frag.h"
#include "model_translucent_unlit_frag.h"

#include "overlay3D_vert.h"
#include "overlay3D_frag.h"
#include "overlay3D_translucent_frag.h"
#include "overlay3D_unlit_frag.h"
#include "overlay3D_translucent_unlit_frag.h"


using namespace render;
using namespace std::placeholders;

void initOverlay3DPipelines(ShapePlumber& plumber);
void initDeferredPipelines(ShapePlumber& plumber);
void initForwardPipelines(ShapePlumber& plumber);

void addPlumberPipeline(ShapePlumber& plumber,
        const ShapeKey& key, const gpu::ShaderPointer& vertex, const gpu::ShaderPointer& pixel);

void batchSetter(const ShapePipeline& pipeline, gpu::Batch& batch);
void lightBatchSetter(const ShapePipeline& pipeline, gpu::Batch& batch);

void initOverlay3DPipelines(ShapePlumber& plumber) {
    auto vertex = gpu::Shader::createVertex(std::string(overlay3D_vert));
    auto pixel = gpu::Shader::createPixel(std::string(overlay3D_frag));
    auto pixelTranslucent = gpu::Shader::createPixel(std::string(overlay3D_translucent_frag));
    auto pixelUnlit = gpu::Shader::createPixel(std::string(overlay3D_unlit_frag));
    auto pixelTranslucentUnlit = gpu::Shader::createPixel(std::string(overlay3D_translucent_unlit_frag));

    auto opaqueProgram = gpu::Shader::createProgram(vertex, pixel);
    auto translucentProgram = gpu::Shader::createProgram(vertex, pixelTranslucent);
    auto unlitOpaqueProgram = gpu::Shader::createProgram(vertex, pixelUnlit);
    auto unlitTranslucentProgram = gpu::Shader::createProgram(vertex, pixelTranslucentUnlit);

    for (int i = 0; i < 8; i++) {
        bool isCulled = (i & 1);
        bool isBiased = (i & 2);
        bool isOpaque = (i & 4);

        auto state = std::make_shared<gpu::State>();
        state->setDepthTest(false);
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
        plumber.addPipeline(builder.withoutUnlit().build(), simpleProgram, state, &lightBatchSetter);
        plumber.addPipeline(builder.withUnlit().build(), unlitProgram, state, &batchSetter);
    }
}

void initDeferredPipelines(render::ShapePlumber& plumber) {
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
    auto modelUnlitPixel = gpu::Shader::createPixel(std::string(model_unlit_frag));
    auto modelNormalMapPixel = gpu::Shader::createPixel(std::string(model_normal_map_frag));
    auto modelSpecularMapPixel = gpu::Shader::createPixel(std::string(model_specular_map_frag));
    auto modelNormalSpecularMapPixel = gpu::Shader::createPixel(std::string(model_normal_specular_map_frag));
    auto modelTranslucentPixel = gpu::Shader::createPixel(std::string(model_translucent_frag));
    auto modelTranslucentUnlitPixel = gpu::Shader::createPixel(std::string(model_translucent_unlit_frag));
    auto modelShadowPixel = gpu::Shader::createPixel(std::string(model_shadow_frag));
    auto modelLightmapPixel = gpu::Shader::createPixel(std::string(model_lightmap_frag));
    auto modelLightmapNormalMapPixel = gpu::Shader::createPixel(std::string(model_lightmap_normal_map_frag));
    auto modelLightmapSpecularMapPixel = gpu::Shader::createPixel(std::string(model_lightmap_specular_map_frag));
    auto modelLightmapNormalSpecularMapPixel = gpu::Shader::createPixel(std::string(model_lightmap_normal_specular_map_frag));

    using Key = render::ShapeKey;
    auto addPipeline = std::bind(&addPlumberPipeline, std::ref(plumber), _1, _2, _3);
    // TODO: Refactor this to use a filter
    // Opaques
    addPipeline(
        Key::Builder(),
        modelVertex, modelPixel);
    addPipeline(
        Key::Builder().withUnlit(),
        modelVertex, modelUnlitPixel);
    addPipeline(
        Key::Builder().withTangents(),
        modelNormalMapVertex, modelNormalMapPixel);
    addPipeline(
        Key::Builder().withSpecular(),
        modelVertex, modelSpecularMapPixel);
    addPipeline(
        Key::Builder().withTangents().withSpecular(),
        modelNormalMapVertex, modelNormalSpecularMapPixel);
    // Translucents
    addPipeline(
        Key::Builder().withTranslucent(),
        modelVertex, modelTranslucentPixel);
    addPipeline(
        Key::Builder().withTranslucent().withUnlit(),
        modelVertex, modelTranslucentUnlitPixel);
    addPipeline(
        Key::Builder().withTranslucent().withTangents(),
        modelNormalMapVertex, modelTranslucentPixel);
    addPipeline(
        Key::Builder().withTranslucent().withSpecular(),
        modelVertex, modelTranslucentPixel);
    addPipeline(
        Key::Builder().withTranslucent().withTangents().withSpecular(),
        modelNormalMapVertex, modelTranslucentPixel);
    addPipeline(
        // FIXME: Ignore lightmap for translucents meshpart
        Key::Builder().withTranslucent().withLightmap(),
        modelVertex, modelTranslucentPixel);
    // Lightmapped
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
    // Skinned
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
    // Skinned and Translucent
    addPipeline(
        Key::Builder().withSkinned().withTranslucent(),
        skinModelVertex, modelTranslucentPixel);
    addPipeline(
        Key::Builder().withSkinned().withTranslucent().withTangents(),
        skinModelNormalMapVertex, modelTranslucentPixel);
    addPipeline(
        Key::Builder().withSkinned().withTranslucent().withSpecular(),
        skinModelVertex, modelTranslucentPixel);
    addPipeline(
        Key::Builder().withSkinned().withTranslucent().withTangents().withSpecular(),
        skinModelNormalMapVertex, modelTranslucentPixel);
    // Depth-only
    addPipeline(
        Key::Builder().withDepthOnly(),
        modelShadowVertex, modelShadowPixel);
    addPipeline(
        Key::Builder().withSkinned().withDepthOnly(),
        skinModelShadowVertex, modelShadowPixel);
}

void initForwardPipelines(render::ShapePlumber& plumber) {
    // Vertex shaders
    auto modelVertex = gpu::Shader::createVertex(std::string(model_vert));
    auto modelNormalMapVertex = gpu::Shader::createVertex(std::string(model_normal_map_vert));
    auto skinModelVertex = gpu::Shader::createVertex(std::string(skin_model_vert));
    auto skinModelNormalMapVertex = gpu::Shader::createVertex(std::string(skin_model_normal_map_vert));

    // Pixel shaders
    auto modelPixel = gpu::Shader::createPixel(std::string(model_frag));
    auto modelUnlitPixel = gpu::Shader::createPixel(std::string(model_unlit_frag));
    auto modelNormalMapPixel = gpu::Shader::createPixel(std::string(model_normal_map_frag));
    auto modelSpecularMapPixel = gpu::Shader::createPixel(std::string(model_specular_map_frag));
    auto modelNormalSpecularMapPixel = gpu::Shader::createPixel(std::string(model_normal_specular_map_frag));

    using Key = render::ShapeKey;
    auto addPipeline = std::bind(&addPlumberPipeline, std::ref(plumber), _1, _2, _3);
    // Opaques
    addPipeline(
        Key::Builder(),
        modelVertex, modelPixel);
    addPipeline(
        Key::Builder().withUnlit(),
        modelVertex, modelUnlitPixel);
    addPipeline(
        Key::Builder().withTangents(),
        modelNormalMapVertex, modelNormalMapPixel);
    addPipeline(
        Key::Builder().withSpecular(),
        modelVertex, modelSpecularMapPixel);
    addPipeline(
        Key::Builder().withTangents().withSpecular(),
        modelNormalMapVertex, modelNormalSpecularMapPixel);
    // Skinned
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
}

void addPlumberPipeline(ShapePlumber& plumber,
        const ShapeKey& key, const gpu::ShaderPointer& vertex, const gpu::ShaderPointer& pixel) {
    // These key-values' pipelines are added by this functor in addition to the key passed
    assert(!key.isWireFrame());
    assert(!key.isDepthBiased());
    assert(key.isCullFace());

    gpu::ShaderPointer program = gpu::Shader::createProgram(vertex, pixel);

    for (int i = 0; i < 8; i++) {
        bool isCulled = (i & 1);
        bool isBiased = (i & 2);
        bool isWireframed = (i & 4);

        auto state = std::make_shared<gpu::State>();

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

        plumber.addPipeline(builder.build(), program, state,
                key.isTranslucent() ? &lightBatchSetter : &batchSetter);
    }
}

void batchSetter(const ShapePipeline& pipeline, gpu::Batch& batch) {
    // Set a default albedo map
    batch.setResourceTexture(render::ShapePipeline::Slot::MAP::ALBEDO,
        DependencyManager::get<TextureCache>()->getWhiteTexture());
    // Set a default normal map
    batch.setResourceTexture(render::ShapePipeline::Slot::MAP::NORMAL_FITTING,
        DependencyManager::get<TextureCache>()->getNormalFittingTexture());

    // Set a default material
    if (pipeline.locations->materialBufferUnit >= 0) {
        // Create a default schema
        static bool isMaterialSet = false;
        static model::Material material;
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

void lightBatchSetter(const ShapePipeline& pipeline, gpu::Batch& batch) {
    // Set the batch
    batchSetter(pipeline, batch);

    // Set the light
    if (pipeline.locations->lightBufferUnit >= 0) {
        DependencyManager::get<DeferredLightingEffect>()->setupKeyLightBatch(batch,
            pipeline.locations->lightBufferUnit,
            pipeline.locations->lightAmbientBufferUnit,
            pipeline.locations->lightAmbientMapUnit);
    }
}
