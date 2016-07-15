
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
#include <RenderArgs.h>
#include <ViewFrustum.h>
#include <gpu/Context.h>

#include <render/CullTask.h>
#include <render/SortTask.h>
#include <render/DrawTask.h>
#include <render/DrawStatus.h>
#include <render/DrawSceneOctree.h>
#include <render/BlurTask.h>

#include "LightingModel.h"
#include "DebugDeferredBuffer.h"
#include "DeferredFramebuffer.h"
#include "DeferredLightingEffect.h"
#include "SurfaceGeometryPass.h"
#include "FramebufferCache.h"
#include "HitEffect.h"
#include "TextureCache.h"

#include "AmbientOcclusionEffect.h"
#include "AntialiasingEffect.h"
#include "ToneMappingEffect.h"
#include "SubsurfaceScattering.h"

#include <gpu/StandardShaderLib.h>

#include "drawOpaqueStencil_frag.h"


using namespace render;
extern void initOverlay3DPipelines(render::ShapePlumber& plumber);
extern void initDeferredPipelines(render::ShapePlumber& plumber);

RenderDeferredTask::RenderDeferredTask(CullFunctor cullFunctor) {
    cullFunctor = cullFunctor ? cullFunctor : [](const RenderArgs*, const AABox&){ return true; };

    // Prepare the ShapePipelines
    ShapePlumberPointer shapePlumber = std::make_shared<ShapePlumber>();
    initDeferredPipelines(*shapePlumber);

    // CPU jobs:
    // Fetch and cull the items from the scene
    auto spatialFilter = ItemFilter::Builder::visibleWorldItems().withoutLayered();
    const auto spatialSelection = addJob<FetchSpatialTree>("FetchSceneSelection", spatialFilter);
    const auto culledSpatialSelection = addJob<CullSpatialSelection>("CullSceneSelection", spatialSelection, cullFunctor, RenderDetails::ITEM, spatialFilter);

    // Overlays are not culled
    const auto nonspatialSelection = addJob<FetchNonspatialItems>("FetchOverlaySelection");

    // Multi filter visible items into different buckets
    const int NUM_FILTERS = 3;
    const int OPAQUE_SHAPE_BUCKET = 0;
    const int TRANSPARENT_SHAPE_BUCKET = 1;
    const int LIGHT_BUCKET = 2;
    const int BACKGROUND_BUCKET = 2;
    MultiFilterItem<NUM_FILTERS>::ItemFilterArray spatialFilters = { {
            ItemFilter::Builder::opaqueShape(),
            ItemFilter::Builder::transparentShape(),
            ItemFilter::Builder::light()
    } };
    MultiFilterItem<NUM_FILTERS>::ItemFilterArray nonspatialFilters = { {
            ItemFilter::Builder::opaqueShape(),
            ItemFilter::Builder::transparentShape(),
            ItemFilter::Builder::background()
    } };
    const auto filteredSpatialBuckets = addJob<MultiFilterItem<NUM_FILTERS>>("FilterSceneSelection", culledSpatialSelection, spatialFilters).get<MultiFilterItem<NUM_FILTERS>::ItemBoundsArray>();
    const auto filteredNonspatialBuckets = addJob<MultiFilterItem<NUM_FILTERS>>("FilterOverlaySelection", nonspatialSelection, nonspatialFilters).get<MultiFilterItem<NUM_FILTERS>::ItemBoundsArray>();

    // Extract / Sort opaques / Transparents / Lights / Overlays
    const auto opaques = addJob<DepthSortItems>("DepthSortOpaque", filteredSpatialBuckets[OPAQUE_SHAPE_BUCKET]);
    const auto transparents = addJob<DepthSortItems>("DepthSortTransparent", filteredSpatialBuckets[TRANSPARENT_SHAPE_BUCKET], DepthSortItems(false));
    const auto lights = filteredSpatialBuckets[LIGHT_BUCKET];

    const auto overlayOpaques = addJob<DepthSortItems>("DepthSortOverlayOpaque", filteredNonspatialBuckets[OPAQUE_SHAPE_BUCKET]);
    const auto overlayTransparents = addJob<DepthSortItems>("DepthSortOverlayTransparent", filteredNonspatialBuckets[TRANSPARENT_SHAPE_BUCKET], DepthSortItems(false));
    const auto background = filteredNonspatialBuckets[BACKGROUND_BUCKET];

    // Prepare deferred, generate the shared Deferred Frame Transform
    const auto deferredFrameTransform = addJob<GenerateDeferredFrameTransform>("DeferredFrameTransform");
    const auto lightingModel = addJob<MakeLightingModel>("LightingModel");


    // GPU jobs: Start preparing the primary, deferred and lighting buffer
    const auto primaryFramebuffer = addJob<PreparePrimaryFramebuffer>("PreparePrimaryBuffer");

    
    const auto prepareDeferredInputs = SurfaceGeometryPass::Inputs(primaryFramebuffer, lightingModel).hasVarying();
    const auto prepareDeferredOutputs = addJob<PrepareDeferred>("PrepareDeferred", prepareDeferredInputs);
    const auto deferredFramebuffer = prepareDeferredOutputs.getN<PrepareDeferred::Outputs>(0);
    const auto lightingFramebuffer = prepareDeferredOutputs.getN<PrepareDeferred::Outputs>(1);

    // Render opaque objects in DeferredBuffer
    const auto opaqueInputs = DrawStateSortDeferred::Inputs(opaques, lightingModel).hasVarying();
    addJob<DrawStateSortDeferred>("DrawOpaqueDeferred", opaqueInputs, shapePlumber);

    // Once opaque is all rendered create stencil background
    addJob<DrawStencilDeferred>("DrawOpaqueStencil", deferredFramebuffer);


    // Opaque all rendered, generate surface geometry buffers
    const auto surfaceGeometryPassInputs = SurfaceGeometryPass::Inputs(deferredFrameTransform, deferredFramebuffer).hasVarying();
    const auto surfaceGeometryPassOutputs = addJob<SurfaceGeometryPass>("SurfaceGeometry", surfaceGeometryPassInputs);
    const auto surfaceGeometryFramebuffer = surfaceGeometryPassOutputs.getN<SurfaceGeometryPass::Outputs>(0);
    const auto curvatureFramebuffer = surfaceGeometryPassOutputs.getN<SurfaceGeometryPass::Outputs>(1);
    const auto linearDepthTexture = surfaceGeometryPassOutputs.getN<SurfaceGeometryPass::Outputs>(2);

    const auto rangeTimer = addJob<BeginTimerGPU>("BeginTimerRange");

    // TODO: Push this 2 diffusion stages into surfaceGeometryPass as they are working together
    const auto diffuseCurvaturePassInputs = BlurGaussianDepthAware::Inputs(curvatureFramebuffer, linearDepthTexture).hasVarying();
    const auto midCurvatureNormalFramebuffer = addJob<render::BlurGaussianDepthAware>("DiffuseCurvatureMid", diffuseCurvaturePassInputs);
    const auto lowCurvatureNormalFramebuffer = addJob<render::BlurGaussianDepthAware>("DiffuseCurvatureLow", diffuseCurvaturePassInputs, true); // THis blur pass generates it s render resource

    const auto scatteringResource = addJob<SubsurfaceScattering>("Scattering");

    // AO job
    addJob<AmbientOcclusionEffect>("AmbientOcclusion");

    // Draw Lights just add the lights to the current list of lights to deal with. NOt really gpu job for now.
    addJob<DrawLight>("DrawLight", lights);

    const auto deferredLightingInputs = RenderDeferred::Inputs(deferredFrameTransform, deferredFramebuffer, lightingModel,
        surfaceGeometryFramebuffer, lowCurvatureNormalFramebuffer, scatteringResource).hasVarying();

    // DeferredBuffer is complete, now let's shade it into the LightingBuffer
    addJob<RenderDeferred>("RenderDeferred", deferredLightingInputs);

    // Use Stencil and draw background in Lighting buffer to complete filling in the opaque
    const auto backgroundInputs = DrawBackgroundDeferred::Inputs(background, lightingModel).hasVarying();
    addJob<DrawBackgroundDeferred>("DrawBackgroundDeferred", backgroundInputs);

    // Render transparent objects forward in LightingBuffer
    const auto transparentsInputs = DrawDeferred::Inputs(transparents, lightingModel).hasVarying();
    addJob<DrawDeferred>("DrawTransparentDeferred", transparentsInputs, shapePlumber);

    // Lighting Buffer ready for tone mapping
    const auto toneMappingInputs = render::Varying(ToneMappingDeferred::Inputs(lightingFramebuffer, primaryFramebuffer));
    addJob<ToneMappingDeferred>("ToneMapping", toneMappingInputs);

    // Overlays
    const auto overlayOpaquesInputs = DrawOverlay3D::Inputs(overlayOpaques, lightingModel).hasVarying();
    const auto overlayTransparentsInputs = DrawOverlay3D::Inputs(overlayTransparents, lightingModel).hasVarying();
    addJob<DrawOverlay3D>("DrawOverlay3DOpaque", overlayOpaquesInputs, true);
    addJob<DrawOverlay3D>("DrawOverlay3DTransparent", overlayTransparentsInputs, false);

    
    // Debugging stages
    {
        addJob<DebugSubsurfaceScattering>("DebugScattering", deferredLightingInputs);

        // Debugging Deferred buffer job
        const auto debugFramebuffers = render::Varying(DebugDeferredBuffer::Inputs(deferredFramebuffer, surfaceGeometryFramebuffer, lowCurvatureNormalFramebuffer));
        addJob<DebugDeferredBuffer>("DebugDeferredBuffer", debugFramebuffers);

        // Scene Octree Debuging job
        {
            addJob<DrawSceneOctree>("DrawSceneOctree", spatialSelection);
            addJob<DrawItemSelection>("DrawItemSelection", spatialSelection);
        }

        // Status icon rendering job
        {
            // Grab a texture map representing the different status icons and assign that to the drawStatsuJob
            auto iconMapPath = PathUtils::resourcesPath() + "icons/statusIconAtlas.svg";
            auto statusIconMap = DependencyManager::get<TextureCache>()->getImageTexture(iconMapPath);
            addJob<DrawStatus>("DrawStatus", opaques, DrawStatus(statusIconMap));
        }
    }


    // AA job to be revisited
    addJob<Antialiasing>("Antialiasing", primaryFramebuffer);

    addJob<EndTimerGPU>("RangeTimer", rangeTimer);
    // Blit!
    addJob<Blit>("Blit", primaryFramebuffer);
    
    
}

