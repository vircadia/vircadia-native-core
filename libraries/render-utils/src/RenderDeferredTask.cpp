
//
//  RenderDeferredTask.cpp
//  render-utils/src/
//
//  Created by Sam Gateau on 5/29/15.
//  Copyright 20154 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <PerfStat.h>
#include <PathUtils.h>
#include <RenderArgs.h>
#include <ViewFrustum.h>
#include <gpu/Context.h>
#include <gpu/StandardShaderLib.h>

#include "DebugDeferredBuffer.h"
#include "DeferredLightingEffect.h"
#include "FramebufferCache.h"
#include "HitEffect.h"
#include "TextureCache.h"

#include "render/DrawTask.h"
#include "render/DrawStatus.h"
#include "AmbientOcclusionEffect.h"
#include "AntialiasingEffect.h"
#include "ToneMappingEffect.h"

#include "RenderDeferredTask.h"

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

void initDeferredPipelines(render::ShapePlumber& plumber);

void PrepareDeferred::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) {
    DependencyManager::get<DeferredLightingEffect>()->prepare(renderContext->args);
}

void RenderDeferred::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) {
    DependencyManager::get<DeferredLightingEffect>()->render(renderContext);
}

RenderDeferredTask::RenderDeferredTask(CullFunctor cullFunctor) {
    cullFunctor = cullFunctor ? cullFunctor : [](const RenderArgs*, const AABox&){ return true; };

    // Prepare the ShapePipelines
    ShapePlumberPointer shapePlumber = std::make_shared<ShapePlumber>();
    initDeferredPipelines(*shapePlumber);
    
    // CPU: Fetch the renderOpaques
    const auto fetchedOpaques = addJob<FetchItems>("FetchOpaque");
    const auto culledOpaques = addJob<CullItems<RenderDetails::OPAQUE_ITEM>>("CullOpaque", fetchedOpaques, cullFunctor);
    const auto opaques = addJob<DepthSortItems>("DepthSortOpaque", culledOpaques);

    // CPU only, create the list of renderedTransparents items
    const auto fetchedTransparents = addJob<FetchItems>("FetchTransparent", FetchItems(
        ItemFilter::Builder::transparentShape().withoutLayered()));
    const auto culledTransparents =
        addJob<CullItems<RenderDetails::TRANSLUCENT_ITEM>>("CullTransparent", fetchedTransparents, cullFunctor);
    const auto transparents = addJob<DepthSortItems>("DepthSortTransparent", culledTransparents, DepthSortItems(false));

    // GPU Jobs: Start preparing the deferred and lighting buffer
    addJob<PrepareDeferred>("PrepareDeferred");

    // Render opaque objects in DeferredBuffer
    addJob<DrawDeferred>("DrawOpaqueDeferred", opaques, shapePlumber);

    // Once opaque is all rendered create stencil background
    addJob<DrawStencilDeferred>("DrawOpaqueStencil");

    // Use Stencil and start drawing background in Lighting buffer
    addJob<DrawBackgroundDeferred>("DrawBackgroundDeferred");

    // AO job
    addJob<AmbientOcclusionEffect>("AmbientOcclusion");

    // Draw Lights just add the lights to the current list of lights to deal with. NOt really gpu job for now.
    addJob<DrawLight>("DrawLight", cullFunctor);

    // DeferredBuffer is complete, now let's shade it into the LightingBuffer
    addJob<RenderDeferred>("RenderDeferred");

    // AA job to be revisited
    addJob<Antialiasing>("Antialiasing");

    // Render transparent objects forward in LightingBuffer
    addJob<DrawDeferred>("DrawTransparentDeferred", transparents, shapePlumber);
    
    // Lighting Buffer ready for tone mapping
    addJob<ToneMappingDeferred>("ToneMapping");

    // Debugging Deferred buffer job
    addJob<DebugDeferredBuffer>("DebugDeferredBuffer");

    // Status icon rendering job
    {
        // Grab a texture map representing the different status icons and assign that to the drawStatsuJob
        auto iconMapPath = PathUtils::resourcesPath() + "icons/statusIconAtlas.svg";
        auto statusIconMap = DependencyManager::get<TextureCache>()->getImageTexture(iconMapPath);
        addJob<DrawStatus>("DrawStatus", opaques, DrawStatus(statusIconMap));
    }

    addJob<DrawOverlay3D>("DrawOverlay3D", shapePlumber);

    addJob<HitEffect>("HitEffect");

    addJob<Blit>("Blit");
}

void RenderDeferredTask::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) {
    // sanity checks
    assert(sceneContext);
    if (!sceneContext->_scene) {
        return;
    }


    // Is it possible that we render without a viewFrustum ?
    if (!(renderContext->args && renderContext->args->_viewFrustum)) {
        return;
    }

    for (auto job : _jobs) {
        job.run(sceneContext, renderContext);
    }
};

