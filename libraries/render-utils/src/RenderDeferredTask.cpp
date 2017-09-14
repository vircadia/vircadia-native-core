
//
//  RenderDeferredTask.cpp
//  render-utils/src/
//
//  Created by Sam Gateau on 5/29/15.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "RenderDeferredTask.h"

#include <PerfStat.h>
#include <PathUtils.h>
#include <ViewFrustum.h>
#include <gpu/Context.h>

#include <render/CullTask.h>
#include <render/FilterTask.h>
#include <render/SortTask.h>
#include <render/DrawTask.h>
#include <render/DrawStatus.h>
#include <render/DrawSceneOctree.h>
#include <render/BlurTask.h>

#include "LightingModel.h"
#include "StencilMaskPass.h"
#include "DebugDeferredBuffer.h"
#include "DeferredFramebuffer.h"
#include "DeferredLightingEffect.h"
#include "SurfaceGeometryPass.h"
#include "FramebufferCache.h"
#include "TextureCache.h"
#include "ZoneRenderer.h"
#include "FadeEffect.h"

#include "AmbientOcclusionEffect.h"
#include "AntialiasingEffect.h"
#include "ToneMappingEffect.h"
#include "SubsurfaceScattering.h"
#include "OutlineEffect.h"

#include <gpu/StandardShaderLib.h>


using namespace render;
extern void initOverlay3DPipelines(render::ShapePlumber& plumber, bool depthTest = false);
extern void initDeferredPipelines(render::ShapePlumber& plumber, const render::ShapePipeline::BatchSetter& batchSetter, const render::ShapePipeline::ItemSetter& itemSetter);

RenderDeferredTask::RenderDeferredTask() {
    DependencyManager::set<FadeEffect>();
}

void RenderDeferredTask::configure(const Config& config)
{
}