void RenderDeferredTask::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) {
    // sanity checks
    assert(sceneContext);
    if (!sceneContext->_scene) {
        return;
    }


    // Is it possible that we render without a viewFrustum ?
    if (!(renderContext->args && renderContext->args->hasViewFrustum())) {
        return;
    }
    RenderArgs* args = renderContext->args;
    auto config = std::static_pointer_cast<Config>(renderContext->jobConfig);

   /* gpu::doInBatch(args->_context, [&](gpu::Batch& batch) {
         _gpuTimer.begin(batch);
    });*/

    for (auto job : _jobs) {
        job.run(sceneContext, renderContext);
    }

    /*gpu::doInBatch(args->_context, [&](gpu::Batch& batch) {
         _gpuTimer.end(batch);
    });*/

//    config->gpuTime = _gpuTimer.getAverage();
    
}

void BeginTimerGPU::run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext, gpu::RangeTimerPointer& timer) {
    timer = _gpuTimer;
    gpu::doInBatch(renderContext->args->_context, [&](gpu::Batch& batch) {
        _gpuTimer->begin(batch);
    });
}

void EndTimerGPU::run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext, const gpu::RangeTimerPointer& timer) {
    gpu::doInBatch(renderContext->args->_context, [&](gpu::Batch& batch) {
        timer->end(batch);
    });
    
    auto config = std::static_pointer_cast<Config>(renderContext->jobConfig);
    config->gpuTime = timer->getAverage();
}


