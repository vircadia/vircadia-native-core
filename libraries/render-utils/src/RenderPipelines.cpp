
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

#include "RenderPipelines.h"

#include <functional>

#include <gpu/Context.h>
#include <model-networking/TextureCache.h>
#include <render/DrawTask.h>
#include <shaders/Shaders.h>
#include <graphics/ShaderConstants.h>

#include "render-utils/ShaderConstants.h"
#include "StencilMaskPass.h"
#include "DeferredLightingEffect.h"
#include "TextureCache.h"

using namespace render;
using namespace std::placeholders;

namespace ru {
    using render_utils::slot::texture::Texture;
    using render_utils::slot::buffer::Buffer;
}

namespace gr {
    using graphics::slot::texture::Texture;
    using graphics::slot::buffer::Buffer;
}

void initDeferredPipelines(ShapePlumber& plumber, const render::ShapePipeline::BatchSetter& batchSetter, const render::ShapePipeline::ItemSetter& itemSetter);
void initForwardPipelines(ShapePlumber& plumber);
void initZPassPipelines(ShapePlumber& plumber, gpu::StatePointer state, const render::ShapePipeline::BatchSetter& batchSetter, const render::ShapePipeline::ItemSetter& itemSetter);

void addPlumberPipeline(ShapePlumber& plumber,
        const ShapeKey& key, int programId,
        const render::ShapePipeline::BatchSetter& batchSetter, const render::ShapePipeline::ItemSetter& itemSetter);

void batchSetter(const ShapePipeline& pipeline, gpu::Batch& batch, RenderArgs* args);
void lightBatchSetter(const ShapePipeline& pipeline, gpu::Batch& batch, RenderArgs* args);
static bool forceLightBatchSetter{ false };