void RenderDeferredTask::build(JobModel& task, const render::Varying& input, render::Varying& output) {
    const auto& items = input.get<Input>();
    auto fadeEffect = DependencyManager::get<FadeEffect>();

    // Prepare the ShapePipelines
    ShapePlumberPointer shapePlumber = std::make_shared<ShapePlumber>();
    initDeferredPipelines(*shapePlumber, fadeEffect->getBatchSetter(), fadeEffect->getItemUniformSetter());

    // Extract opaques / transparents / lights / metas / overlays / background
    const auto& opaques = items.get0()[RenderFetchCullSortTask::OPAQUE_SHAPE];
    const auto& transparents = items.get0()[RenderFetchCullSortTask::TRANSPARENT_SHAPE];
    const auto& lights = items.get0()[RenderFetchCullSortTask::LIGHT];
    const auto& metas = items.get0()[RenderFetchCullSortTask::META];
    const auto& overlayOpaques = items.get0()[RenderFetchCullSortTask::OVERLAY_OPAQUE_SHAPE];
    const auto& overlayTransparents = items.get0()[RenderFetchCullSortTask::OVERLAY_TRANSPARENT_SHAPE];
    //const auto& background = items.get0()[RenderFetchCullSortTask::BACKGROUND];
    const auto& spatialSelection = items[1];

    fadeEffect->build(task, opaques);

    // Prepare deferred, generate the shared Deferred Frame Transform
    const auto deferredFrameTransform = task.addJob<GenerateDeferredFrameTransform>("DeferredFrameTransform");
    const auto lightingModel = task.addJob<MakeLightingModel>("LightingModel");

    // GPU jobs: Start preparing the primary, deferred and lighting buffer
    const auto primaryFramebuffer = task.addJob<PreparePrimaryFramebuffer>("PreparePrimaryBuffer");

    const auto opaqueRangeTimer = task.addJob<BeginGPURangeTimer>("BeginOpaqueRangeTimer", "DrawOpaques");

    const auto prepareDeferredInputs = PrepareDeferred::Inputs(primaryFramebuffer, lightingModel).asVarying();
    const auto prepareDeferredOutputs = task.addJob<PrepareDeferred>("PrepareDeferred", prepareDeferredInputs);
    const auto deferredFramebuffer = prepareDeferredOutputs.getN<PrepareDeferred::Outputs>(0);
    const auto lightingFramebuffer = prepareDeferredOutputs.getN<PrepareDeferred::Outputs>(1);

    // draw a stencil mask in hidden regions of the framebuffer.
    task.addJob<PrepareStencil>("PrepareStencil", primaryFramebuffer);

    // Select items that need to be outlined
    const auto selectionName = "contextOverlayHighlightList";
    const auto selectMetaInput = SelectItems::Inputs(metas, Varying()).asVarying();
    const auto selectedMetas = task.addJob<SelectItems>("PassTestMetaSelection", selectMetaInput, selectionName);
    const auto selectMetaAndOpaqueInput = SelectItems::Inputs(opaques, selectedMetas).asVarying();
    const auto selectedMetasAndOpaques = task.addJob<SelectItems>("PassTestOpaqueSelection", selectMetaAndOpaqueInput, selectionName);
    const auto selectItemInput = SelectItems::Inputs(transparents, selectedMetasAndOpaques).asVarying();
    const auto selectedItems = task.addJob<SelectItems>("PassTestTransparentSelection", selectItemInput, selectionName);

    // Render opaque objects in DeferredBuffer
    const auto opaqueInputs = DrawStateSortDeferred::Inputs(opaques, lightingModel).asVarying();
    task.addJob<DrawStateSortDeferred>("DrawOpaqueDeferred", opaqueInputs, shapePlumber);

    task.addJob<EndGPURangeTimer>("OpaqueRangeTimer", opaqueRangeTimer);


    // Opaque all rendered

    // Linear Depth Pass
    const auto linearDepthPassInputs = LinearDepthPass::Inputs(deferredFrameTransform, deferredFramebuffer).asVarying();
    const auto linearDepthPassOutputs = task.addJob<LinearDepthPass>("LinearDepth", linearDepthPassInputs);
    const auto linearDepthTarget = linearDepthPassOutputs.getN<LinearDepthPass::Outputs>(0);
    
    // Curvature pass
    const auto surfaceGeometryPassInputs = SurfaceGeometryPass::Inputs(deferredFrameTransform, deferredFramebuffer, linearDepthTarget).asVarying();
    const auto surfaceGeometryPassOutputs = task.addJob<SurfaceGeometryPass>("SurfaceGeometry", surfaceGeometryPassInputs);
    const auto surfaceGeometryFramebuffer = surfaceGeometryPassOutputs.getN<SurfaceGeometryPass::Outputs>(0);
    const auto curvatureFramebuffer = surfaceGeometryPassOutputs.getN<SurfaceGeometryPass::Outputs>(1);
    const auto midCurvatureNormalFramebuffer = surfaceGeometryPassOutputs.getN<SurfaceGeometryPass::Outputs>(2);
    const auto lowCurvatureNormalFramebuffer = surfaceGeometryPassOutputs.getN<SurfaceGeometryPass::Outputs>(3);

    // Simply update the scattering resource
    const auto scatteringResource = task.addJob<SubsurfaceScattering>("Scattering");

    // AO job
    const auto ambientOcclusionInputs = AmbientOcclusionEffect::Inputs(deferredFrameTransform, deferredFramebuffer, linearDepthTarget).asVarying();
    const auto ambientOcclusionOutputs = task.addJob<AmbientOcclusionEffect>("AmbientOcclusion", ambientOcclusionInputs);
    const auto ambientOcclusionFramebuffer = ambientOcclusionOutputs.getN<AmbientOcclusionEffect::Outputs>(0);
    const auto ambientOcclusionUniforms = ambientOcclusionOutputs.getN<AmbientOcclusionEffect::Outputs>(1);

    
    // Draw Lights just add the lights to the current list of lights to deal with. NOt really gpu job for now.
    task.addJob<DrawLight>("DrawLight", lights);

    // Filter zones from the general metas bucket
    const auto zones = task.addJob<ZoneRendererTask>("ZoneRenderer", metas);

    // Light Clustering
    // Create the cluster grid of lights, cpu job for now
    const auto lightClusteringPassInputs = LightClusteringPass::Inputs(deferredFrameTransform, lightingModel, linearDepthTarget).asVarying();
    const auto lightClusters = task.addJob<LightClusteringPass>("LightClustering", lightClusteringPassInputs);
    
    
    // DeferredBuffer is complete, now let's shade it into the LightingBuffer
    const auto deferredLightingInputs = RenderDeferred::Inputs(deferredFrameTransform, deferredFramebuffer, lightingModel,
        surfaceGeometryFramebuffer, ambientOcclusionFramebuffer, scatteringResource, lightClusters).asVarying();
    
    task.addJob<RenderDeferred>("RenderDeferred", deferredLightingInputs);

    // Similar to light stage, background stage has been filled by several potential render items and resolved for the frame in this job
    task.addJob<DrawBackgroundStage>("DrawBackgroundDeferred", lightingModel);

    // Render transparent objects forward in LightingBuffer
    const auto transparentsInputs = DrawDeferred::Inputs(transparents, lightingModel).asVarying();
    task.addJob<DrawDeferred>("DrawTransparentDeferred", transparentsInputs, shapePlumber);

    // LIght Cluster Grid Debuging job
    {
        const auto debugLightClustersInputs = DebugLightClusters::Inputs(deferredFrameTransform, deferredFramebuffer, lightingModel, linearDepthTarget, lightClusters).asVarying();
        task.addJob<DebugLightClusters>("DebugLightClusters", debugLightClustersInputs);
    }
    
    const auto toneAndPostRangeTimer = task.addJob<BeginGPURangeTimer>("BeginToneAndPostRangeTimer", "PostToneOverlaysAntialiasing");

    // Lighting Buffer ready for tone mapping
    const auto toneMappingInputs = ToneMappingDeferred::Inputs(lightingFramebuffer, primaryFramebuffer).asVarying();
    task.addJob<ToneMappingDeferred>("ToneMapping", toneMappingInputs);

    const auto outlineRangeTimer = task.addJob<BeginGPURangeTimer>("BeginOutlineRangeTimer", "Outline");
    const auto outlineInputs = DrawOutlineTask::Inputs(selectedItems, shapePlumber, deferredFramebuffer, primaryFramebuffer, deferredFrameTransform).asVarying();
    task.addJob<DrawOutlineTask>("DrawOutline", outlineInputs);
    task.addJob<EndGPURangeTimer>("EndOutlineRangeTimer", outlineRangeTimer);

    { // DEbug the bounds of the rendered items, still look at the zbuffer
        task.addJob<DrawBounds>("DrawMetaBounds", metas);
        task.addJob<DrawBounds>("DrawOpaqueBounds", opaques);
        task.addJob<DrawBounds>("DrawTransparentBounds", transparents);
    
        task.addJob<DrawBounds>("DrawLightBounds", lights);
        task.addJob<DrawBounds>("DrawZones", zones);
    }

    // Overlays
    const auto overlayOpaquesInputs = DrawOverlay3D::Inputs(overlayOpaques, lightingModel).asVarying();
    const auto overlayTransparentsInputs = DrawOverlay3D::Inputs(overlayTransparents, lightingModel).asVarying();
    task.addJob<DrawOverlay3D>("DrawOverlay3DOpaque", overlayOpaquesInputs, true);
    task.addJob<DrawOverlay3D>("DrawOverlay3DTransparent", overlayTransparentsInputs, false);

    { // Debug the bounds of the rendered Overlay items, still look at the zbuffer
        task.addJob<DrawBounds>("DrawOverlayOpaqueBounds", overlayOpaques);
        task.addJob<DrawBounds>("DrawOverlayTransparentBounds", overlayTransparents);
    }

     // Debugging stages
    {
        // Debugging Deferred buffer job
        const auto debugFramebuffers = render::Varying(DebugDeferredBuffer::Inputs(deferredFramebuffer, linearDepthTarget, surfaceGeometryFramebuffer, ambientOcclusionFramebuffer));
        task.addJob<DebugDeferredBuffer>("DebugDeferredBuffer", debugFramebuffers);

        const auto debugSubsurfaceScatteringInputs = DebugSubsurfaceScattering::Inputs(deferredFrameTransform, deferredFramebuffer, lightingModel,
            surfaceGeometryFramebuffer, ambientOcclusionFramebuffer, scatteringResource).asVarying();
        task.addJob<DebugSubsurfaceScattering>("DebugScattering", debugSubsurfaceScatteringInputs);

        const auto debugAmbientOcclusionInputs = DebugAmbientOcclusion::Inputs(deferredFrameTransform, deferredFramebuffer, linearDepthTarget, ambientOcclusionUniforms).asVarying();
        task.addJob<DebugAmbientOcclusion>("DebugAmbientOcclusion", debugAmbientOcclusionInputs);

        // Scene Octree Debugging job
        {
            task.addJob<DrawSceneOctree>("DrawSceneOctree", spatialSelection);
            task.addJob<DrawItemSelection>("DrawItemSelection", spatialSelection);
        }

        // Status icon rendering job
        {
            // Grab a texture map representing the different status icons and assign that to the drawStatsuJob
            auto iconMapPath = PathUtils::resourcesPath() + "icons/statusIconAtlas.svg";
            auto statusIconMap = DependencyManager::get<TextureCache>()->getImageTexture(iconMapPath, image::TextureUsage::STRICT_TEXTURE);
            task.addJob<DrawStatus>("DrawStatus", opaques, DrawStatus(statusIconMap));
        }

        task.addJob<DebugZoneLighting>("DrawZoneStack", deferredFrameTransform);
        
        // Render.getConfig("RenderMainView.DrawSelectionBounds").enabled = true
        task.addJob<DrawBounds>("DrawSelectionBounds", selectedItems);
    }

    // AA job to be revisited
    task.addJob<Antialiasing>("Antialiasing", primaryFramebuffer);

    task.addJob<EndGPURangeTimer>("ToneAndPostRangeTimer", toneAndPostRangeTimer);

    // Blit!
    task.addJob<Blit>("Blit", primaryFramebuffer);
}

