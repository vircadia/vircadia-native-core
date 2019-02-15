
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
#include <material-networking/TextureCache.h>
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
    batch.setResourceTexture(gr::Texture::MaterialAlbedo, DependencyManager::get<TextureCache>()->getWhiteTexture());

    // Set a default material
    if (pipeline.locations->materialBufferUnit) {
        // Create a default schema
        static gpu::BufferView schemaBuffer;
        static std::once_flag once;
        std::call_once(once, [] {
            graphics::MultiMaterial::Schema schema;
            graphics::MaterialKey schemaKey;

            schema._albedo = ColorUtils::sRGBToLinearVec3(vec3(1.0f));
            schema._opacity = 1.0f;
            schema._metallic = 0.1f;
            schema._roughness = 0.9f;

            schemaKey.setAlbedo(true);
            schemaKey.setTranslucentFactor(false);
            schemaKey.setMetallic(true);
            schemaKey.setGlossy(true);
            schema._key = (uint32_t)schemaKey._flags.to_ulong();

            auto schemaSize = sizeof(graphics::MultiMaterial::Schema);
            schemaBuffer = gpu::BufferView(std::make_shared<gpu::Buffer>(schemaSize, (const gpu::Byte*) &schema, schemaSize));
        });

        batch.setUniformBuffer(gr::Buffer::Material, schemaBuffer);
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

void RenderPipelines::bindMaterial(graphics::MaterialPointer& material, gpu::Batch& batch, bool enableTextures) {
    graphics::MultiMaterial multiMaterial;
    multiMaterial.push(graphics::MaterialLayer(material, 0));
    bindMaterials(multiMaterial, batch, enableTextures);
}

void RenderPipelines::updateMultiMaterial(graphics::MultiMaterial& multiMaterial) {
    auto& schemaBuffer = multiMaterial.getSchemaBuffer();

    if (multiMaterial.size() == 0) {
        schemaBuffer.edit<graphics::MultiMaterial::Schema>() = graphics::MultiMaterial::Schema();
        return;
    }

    auto& drawMaterialTextures = multiMaterial.getTextureTable();
    multiMaterial.setTexturesLoading(false);

    // The total list of things we need to look for
    static std::set<uint> allFlags;
    static std::once_flag once;
    std::call_once(once, [] {
        for (int i = 0; i < graphics::Material::NUM_TOTAL_FLAGS; i++) {
            // The opacity mask/map are derived from the albedo map
            if (i != graphics::MaterialKey::OPACITY_MASK_MAP_BIT &&
                    i != graphics::MaterialKey::OPACITY_TRANSLUCENT_MAP_BIT) {
                allFlags.insert(i);
            }
        }
    });

    graphics::MultiMaterial materials = multiMaterial;
    graphics::MultiMaterial::Schema schema;
    graphics::MaterialKey schemaKey;

    std::set<uint> flagsToCheck = allFlags;
    std::set<uint> flagsToSetDefault;

    while (!materials.empty()) {
        auto material = materials.top().material;
        if (!material) {
            break;
        }
        materials.pop();

        bool defaultFallthrough = material->getDefaultFallthrough();
        const auto& materialKey = material->getKey();
        const auto& textureMaps = material->getTextureMaps();

        auto it = flagsToCheck.begin();
        while (it != flagsToCheck.end()) {
            auto flag = *it;
            bool fallthrough = defaultFallthrough || material->getPropertyFallthrough(flag);

            bool wasSet = false;
            bool forceDefault = false;
            switch (flag) {
                case graphics::MaterialKey::EMISSIVE_VAL_BIT:
                    if (materialKey.isEmissive()) {
                        schema._emissive = material->getEmissive(false);
                        schemaKey.setEmissive(true);
                        wasSet = true;
                    }
                    break;
                case graphics::MaterialKey::UNLIT_VAL_BIT:
                    if (materialKey.isUnlit()) {
                        schemaKey.setUnlit(true);
                        wasSet = true;
                    }
                    break;
                case graphics::MaterialKey::ALBEDO_VAL_BIT:
                    if (materialKey.isAlbedo()) {
                        schema._albedo = material->getAlbedo(false);
                        schemaKey.setAlbedo(true);
                        wasSet = true;
                    }
                    break;
                case graphics::MaterialKey::METALLIC_VAL_BIT:
                    if (materialKey.isMetallic()) {
                        schema._metallic = material->getMetallic();
                        schemaKey.setMetallic(true);
                        wasSet = true;
                    }
                    break;
                case graphics::MaterialKey::GLOSSY_VAL_BIT:
                    if (materialKey.isRough() || materialKey.isGlossy()) {
                        schema._roughness = material->getRoughness();
                        schemaKey.setGlossy(materialKey.isGlossy());
                        wasSet = true;
                    }
                    break;
                case graphics::MaterialKey::OPACITY_VAL_BIT:
                    if (materialKey.isTranslucentFactor()) {
                        schema._opacity = material->getOpacity();
                        schemaKey.setTranslucentFactor(true);
                        wasSet = true;
                    }
                    break;
                case graphics::MaterialKey::SCATTERING_VAL_BIT:
                    if (materialKey.isScattering()) {
                        schema._scattering = material->getScattering();
                        schemaKey.setScattering(true);
                        wasSet = true;
                    }
                    break;
                case graphics::MaterialKey::ALBEDO_MAP_BIT:
                    if (materialKey.isAlbedoMap()) {
                        auto itr = textureMaps.find(graphics::MaterialKey::ALBEDO_MAP);
                        if (itr != textureMaps.end()) {
                            if (itr->second->isDefined()) {
                                material->resetOpacityMap();
                                drawMaterialTextures->setTexture(gr::Texture::MaterialAlbedo, itr->second->getTextureView());
                                wasSet = true;
                            } else {
                                multiMaterial.setTexturesLoading(true);
                                forceDefault = true;
                            }
                        } else {
                            forceDefault = true;
                        }
                        schemaKey.setAlbedoMap(true);
                        schemaKey.setOpacityMaskMap(material->getKey().isOpacityMaskMap());
                        schemaKey.setTranslucentMap(material->getKey().isTranslucentMap());
                    }
                    break;
                case graphics::MaterialKey::METALLIC_MAP_BIT:
                    if (materialKey.isMetallicMap()) {
                        auto itr = textureMaps.find(graphics::MaterialKey::METALLIC_MAP);
                        if (itr != textureMaps.end()) {
                            if (itr->second->isDefined()) {
                                drawMaterialTextures->setTexture(gr::Texture::MaterialMetallic, itr->second->getTextureView());
                                wasSet = true;
                            } else {
                                multiMaterial.setTexturesLoading(true);
                                forceDefault = true;
                            }
                        } else {
                            forceDefault = true;
                        }
                        schemaKey.setMetallicMap(true);
                    }
                    break;
                case graphics::MaterialKey::ROUGHNESS_MAP_BIT:
                    if (materialKey.isRoughnessMap()) {
                        auto itr = textureMaps.find(graphics::MaterialKey::ROUGHNESS_MAP);
                        if (itr != textureMaps.end()) {
                            if (itr->second->isDefined()) {
                                drawMaterialTextures->setTexture(gr::Texture::MaterialRoughness, itr->second->getTextureView());
                                wasSet = true;
                            } else {
                                multiMaterial.setTexturesLoading(true);
                                forceDefault = true;
                            }
                        } else {
                            forceDefault = true;
                        }
                        schemaKey.setRoughnessMap(true);
                    }
                    break;
                case graphics::MaterialKey::NORMAL_MAP_BIT:
                    if (materialKey.isNormalMap()) {
                        auto itr = textureMaps.find(graphics::MaterialKey::NORMAL_MAP);
                        if (itr != textureMaps.end()) {
                            if (itr->second->isDefined()) {
                                drawMaterialTextures->setTexture(gr::Texture::MaterialNormal, itr->second->getTextureView());
                                wasSet = true;
                            } else {
                                multiMaterial.setTexturesLoading(true);
                                forceDefault = true;
                            }
                        } else {
                            forceDefault = true;
                        }
                        schemaKey.setNormalMap(true);
                    }
                    break;
                case graphics::MaterialKey::OCCLUSION_MAP_BIT:
                    if (materialKey.isOcclusionMap()) {
                        auto itr = textureMaps.find(graphics::MaterialKey::OCCLUSION_MAP);
                        if (itr != textureMaps.end()) {
                            if (itr->second->isDefined()) {
                                drawMaterialTextures->setTexture(gr::Texture::MaterialOcclusion, itr->second->getTextureView());
                                wasSet = true;
                            } else {
                                multiMaterial.setTexturesLoading(true);
                                forceDefault = true;
                            }
                        } else {
                            forceDefault = true;
                        }
                        schemaKey.setOcclusionMap(true);
                    }
                    break;
                case graphics::MaterialKey::SCATTERING_MAP_BIT:
                    if (materialKey.isScatteringMap()) {
                        auto itr = textureMaps.find(graphics::MaterialKey::SCATTERING_MAP);
                        if (itr != textureMaps.end()) {
                            if (itr->second->isDefined()) {
                                drawMaterialTextures->setTexture(gr::Texture::MaterialScattering, itr->second->getTextureView());
                                wasSet = true;
                            } else {
                                multiMaterial.setTexturesLoading(true);
                                forceDefault = true;
                            }
                        } else {
                            forceDefault = true;
                        }
                        schemaKey.setScatteringMap(true);
                    }
                    break;
                case graphics::MaterialKey::EMISSIVE_MAP_BIT:
                    // Lightmap takes precendence over emissive map for legacy reasons
                    if (materialKey.isEmissiveMap() && !materialKey.isLightmapMap()) {
                        auto itr = textureMaps.find(graphics::MaterialKey::EMISSIVE_MAP);
                        if (itr != textureMaps.end()) {
                            if (itr->second->isDefined()) {
                                drawMaterialTextures->setTexture(gr::Texture::MaterialEmissiveLightmap, itr->second->getTextureView());
                                wasSet = true;
                            } else {
                                multiMaterial.setTexturesLoading(true);
                                forceDefault = true;
                            }
                        } else {
                            forceDefault = true;
                        }
                        schemaKey.setEmissiveMap(true);
                    } else if (materialKey.isLightmapMap()) {
                        // We'll set this later when we check the lightmap
                        wasSet = true;
                    }
                    break;
                case graphics::MaterialKey::LIGHTMAP_MAP_BIT:
                    if (materialKey.isLightmapMap()) {
                        auto itr = textureMaps.find(graphics::MaterialKey::LIGHTMAP_MAP);
                        if (itr != textureMaps.end()) {
                            if (itr->second->isDefined()) {
                                drawMaterialTextures->setTexture(gr::Texture::MaterialEmissiveLightmap, itr->second->getTextureView());
                                wasSet = true;
                            } else {
                                multiMaterial.setTexturesLoading(true);
                                forceDefault = true;
                            }
                        } else {
                            forceDefault = true;
                        }
                        schemaKey.setLightmapMap(true);
                    }
                    break;
                case graphics::Material::TEXCOORDTRANSFORM0:
                    if (!fallthrough) {
                        schema._texcoordTransforms[0] = material->getTexCoordTransform(0);
                        wasSet = true;
                    }
                    break;
                case graphics::Material::TEXCOORDTRANSFORM1:
                    if (!fallthrough) {
                        schema._texcoordTransforms[1] = material->getTexCoordTransform(1);
                        wasSet = true;
                    }
                    break;
                case graphics::Material::LIGHTMAP_PARAMS:
                    if (!fallthrough) {
                        schema._lightmapParams = material->getLightmapParams();
                        wasSet = true;
                    }
                    break;
                case graphics::Material::MATERIAL_PARAMS:
                    if (!fallthrough) {
                        schema._materialParams = material->getMaterialParams();
                        wasSet = true;
                    }
                    break;
                default:
                    break;
            }

            if (wasSet) {
                flagsToCheck.erase(it++);
            } else if (forceDefault || !fallthrough) {
                flagsToSetDefault.insert(flag);
                flagsToCheck.erase(it++);
            } else {
                ++it;
            }
        }

        if (flagsToCheck.empty()) {
            break;
        }
    }

    for (auto flagBit : flagsToCheck) {
        flagsToSetDefault.insert(flagBit);
    }

    auto textureCache = DependencyManager::get<TextureCache>();
    // Handle defaults
    for (auto flag : flagsToSetDefault) {
        switch (flag) {
            case graphics::MaterialKey::EMISSIVE_VAL_BIT:
            case graphics::MaterialKey::UNLIT_VAL_BIT:
            case graphics::MaterialKey::ALBEDO_VAL_BIT:
            case graphics::MaterialKey::METALLIC_VAL_BIT:
            case graphics::MaterialKey::GLOSSY_VAL_BIT:
            case graphics::MaterialKey::OPACITY_VAL_BIT:
            case graphics::MaterialKey::SCATTERING_VAL_BIT:
            case graphics::Material::TEXCOORDTRANSFORM0:
            case graphics::Material::TEXCOORDTRANSFORM1:
            case graphics::Material::LIGHTMAP_PARAMS:
            case graphics::Material::MATERIAL_PARAMS:
                // these are initialized to the correct default values in Schema()
                break;
            case graphics::MaterialKey::ALBEDO_MAP_BIT:
                if (schemaKey.isAlbedoMap()) {
                    drawMaterialTextures->setTexture(gr::Texture::MaterialAlbedo, textureCache->getWhiteTexture());
                }
                break;
            case graphics::MaterialKey::METALLIC_MAP_BIT:
                if (schemaKey.isMetallicMap()) {
                    drawMaterialTextures->setTexture(gr::Texture::MaterialMetallic, textureCache->getBlackTexture());
                }
                break;
            case graphics::MaterialKey::ROUGHNESS_MAP_BIT:
                if (schemaKey.isRoughnessMap()) {
                    drawMaterialTextures->setTexture(gr::Texture::MaterialRoughness, textureCache->getWhiteTexture());
                }
                break;
            case graphics::MaterialKey::NORMAL_MAP_BIT:
                if (schemaKey.isNormalMap()) {
                    drawMaterialTextures->setTexture(gr::Texture::MaterialNormal, textureCache->getBlueTexture());
                }
                break;
            case graphics::MaterialKey::OCCLUSION_MAP_BIT:
                if (schemaKey.isOcclusionMap()) {
                    drawMaterialTextures->setTexture(gr::Texture::MaterialOcclusion, textureCache->getWhiteTexture());
                }
                break;
            case graphics::MaterialKey::SCATTERING_MAP_BIT:
                if (schemaKey.isScatteringMap()) {
                    drawMaterialTextures->setTexture(gr::Texture::MaterialScattering, textureCache->getWhiteTexture());
                }
                break;
            case graphics::MaterialKey::EMISSIVE_MAP_BIT:
                if (schemaKey.isEmissiveMap() && !schemaKey.isLightmapMap()) {
                    drawMaterialTextures->setTexture(gr::Texture::MaterialEmissiveLightmap, textureCache->getGrayTexture());
                }
                break;
            case graphics::MaterialKey::LIGHTMAP_MAP_BIT:
                if (schemaKey.isLightmapMap()) {
                    drawMaterialTextures->setTexture(gr::Texture::MaterialEmissiveLightmap, textureCache->getBlackTexture());
                }
                break;
            default:
                break;
        }
    }

    schema._key = (uint32_t)schemaKey._flags.to_ulong();
    schemaBuffer.edit<graphics::MultiMaterial::Schema>() = schema;
    multiMaterial.setNeedsUpdate(false);
}

void RenderPipelines::bindMaterials(graphics::MultiMaterial& multiMaterial, gpu::Batch& batch, bool enableTextures) {
    if (multiMaterial.size() == 0) {
        return;
    }

    if (multiMaterial.needsUpdate() || multiMaterial.areTexturesLoading()) {
        updateMultiMaterial(multiMaterial);
    }

    auto textureCache = DependencyManager::get<TextureCache>();

    static gpu::TextureTablePointer defaultMaterialTextures = std::make_shared<gpu::TextureTable>();
    static std::once_flag once;
    std::call_once(once, [textureCache] {
        defaultMaterialTextures->setTexture(gr::Texture::MaterialAlbedo, textureCache->getWhiteTexture());
        defaultMaterialTextures->setTexture(gr::Texture::MaterialMetallic, textureCache->getBlackTexture());
        defaultMaterialTextures->setTexture(gr::Texture::MaterialRoughness, textureCache->getWhiteTexture());
        defaultMaterialTextures->setTexture(gr::Texture::MaterialNormal, textureCache->getBlueTexture());
        defaultMaterialTextures->setTexture(gr::Texture::MaterialOcclusion, textureCache->getWhiteTexture());
        defaultMaterialTextures->setTexture(gr::Texture::MaterialScattering, textureCache->getWhiteTexture());
        // MaterialEmissiveLightmap has to be set later
    });

    auto& schemaBuffer = multiMaterial.getSchemaBuffer();
    batch.setUniformBuffer(gr::Buffer::Material, schemaBuffer);
    if (enableTextures) {
        batch.setResourceTextureTable(multiMaterial.getTextureTable());
    } else {
        auto key = multiMaterial.getMaterialKey();
        if (key.isLightmapMap()) {
            defaultMaterialTextures->setTexture(gr::Texture::MaterialEmissiveLightmap, textureCache->getBlackTexture());
        } else if (key.isEmissiveMap()) {
            defaultMaterialTextures->setTexture(gr::Texture::MaterialEmissiveLightmap, textureCache->getGrayTexture());
        }
        batch.setResourceTextureTable(defaultMaterialTextures);
    }
}