void initDeferredPipelines(render::ShapePlumber& plumber, const render::ShapePipeline::BatchSetter& batchSetter, const render::ShapePipeline::ItemSetter& itemSetter) {
    using namespace shader::render_utils::program;
    using Key = render::ShapeKey;
    auto addPipeline = std::bind(&addPlumberPipeline, std::ref(plumber), _1, _2, _3, _4);
    // TODO: Refactor this to use a filter
    // Opaques
    addPipeline(
        Key::Builder().withMaterial(),
        model, nullptr, nullptr);
    addPipeline(
        Key::Builder(),
        simple_textured, nullptr, nullptr);
    addPipeline(
        Key::Builder().withMaterial().withUnlit(),
        model_unlit, nullptr, nullptr);
    addPipeline(
        Key::Builder().withUnlit(),
        simple_textured_unlit, nullptr, nullptr);
    addPipeline(
        Key::Builder().withMaterial().withTangents(),
        model_normal_map, nullptr, nullptr);

    // Same thing but with Fade on
    addPipeline(
        Key::Builder().withMaterial().withFade(),
        model_fade, batchSetter, itemSetter);
    addPipeline(
        Key::Builder().withFade(),
        simple_textured_fade, batchSetter, itemSetter);
    addPipeline(
        Key::Builder().withMaterial().withUnlit().withFade(),
        model_unlit_fade, batchSetter, itemSetter);
    addPipeline(
        Key::Builder().withUnlit().withFade(),
        simple_textured_unlit_fade, batchSetter, itemSetter);
    addPipeline(
        Key::Builder().withMaterial().withTangents().withFade(),
        model_normal_map_fade, batchSetter, itemSetter);

    // Translucents
    addPipeline(
        Key::Builder().withMaterial().withTranslucent(),
        model_translucent, nullptr, nullptr);
    addPipeline(
        Key::Builder().withTranslucent(),
        simple_transparent_textured, nullptr, nullptr);
    addPipeline(
        Key::Builder().withMaterial().withTranslucent().withUnlit(),
        model_translucent_unlit, nullptr, nullptr);
    addPipeline(
        Key::Builder().withTranslucent().withUnlit(),
        simple_transparent_textured_unlit, nullptr, nullptr);
    addPipeline(
        Key::Builder().withMaterial().withTranslucent().withTangents(),
        model_translucent_normal_map, nullptr, nullptr);
    addPipeline(
        // FIXME: Ignore lightmap for translucents meshpart
        Key::Builder().withMaterial().withTranslucent().withLightmap(),
        model_translucent, nullptr, nullptr);
    // Same thing but with Fade on
    addPipeline(
        Key::Builder().withMaterial().withTranslucent().withFade(),
        model_translucent_fade, batchSetter, itemSetter);
    addPipeline(
        Key::Builder().withTranslucent().withFade(),
        simple_transparent_textured_fade, batchSetter, itemSetter);
    addPipeline(
        Key::Builder().withMaterial().withTranslucent().withUnlit().withFade(),
        model_translucent_unlit_fade, batchSetter, itemSetter);
    addPipeline(
        Key::Builder().withTranslucent().withUnlit().withFade(),
        simple_transparent_textured_unlit_fade, batchSetter, itemSetter);
    addPipeline(
        Key::Builder().withMaterial().withTranslucent().withTangents().withFade(),
        model_translucent_normal_map_fade, batchSetter, itemSetter);
    addPipeline(
        // FIXME: Ignore lightmap for translucents meshpart
        Key::Builder().withMaterial().withTranslucent().withLightmap().withFade(),
        model_translucent_fade, batchSetter, itemSetter);
    // Lightmapped
    addPipeline(
        Key::Builder().withMaterial().withLightmap(),
        model_lightmap, nullptr, nullptr);
    addPipeline(
        Key::Builder().withMaterial().withLightmap().withTangents(),
        model_lightmap_normal_map, nullptr, nullptr);
    // Same thing but with Fade on
    addPipeline(
        Key::Builder().withMaterial().withLightmap().withFade(),
        model_lightmap_fade, batchSetter, itemSetter);
    addPipeline(
        Key::Builder().withMaterial().withLightmap().withTangents().withFade(),
        model_lightmap_normal_map_fade, batchSetter, itemSetter);

    // matrix palette skinned
    addPipeline(
        Key::Builder().withMaterial().withDeformed(),
        deformed_model, nullptr, nullptr);
    addPipeline(
        Key::Builder().withMaterial().withDeformed().withTangents(),
        deformed_model_normal_map, nullptr, nullptr);
    // Same thing but with Fade on
    addPipeline(
        Key::Builder().withMaterial().withDeformed().withFade(),
        deformed_model_fade, batchSetter, itemSetter);
    addPipeline(
        Key::Builder().withMaterial().withDeformed().withTangents().withFade(),
        deformed_model_normal_map_fade, batchSetter, itemSetter);
    // matrix palette skinned and translucent
    addPipeline(
        Key::Builder().withMaterial().withDeformed().withTranslucent(),
        deformed_model_translucent, nullptr, nullptr);
    addPipeline(
        Key::Builder().withMaterial().withDeformed().withTranslucent().withTangents(),
        deformed_model_normal_map_translucent, nullptr, nullptr);
    // Same thing but with Fade on
    addPipeline(
        Key::Builder().withMaterial().withDeformed().withTranslucent().withFade(),
        deformed_model_translucent_fade, batchSetter, itemSetter);
    addPipeline(
        Key::Builder().withMaterial().withDeformed().withTranslucent().withTangents().withFade(),
        deformed_model_normal_map_translucent_fade, batchSetter, itemSetter);

    // dual quaternion skinned
    addPipeline(
        Key::Builder().withMaterial().withDeformed().withDualQuatSkinned(),
        deformed_model_dq, nullptr, nullptr);
    addPipeline(
        Key::Builder().withMaterial().withDeformed().withDualQuatSkinned().withTangents(),
        deformed_model_normal_map_dq, nullptr, nullptr);
    // Same thing but with Fade on
    addPipeline(
        Key::Builder().withMaterial().withDeformed().withDualQuatSkinned().withFade(),
        deformed_model_fade_dq, batchSetter, itemSetter);
    addPipeline(
        Key::Builder().withMaterial().withDeformed().withDualQuatSkinned().withTangents().withFade(),
        deformed_model_normal_map_fade_dq, batchSetter, itemSetter);
    // dual quaternion skinned and translucent
    addPipeline(
        Key::Builder().withMaterial().withDeformed().withDualQuatSkinned().withTranslucent(),
        deformed_model_translucent_dq, nullptr, nullptr);
    addPipeline(
        Key::Builder().withMaterial().withDeformed().withDualQuatSkinned().withTranslucent().withTangents(),
        deformed_model_normal_map_translucent_dq, nullptr, nullptr);
    // Same thing but with Fade on
    addPipeline(
        Key::Builder().withMaterial().withDeformed().withDualQuatSkinned().withTranslucent().withFade(),
        deformed_model_translucent_fade_dq, batchSetter, itemSetter);
    addPipeline(
        Key::Builder().withMaterial().withDeformed().withDualQuatSkinned().withTranslucent().withTangents().withFade(),
        deformed_model_normal_map_translucent_fade_dq, batchSetter, itemSetter);
}