void BeginGPURangeTimer::run(const render::RenderContextPointer& renderContext, gpu::RangeTimerPointer& timer) {
    timer = _gpuTimer;
    gpu::doInBatch(renderContext->args->_context, [&](gpu::Batch& batch) {
        _gpuTimer->begin(batch);
    });
}

void EndGPURangeTimer::run(const render::RenderContextPointer& renderContext, const gpu::RangeTimerPointer& timer) {
    gpu::doInBatch(renderContext->args->_context, [&](gpu::Batch& batch) {
        timer->end(batch);
    });
    
    auto config = std::static_pointer_cast<Config>(renderContext->jobConfig);
    config->setGPUBatchRunTime(timer->getGPUAverage(), timer->getBatchAverage());
}


void DrawDeferred::run(const RenderContextPointer& renderContext, const Inputs& inputs) {
    assert(renderContext->args);
    assert(renderContext->args->hasViewFrustum());

    auto config = std::static_pointer_cast<Config>(renderContext->jobConfig);

    const auto& inItems = inputs.get0();
    const auto& lightingModel = inputs.get1();

    RenderArgs* args = renderContext->args;

    gpu::doInBatch(args->_context, [&](gpu::Batch& batch) {
        args->_batch = &batch;
        
        // Setup camera, projection and viewport for all items
        batch.setViewportTransform(args->_viewport);
        batch.setStateScissorRect(args->_viewport);

        glm::mat4 projMat;
        Transform viewMat;
        args->getViewFrustum().evalProjectionMatrix(projMat);
        args->getViewFrustum().evalViewTransform(viewMat);

        batch.setProjectionTransform(projMat);
        batch.setViewTransform(viewMat);

        // Setup lighting model for all items;
        batch.setUniformBuffer(render::ShapePipeline::Slot::LIGHTING_MODEL, lightingModel->getParametersBuffer());

        // From the lighting model define a global shapKey ORED with individiual keys
        ShapeKey::Builder keyBuilder;
        if (lightingModel->isWireframeEnabled()) {
            keyBuilder.withWireframe();
        }

        ShapeKey globalKey = keyBuilder.build();
        args->_globalShapeKey = globalKey._flags.to_ulong();

        renderShapes(renderContext, _shapePlumber, inItems, _maxDrawn, globalKey);

        args->_batch = nullptr;
        args->_globalShapeKey = 0;
    });

    config->setNumDrawn((int)inItems.size());
}