void DrawDeferred::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const Inputs& inputs) {
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

        renderShapes(sceneContext, renderContext, _shapePlumber, inItems, _maxDrawn);
        args->_batch = nullptr;
    });

    config->setNumDrawn((int)inItems.size());
}

void DrawStateSortDeferred::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const Inputs& inputs) {
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

        if (_stateSort) {
            renderStateSortShapes(sceneContext, renderContext, _shapePlumber, inItems, _maxDrawn);
        } else {
            renderShapes(sceneContext, renderContext, _shapePlumber, inItems, _maxDrawn);
        }
        args->_batch = nullptr;
    });

    config->setNumDrawn((int)inItems.size());
}

DrawOverlay3D::DrawOverlay3D(bool opaque) :
    _shapePlumber(std::make_shared<ShapePlumber>()),
    _opaquePass(opaque) {
    initOverlay3DPipelines(*_shapePlumber);
}

void DrawOverlay3D::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const Inputs& inputs) {
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
            gpu::Batch batch;
            batch.enableStereo(false);
            batch.clearFramebuffer(gpu::Framebuffer::BUFFER_DEPTH, glm::vec4(), 1.f, 0, true);
            args->_context->render(batch);
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

            renderShapes(sceneContext, renderContext, _shapePlumber, inItems, _maxDrawn);
            args->_batch = nullptr;
        });
    }
}


gpu::PipelinePointer DrawStencilDeferred::getOpaquePipeline() {
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

void DrawStencilDeferred::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const DeferredFramebufferPointer& deferredFramebuffer) {
    assert(renderContext->args);
    assert(renderContext->args->hasViewFrustum());

    // from the touched pixel generate the stencil buffer 
    RenderArgs* args = renderContext->args;
    doInBatch(args->_context, [&](gpu::Batch& batch) {
        args->_batch = &batch;

        auto deferredFboColorDepthStencil = deferredFramebuffer->getDeferredFramebufferDepthColor();
        

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

void DrawBackgroundDeferred::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const Inputs& inputs) {
    assert(renderContext->args);
    assert(renderContext->args->hasViewFrustum());

    const auto& inItems = inputs.get0();
    const auto& lightingModel = inputs.get1();
    if (!lightingModel->isBackgroundEnabled()) {
        return;
    }

    RenderArgs* args = renderContext->args;
    doInBatch(args->_context, [&](gpu::Batch& batch) {
        args->_batch = &batch;
    //    _gpuTimer.begin(batch);

        batch.enableSkybox(true);
        
        batch.setViewportTransform(args->_viewport);
        batch.setStateScissorRect(args->_viewport);

        glm::mat4 projMat;
        Transform viewMat;
        args->getViewFrustum().evalProjectionMatrix(projMat);
        args->getViewFrustum().evalViewTransform(viewMat);

        batch.setProjectionTransform(projMat);
        batch.setViewTransform(viewMat);

        renderItems(sceneContext, renderContext, inItems);
     //   _gpuTimer.end(batch);
    });
    args->_batch = nullptr;

   // std::static_pointer_cast<Config>(renderContext->jobConfig)->gpuTime = _gpuTimer.getAverage();
}

void Blit::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const gpu::FramebufferPointer& srcFramebuffer) {
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