void initForwardPipelines(ShapePlumber& plumber) {
    using namespace shader::render_utils;

    using Key = render::ShapeKey;
    auto addPipelineBind = std::bind(&addPlumberPipeline, std::ref(plumber), _1, _2, _3, _4);

    // Disable fade on the forward pipeline, all shaders get added twice, once with the fade key and once without
    auto addPipeline = [&](const ShapeKey& key, int programId) {
        addPipelineBind(key, programId, nullptr, nullptr);
        addPipelineBind(Key::Builder(key).withFade(), programId, nullptr, nullptr);
    };

    // Forward pipelines need the lightBatchSetter for opaques and transparents
    forceLightBatchSetter = true;

    // Simple Opaques
    addPipeline(Key::Builder(), program::forward_simple_textured);
    addPipeline(Key::Builder().withUnlit(), program::forward_simple_textured_unlit);

    // Simple Translucents
    addPipeline(Key::Builder().withTranslucent(), program::forward_simple_textured_transparent);
    addPipeline(Key::Builder().withTranslucent().withUnlit(), program::simple_transparent_textured_unlit);

    // Opaques
    addPipeline(Key::Builder().withMaterial(), program::forward_model);
    addPipeline(Key::Builder().withMaterial().withUnlit(), program::forward_model_unlit);
    addPipeline(Key::Builder().withMaterial().withTangents(), program::forward_model_normal_map);
 
    // Deformed Opaques
    addPipeline(Key::Builder().withMaterial().withDeformed(), program::forward_deformed_model);
    addPipeline(Key::Builder().withMaterial().withDeformed().withTangents(), program::forward_deformed_model_normal_map);
    addPipeline(Key::Builder().withMaterial().withDeformed().withDualQuatSkinned(), program::forward_deformed_model_dq);
    addPipeline(Key::Builder().withMaterial().withDeformed().withTangents().withDualQuatSkinned(), program::forward_deformed_model_normal_map_dq);

    // Translucents
    addPipeline(Key::Builder().withMaterial().withTranslucent(), program::forward_model_translucent);
    addPipeline(Key::Builder().withMaterial().withTranslucent().withTangents(), program::forward_model_normal_map_translucent);

    // Deformed Translucents
    addPipeline(Key::Builder().withMaterial().withDeformed().withTranslucent(), program::forward_deformed_translucent);
    addPipeline(Key::Builder().withMaterial().withDeformed().withTranslucent().withTangents(), program::forward_deformed_translucent_normal_map);
    addPipeline(Key::Builder().withMaterial().withDeformed().withTranslucent().withDualQuatSkinned(), program::forward_deformed_translucent_dq);
    addPipeline(Key::Builder().withMaterial().withDeformed().withTranslucent().withTangents().withDualQuatSkinned(), program::forward_deformed_translucent_normal_map_dq);

    // FIXME: incorrent pipelines for normal mapped + translucent models

    forceLightBatchSetter = false;
}