void DrawStateSortDeferred::run(const RenderContextPointer& renderContext, const Inputs& inputs) {
    assert(renderContext->args);
    assert(renderContext->args->hasViewFrustum());

    auto config = std::static_pointer_cast<Config>(renderContext->jobConfig);

    const auto& inItems = inputs.get0();
    const auto& lightingModel = inputs.get1();

    RenderArgs* args = renderContext->args;

    gpu::doInBatch(args->_context, [&](gpu::Batch& batch) {
        args->_batch = &batch;

        // Setup camera, projection and viewport for all items
        batch.setViewportTransform(args->_viewport);
        batch.setStateScissorRect(args->_viewport);

        glm::mat4 projMat;
        Transform viewMat;
        args->getViewFrustum().evalProjectionMatrix(projMat);
        args->getViewFrustum().evalViewTransform(viewMat);

        batch.setProjectionTransform(projMat);
        batch.setViewTransform(viewMat);

        // Setup lighting model for all items;
        batch.setUniformBuffer(render::ShapePipeline::Slot::LIGHTING_MODEL, lightingModel->getParametersBuffer());

        // From the lighting model define a global shapeKey ORED with individiual keys
        ShapeKey::Builder keyBuilder;
        if (lightingModel->isWireframeEnabled()) {
            keyBuilder.withWireframe();
        }

        ShapeKey globalKey = keyBuilder.build();
        args->_globalShapeKey = globalKey._flags.to_ulong();

        if (_stateSort) {
            renderStateSortShapes(renderContext, _shapePlumber, inItems, _maxDrawn, globalKey);
        } else {
            renderShapes(renderContext, _shapePlumber, inItems, _maxDrawn, globalKey);
        }
        args->_batch = nullptr;
        args->_globalShapeKey = 0;
    });

    config->setNumDrawn((int)inItems.size());
}

