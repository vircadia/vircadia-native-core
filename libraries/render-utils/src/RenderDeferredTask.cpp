
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
#include "RenderDeferredTask.h"

#include <PerfStat.h>
#include <RenderArgs.h>
#include <ViewFrustum.h>
#include <gpu/Context.h>

#include "FramebufferCache.h"
#include "DeferredLightingEffect.h"
#include "TextureCache.h"
#include "HitEffect.h"

#include "render/DrawStatus.h"
#include "AmbientOcclusionEffect.h"
#include "AntialiasingEffect.h"

#include "overlay3D_vert.h"
#include "overlay3D_frag.h"

using namespace render;

void SetupDeferred::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) {
    RenderArgs* args = renderContext->args;
    gpu::doInBatch(args->_context, [=](gpu::Batch& batch) {

        auto primaryFbo = DependencyManager::get<FramebufferCache>()->getPrimaryFramebufferDepthColor();

        batch.enableStereo(false);
        batch.setFramebuffer(nullptr);
        batch.setFramebuffer(primaryFbo);
 
        batch.setViewportTransform(args->_viewport);
        batch.setStateScissorRect(args->_viewport);

        batch.clearFramebuffer(
            gpu::Framebuffer::BUFFER_COLOR0 |
            gpu::Framebuffer::BUFFER_DEPTH,
            vec4(vec3(0), 1), 1.0, 0.0, true);
    });
}

void PrepareDeferred::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) {
    DependencyManager::get<DeferredLightingEffect>()->prepare(renderContext->args);
}

void RenderDeferred::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) {
    DependencyManager::get<DeferredLightingEffect>()->render(renderContext->args);
}

void ResolveDeferred::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) {
    PerformanceTimer perfTimer("ResolveDeferred");
    DependencyManager::get<DeferredLightingEffect>()->copyBack(renderContext->args);
}

RenderDeferredTask::RenderDeferredTask() : Task() {
    _jobs.push_back(Job(new SetupDeferred::JobModel("SetupFramebuffer")));
    _jobs.push_back(Job(new DrawBackground::JobModel("DrawBackground")));

    _jobs.push_back(Job(new PrepareDeferred::JobModel("PrepareDeferred")));
    _jobs.push_back(Job(new FetchItems::JobModel("FetchOpaque",
        FetchItems(
            [] (const RenderContextPointer& context, int count) {
                context->_numFeedOpaqueItems = count; 
            }
        )
    )));
    _jobs.push_back(Job(new CullItems::JobModel("CullOpaque", _jobs.back().getOutput())));
    _jobs.push_back(Job(new DepthSortItems::JobModel("DepthSortOpaque", _jobs.back().getOutput())));
    auto& renderedOpaques = _jobs.back().getOutput();
    _jobs.push_back(Job(new DrawOpaqueDeferred::JobModel("DrawOpaqueDeferred", _jobs.back().getOutput())));
  
    _jobs.push_back(Job(new DrawLight::JobModel("DrawLight")));
    _jobs.push_back(Job(new RenderDeferred::JobModel("RenderDeferred")));
    _jobs.push_back(Job(new ResolveDeferred::JobModel("ResolveDeferred")));
    _jobs.push_back(Job(new AmbientOcclusion::JobModel("AmbientOcclusion")));

    _jobs.back().setEnabled(false);
    _occlusionJobIndex = _jobs.size() - 1;

    _jobs.push_back(Job(new Antialiasing::JobModel("Antialiasing")));

    _jobs.back().setEnabled(false);
    _antialiasingJobIndex = _jobs.size() - 1;

    _jobs.push_back(Job(new FetchItems::JobModel("FetchTransparent",
         FetchItems(
            ItemFilter::Builder::transparentShape().withoutLayered(),
            [] (const RenderContextPointer& context, int count) {
                context->_numFeedTransparentItems = count; 
            }
         )
     )));
    _jobs.push_back(Job(new CullItems::JobModel("CullTransparent", _jobs.back().getOutput())));
    _jobs.push_back(Job(new DepthSortItems::JobModel("DepthSortTransparent", _jobs.back().getOutput(), DepthSortItems(false))));
    _jobs.push_back(Job(new DrawTransparentDeferred::JobModel("TransparentDeferred", _jobs.back().getOutput())));
    
    _jobs.push_back(Job(new render::DrawStatus::JobModel("DrawStatus", renderedOpaques)));


    _jobs.back().setEnabled(false);
    _drawStatusJobIndex = _jobs.size() - 1;

    _jobs.push_back(Job(new DrawOverlay3D::JobModel("DrawOverlay3D")));

    _jobs.push_back(Job(new HitEffect::JobModel("HitEffect")));
    _jobs.back().setEnabled(false);
    _drawHitEffectJobIndex = _jobs.size() -1;


    // Give ourselves 3 frmaes of timer queries
    _timerQueries.push_back(std::make_shared<gpu::Query>());
    _timerQueries.push_back(std::make_shared<gpu::Query>());
    _timerQueries.push_back(std::make_shared<gpu::Query>());
    _currentTimerQueryIndex = 0;
}