void DrawDeferred::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const ItemIDsBounds& inItems) {
    assert(renderContext->args);
    assert(renderContext->args->_viewFrustum);

    auto config = std::static_pointer_cast<Config>(renderContext->jobConfig);

    RenderArgs* args = renderContext->args;
    gpu::doInBatch(args->_context, [&](gpu::Batch& batch) {
        batch.setViewportTransform(args->_viewport);
        batch.setStateScissorRect(args->_viewport);
        args->_batch = &batch;

        config->numDrawn = (int)inItems.size();

        glm::mat4 projMat;
        Transform viewMat;
        args->_viewFrustum->evalProjectionMatrix(projMat);
        args->_viewFrustum->evalViewTransform(viewMat);

        batch.setProjectionTransform(projMat);
        batch.setViewTransform(viewMat);

        renderShapes(sceneContext, renderContext, _shapePlumber, inItems, _maxDrawn);
        args->_batch = nullptr;
    });
}

// TODO: Move this to the shapePlumber
gpu::PipelinePointer DrawOverlay3D::_opaquePipeline;
const gpu::PipelinePointer& DrawOverlay3D::getOpaquePipeline() {
    if (!_opaquePipeline) {
        auto vs = gpu::Shader::createVertex(std::string(overlay3D_vert));
        auto ps = gpu::Shader::createPixel(std::string(overlay3D_frag));
        auto program = gpu::Shader::createProgram(vs, ps);
        
        auto state = std::make_shared<gpu::State>();
        state->setDepthTest(false);
        // additive blending
        state->setBlendFunction(true, gpu::State::ONE, gpu::State::BLEND_OP_ADD, gpu::State::ONE);

        _opaquePipeline = gpu::Pipeline::create(program, state);
    }
    return _opaquePipeline;
}

void DrawOverlay3D::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) {
    assert(renderContext->args);
    assert(renderContext->args->_viewFrustum);

    // render backgrounds
    auto& scene = sceneContext->_scene;
    auto& items = scene->getMasterBucket().at(ItemFilter::Builder::opaqueShape().withLayered());

    auto config = std::static_pointer_cast<Config>(renderContext->jobConfig);

    ItemIDsBounds inItems;
    inItems.reserve(items.size());
    for (auto id : items) {
        auto& item = scene->getItem(id);
        if (item.getKey().isVisible() && (item.getLayer() == 1)) {
            inItems.emplace_back(id);
        }
    }
    config->numItems = (int)inItems.size();
    config->numDrawn = (int)inItems.size();

    if (!inItems.empty()) {
        RenderArgs* args = renderContext->args;

        // Clear the framebuffer without stereo
        // Needs to be distinct from the other batch because using the clear call 
        // while stereo is enabled triggers a warning
        {
            gpu::Batch batch;
            batch.enableStereo(false);
            batch.clearFramebuffer(gpu::Framebuffer::BUFFER_DEPTH, glm::vec4(), 1.f, 0, true);
            args->_context->render(batch);
        }

        // Render the items
        gpu::doInBatch(args->_context, [&](gpu::Batch& batch) {
            args->_batch = &batch;
            args->_whiteTexture = DependencyManager::get<TextureCache>()->getWhiteTexture();

            glm::mat4 projMat;
            Transform viewMat;
            args->_viewFrustum->evalProjectionMatrix(projMat);
            args->_viewFrustum->evalViewTransform(viewMat);

            batch.setProjectionTransform(projMat);
            batch.setViewTransform(viewMat);
            batch.setViewportTransform(args->_viewport);
            batch.setStateScissorRect(args->_viewport);

            batch.setPipeline(getOpaquePipeline());
            batch.setResourceTexture(0, args->_whiteTexture);
            renderShapes(sceneContext, renderContext, _shapePlumber, inItems, _maxDrawn);
        });
        args->_batch = nullptr;
        args->_whiteTexture.reset();
    }
}

gpu::PipelinePointer DrawStencilDeferred::_opaquePipeline;
const gpu::PipelinePointer& DrawStencilDeferred::getOpaquePipeline() {
    if (!_opaquePipeline) {
        const gpu::int8 STENCIL_OPAQUE = 1;
        auto vs = gpu::StandardShaderLib::getDrawUnitQuadTexcoordVS();
        auto ps = gpu::Shader::createPixel(std::string(drawOpaqueStencil_frag));
        auto program = gpu::Shader::createProgram(vs, ps);
        

        gpu::Shader::makeProgram((*program));

        auto state = std::make_shared<gpu::State>();
        state->setDepthTest(true, false, gpu::LESS_EQUAL);
        state->setStencilTest(true, 0xFF, gpu::State::StencilTest(STENCIL_OPAQUE, 0xFF, gpu::ALWAYS, gpu::State::STENCIL_OP_REPLACE, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_REPLACE)); 
        state->setColorWriteMask(0);

        _opaquePipeline = gpu::Pipeline::create(program, state);
    }
    return _opaquePipeline;
}