void addPlumberPipeline(ShapePlumber& plumber,
        const ShapeKey& key, int programId,
        const render::ShapePipeline::BatchSetter& extraBatchSetter, const render::ShapePipeline::ItemSetter& itemSetter) {
    // These key-values' pipelines are added by this functor in addition to the key passed
    assert(!key.isWireframe());
    assert(!key.isDepthBiased());
    assert(key.isCullFace());

    gpu::ShaderPointer program = gpu::Shader::createProgram(programId);

    for (int i = 0; i < 8; i++) {
        bool isCulled = (i & 1);
        bool isBiased = (i & 2);
        bool isWireframed = (i & 4);

        auto state = std::make_shared<gpu::State>();
        key.isTranslucent() ? PrepareStencil::testMask(*state) : PrepareStencil::testMaskDrawShape(*state);

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
    batch.setResourceTexture(gr::Texture::MaterialAlbedo,
        DependencyManager::get<TextureCache>()->getWhiteTexture());

    // Set a default material
    if (pipeline.locations->materialBufferUnit) {
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
        batch.setUniformBuffer(gr::Buffer::Material, material.getSchemaBuffer());
    }
}

void lightBatchSetter(const ShapePipeline& pipeline, gpu::Batch& batch, RenderArgs* args) {
    // Set the batch
    batchSetter(pipeline, batch, args);

    // Set the light
    if (pipeline.locations->keyLightBufferUnit) {
        DependencyManager::get<DeferredLightingEffect>()->setupKeyLightBatch(args, batch);
    }
}

void initZPassPipelines(ShapePlumber& shapePlumber, gpu::StatePointer state, const render::ShapePipeline::BatchSetter& extraBatchSetter, const render::ShapePipeline::ItemSetter& itemSetter) {
    using namespace shader::render_utils::program;

    shapePlumber.addPipeline(
        ShapeKey::Filter::Builder().withoutDeformed().withoutFade(),
        gpu::Shader::createProgram(model_shadow), state);
    shapePlumber.addPipeline(
        ShapeKey::Filter::Builder().withoutDeformed().withFade(),
        gpu::Shader::createProgram(model_shadow_fade), state, extraBatchSetter, itemSetter);

    shapePlumber.addPipeline(
        ShapeKey::Filter::Builder().withDeformed().withoutDualQuatSkinned().withoutFade(),
        gpu::Shader::createProgram(deformed_model_shadow), state);
    shapePlumber.addPipeline(
        ShapeKey::Filter::Builder().withDeformed().withoutDualQuatSkinned().withFade(),
        gpu::Shader::createProgram(deformed_model_shadow_fade), state, extraBatchSetter, itemSetter);

    shapePlumber.addPipeline(
        ShapeKey::Filter::Builder().withDeformed().withDualQuatSkinned().withoutFade(),
        gpu::Shader::createProgram(deformed_model_shadow_dq), state);
    shapePlumber.addPipeline(
        ShapeKey::Filter::Builder().withDeformed().withDualQuatSkinned().withFade(),
        gpu::Shader::createProgram(deformed_model_shadow_fade_dq), state, extraBatchSetter, itemSetter);
}

// FIXME find a better way to setup the default textures
void RenderPipelines::bindMaterial(const graphics::MaterialPointer& material, gpu::Batch& batch, bool enableTextures) {
    if (!material) {
        return;
    }

    auto textureCache = DependencyManager::get<TextureCache>();

    batch.setUniformBuffer(gr::Buffer::Material, material->getSchemaBuffer());

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
            drawMaterialTextures->setTexture(gr::Texture::MaterialAlbedo, itr->second->getTextureView());
        } else {
            drawMaterialTextures->setTexture(gr::Texture::MaterialAlbedo, textureCache->getWhiteTexture());
        }
    }

    // Roughness map
    if (materialKey.isRoughnessMap()) {
        auto itr = textureMaps.find(graphics::MaterialKey::ROUGHNESS_MAP);
        if (enableTextures && itr != textureMaps.end() && itr->second->isDefined()) {
            drawMaterialTextures->setTexture(gr::Texture::MaterialRoughness, itr->second->getTextureView());
        } else {
            drawMaterialTextures->setTexture(gr::Texture::MaterialRoughness, textureCache->getWhiteTexture());
        }
    }

    // Normal map
    if (materialKey.isNormalMap()) {
        auto itr = textureMaps.find(graphics::MaterialKey::NORMAL_MAP);
        if (enableTextures && itr != textureMaps.end() && itr->second->isDefined()) {
            drawMaterialTextures->setTexture(gr::Texture::MaterialNormal, itr->second->getTextureView());
        } else {
            drawMaterialTextures->setTexture(gr::Texture::MaterialNormal, textureCache->getBlueTexture());
        }
    }

    // Metallic map
    if (materialKey.isMetallicMap()) {
        auto itr = textureMaps.find(graphics::MaterialKey::METALLIC_MAP);
        if (enableTextures && itr != textureMaps.end() && itr->second->isDefined()) {
            drawMaterialTextures->setTexture(gr::Texture::MaterialMetallic, itr->second->getTextureView());
        } else {
            drawMaterialTextures->setTexture(gr::Texture::MaterialMetallic, textureCache->getBlackTexture());
        }
    }

    // Occlusion map
    if (materialKey.isOcclusionMap()) {
        auto itr = textureMaps.find(graphics::MaterialKey::OCCLUSION_MAP);
        if (enableTextures && itr != textureMaps.end() && itr->second->isDefined()) {
            drawMaterialTextures->setTexture(gr::Texture::MaterialOcclusion, itr->second->getTextureView());
        } else {
            drawMaterialTextures->setTexture(gr::Texture::MaterialOcclusion, textureCache->getWhiteTexture());
        }
    }

    // Scattering map
    if (materialKey.isScatteringMap()) {
        auto itr = textureMaps.find(graphics::MaterialKey::SCATTERING_MAP);
        if (enableTextures && itr != textureMaps.end() && itr->second->isDefined()) {
            drawMaterialTextures->setTexture(gr::Texture::MaterialScattering, itr->second->getTextureView());
        } else {
            drawMaterialTextures->setTexture(gr::Texture::MaterialScattering, textureCache->getWhiteTexture());
        }
    }

    // Emissive / Lightmap
    if (materialKey.isLightmapMap()) {
        auto itr = textureMaps.find(graphics::MaterialKey::LIGHTMAP_MAP);

        if (enableTextures && itr != textureMaps.end() && itr->second->isDefined()) {
            drawMaterialTextures->setTexture(gr::Texture::MaterialEmissiveLightmap, itr->second->getTextureView());
        } else {
            drawMaterialTextures->setTexture(gr::Texture::MaterialEmissiveLightmap, textureCache->getGrayTexture());
        }
    } else if (materialKey.isEmissiveMap()) {
        auto itr = textureMaps.find(graphics::MaterialKey::EMISSIVE_MAP);
        if (enableTextures && itr != textureMaps.end() && itr->second->isDefined()) {
            drawMaterialTextures->setTexture(gr::Texture::MaterialEmissiveLightmap, itr->second->getTextureView());
        } else {
            drawMaterialTextures->setTexture(gr::Texture::MaterialEmissiveLightmap, textureCache->getBlackTexture());
        }
    }

    batch.setResourceTextureTable(material->getTextureTable());
}