RenderDeferredTask::~RenderDeferredTask() {
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

    // Make sure we turn the displayItemStatus on/off
    setDrawItemStatus(renderContext->_drawItemStatus);
    
    //Make sure we display hit effect on screen, as desired from a script
    setDrawHitEffect(renderContext->_drawHitEffect);
    

    // TODO: turn on/off AO through menu item
    setOcclusionStatus(renderContext->_occlusionStatus);

    setAntialiasingStatus(renderContext->_fxaaStatus);

    renderContext->args->_context->syncCache();

    for (auto job : _jobs) {
        job.run(sceneContext, renderContext);
    }

};

void DrawOpaqueDeferred::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const ItemIDsBounds& inItems) {
    assert(renderContext->args);
    assert(renderContext->args->_viewFrustum);

    RenderArgs* args = renderContext->args;
    gpu::doInBatch(args->_context, [=](gpu::Batch& batch) {
        batch.setViewportTransform(args->_viewport);
        batch.setStateScissorRect(args->_viewport);
        args->_batch = &batch;

        renderContext->_numDrawnOpaqueItems = inItems.size();

        glm::mat4 projMat;
        Transform viewMat;
        args->_viewFrustum->evalProjectionMatrix(projMat);
        args->_viewFrustum->evalViewTransform(viewMat);

        batch.setProjectionTransform(projMat);
        batch.setViewTransform(viewMat);

        {
            const float OPAQUE_ALPHA_THRESHOLD = 0.5f;
            args->_alphaThreshold = OPAQUE_ALPHA_THRESHOLD;
        }
        renderItems(sceneContext, renderContext, inItems, renderContext->_maxDrawnOpaqueItems);
        args->_batch = nullptr;
    });
}

void DrawTransparentDeferred::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const ItemIDsBounds& inItems) {
    assert(renderContext->args);
    assert(renderContext->args->_viewFrustum);

    RenderArgs* args = renderContext->args;
    gpu::doInBatch(args->_context, [=](gpu::Batch& batch) {
        batch.setViewportTransform(args->_viewport);
        batch.setStateScissorRect(args->_viewport);
        args->_batch = &batch;
    
        renderContext->_numDrawnTransparentItems = inItems.size();

        glm::mat4 projMat;
        Transform viewMat;
        args->_viewFrustum->evalProjectionMatrix(projMat);
        args->_viewFrustum->evalViewTransform(viewMat);

        batch.setProjectionTransform(projMat);
        batch.setViewTransform(viewMat);
    
        const float TRANSPARENT_ALPHA_THRESHOLD = 0.0f;
        args->_alphaThreshold = TRANSPARENT_ALPHA_THRESHOLD;
    
        renderItems(sceneContext, renderContext, inItems, renderContext->_maxDrawnTransparentItems);
        args->_batch = nullptr;
    });
}

gpu::PipelinePointer DrawOverlay3D::_opaquePipeline;
const gpu::PipelinePointer& DrawOverlay3D::getOpaquePipeline() {
    if (!_opaquePipeline) {
        auto vs = gpu::ShaderPointer(gpu::Shader::createVertex(std::string(overlay3D_vert)));
        auto ps = gpu::ShaderPointer(gpu::Shader::createPixel(std::string(overlay3D_frag)));
        auto program = gpu::ShaderPointer(gpu::Shader::createProgram(vs, ps));
        
        auto state = std::make_shared<gpu::State>();
        state->setDepthTest(true, true, gpu::LESS_EQUAL);

        _opaquePipeline.reset(gpu::Pipeline::create(program, state));
    }
    return _opaquePipeline;
}

void DrawOverlay3D::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) {
    assert(renderContext->args);
    assert(renderContext->args->_viewFrustum);

    // render backgrounds
    auto& scene = sceneContext->_scene;
    auto& items = scene->getMasterBucket().at(ItemFilter::Builder::opaqueShape().withLayered());


    ItemIDsBounds inItems;
    inItems.reserve(items.size());
    for (auto id : items) {
        auto& item = scene->getItem(id);
        if (item.getKey().isVisible() && (item.getLayer() == 1)) {
            inItems.emplace_back(id);
        }
    }
    renderContext->_numFeedOverlay3DItems = inItems.size();
    renderContext->_numDrawnOverlay3DItems = inItems.size();

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
        gpu::doInBatch(args->_context, [=](gpu::Batch& batch) {
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
            renderItems(sceneContext, renderContext, inItems, renderContext->_maxDrawnOverlay3DItems);
        });
        args->_batch = nullptr;
        args->_whiteTexture.reset();
    }
}