void DrawStencilDeferred::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) {
    assert(renderContext->args);
    assert(renderContext->args->_viewFrustum);

    // from the touched pixel generate the stencil buffer 
    RenderArgs* args = renderContext->args;
    doInBatch(args->_context, [&](gpu::Batch& batch) {
        args->_batch = &batch;

        auto deferredFboColorDepthStencil = DependencyManager::get<FramebufferCache>()->getDeferredFramebufferDepthColor();

        batch.enableStereo(false);

        batch.setFramebuffer(deferredFboColorDepthStencil);
        batch.setViewportTransform(args->_viewport);
        batch.setStateScissorRect(args->_viewport);

        batch.setPipeline(getOpaquePipeline());

        batch.draw(gpu::TRIANGLE_STRIP, 4);
        batch.setResourceTexture(0, nullptr);

    });
    args->_batch = nullptr;
}

void DrawBackgroundDeferred::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) {
    assert(renderContext->args);
    assert(renderContext->args->_viewFrustum);

    // render backgrounds
    auto& scene = sceneContext->_scene;
    auto& items = scene->getMasterBucket().at(ItemFilter::Builder::background());


    ItemIDsBounds inItems;
    inItems.reserve(items.size());
    for (auto id : items) {
        inItems.emplace_back(id);
    }
    RenderArgs* args = renderContext->args;
    doInBatch(args->_context, [&](gpu::Batch& batch) {
        args->_batch = &batch;

        auto lightingFBO = DependencyManager::get<FramebufferCache>()->getLightingFramebuffer();

        batch.enableSkybox(true);

        batch.setFramebuffer(lightingFBO);

        batch.setViewportTransform(args->_viewport);
        batch.setStateScissorRect(args->_viewport);

        glm::mat4 projMat;
        Transform viewMat;
        args->_viewFrustum->evalProjectionMatrix(projMat);
        args->_viewFrustum->evalViewTransform(viewMat);

        batch.setProjectionTransform(projMat);
        batch.setViewTransform(viewMat);

        renderItems(sceneContext, renderContext, inItems);
    });
    args->_batch = nullptr;
}

void Blit::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) {
    assert(renderContext->args);
    assert(renderContext->args->_context);

    RenderArgs* renderArgs = renderContext->args;
    auto blitFbo = renderArgs->_blitFramebuffer;

    if (!blitFbo) {
        return;
    }

    // Determine size from viewport
    int width = renderArgs->_viewport.z;
    int height = renderArgs->_viewport.w;

    // Blit primary to blit FBO
    auto framebufferCache = DependencyManager::get<FramebufferCache>();
    auto primaryFbo = framebufferCache->getPrimaryFramebuffer();

    gpu::doInBatch(renderArgs->_context, [&](gpu::Batch& batch) {
        batch.setFramebuffer(blitFbo);

        if (renderArgs->_renderMode == RenderArgs::MIRROR_RENDER_MODE) {
            if (renderArgs->_context->isStereo()) {
                gpu::Vec4i srcRectLeft;
                srcRectLeft.z = width / 2;
                srcRectLeft.w = height;

                gpu::Vec4i srcRectRight;
                srcRectRight.x = width / 2;
                srcRectRight.z = width;
                srcRectRight.w = height;

                gpu::Vec4i destRectLeft;
                destRectLeft.x = srcRectLeft.z;
                destRectLeft.z = srcRectLeft.x;
                destRectLeft.y = srcRectLeft.y;
                destRectLeft.w = srcRectLeft.w;

                gpu::Vec4i destRectRight;
                destRectRight.x = srcRectRight.z;
                destRectRight.z = srcRectRight.x;
                destRectRight.y = srcRectRight.y;
                destRectRight.w = srcRectRight.w;

                // Blit left to right and right to left in stereo
                batch.blit(primaryFbo, srcRectRight, blitFbo, destRectLeft);
                batch.blit(primaryFbo, srcRectLeft, blitFbo, destRectRight);
            } else {
                gpu::Vec4i srcRect;
                srcRect.z = width;
                srcRect.w = height;

                gpu::Vec4i destRect;
                destRect.x = width;
                destRect.y = 0;
                destRect.z = 0;
                destRect.w = height;

                batch.blit(primaryFbo, srcRect, blitFbo, destRect);
            }
        } else {
            gpu::Vec4i rect;
            rect.z = width;
            rect.w = height;

            batch.blit(primaryFbo, rect, blitFbo, rect);
        }
    });
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