DrawOverlay3D::DrawOverlay3D(bool opaque) :
    _shapePlumber(std::make_shared<ShapePlumber>()),
    _opaquePass(opaque) {
    initOverlay3DPipelines(*_shapePlumber);
}

void DrawOverlay3D::run(const RenderContextPointer& renderContext, const Inputs& inputs) {
    assert(renderContext->args);
    assert(renderContext->args->hasViewFrustum());

    auto config = std::static_pointer_cast<Config>(renderContext->jobConfig);

    const auto& inItems = inputs.get0();
    const auto& lightingModel = inputs.get1();
    
    config->setNumDrawn((int)inItems.size());
    emit config->numDrawnChanged();

    if (!inItems.empty()) {
        RenderArgs* args = renderContext->args;

        // Clear the framebuffer without stereo
        // Needs to be distinct from the other batch because using the clear call 
        // while stereo is enabled triggers a warning
        if (_opaquePass) {
            gpu::doInBatch(args->_context, [&](gpu::Batch& batch){
                batch.enableStereo(false);
                batch.clearFramebuffer(gpu::Framebuffer::BUFFER_DEPTH, glm::vec4(), 1.f, 0, false);
            });
        }

        // Render the items
        gpu::doInBatch(args->_context, [&](gpu::Batch& batch) {
            args->_batch = &batch;
            batch.setViewportTransform(args->_viewport);
            batch.setStateScissorRect(args->_viewport);

            glm::mat4 projMat;
            Transform viewMat;
            args->getViewFrustum().evalProjectionMatrix(projMat);
            args->getViewFrustum().evalViewTransform(viewMat);

            batch.setProjectionTransform(projMat);
            batch.setViewTransform(viewMat);

            // Setup lighting model for all items;
            batch.setUniformBuffer(render::ShapePipeline::Slot::LIGHTING_MODEL, lightingModel->getParametersBuffer());

            renderShapes(renderContext, _shapePlumber, inItems, _maxDrawn);
            args->_batch = nullptr;
        });
    }
}

void Blit::run(const RenderContextPointer& renderContext, const gpu::FramebufferPointer& srcFramebuffer) {
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
    auto primaryFbo = srcFramebuffer;

    gpu::doInBatch(renderArgs->_context, [&](gpu::Batch& batch) {
        batch.setFramebuffer(blitFbo);

        if (renderArgs->_renderMode == RenderArgs::MIRROR_RENDER_MODE) {
            if (renderArgs->isStereo()) {
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

