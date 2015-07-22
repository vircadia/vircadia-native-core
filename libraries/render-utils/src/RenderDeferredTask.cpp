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

#include <gpu/GPUConfig.h>
#include <gpu/Batch.h>
#include <gpu/Context.h>
#include <PerfStat.h>
#include <RenderArgs.h>
#include <ViewFrustum.h>

#include "DeferredLightingEffect.h"
#include "TextureCache.h"

#include "render/DrawStatus.h"

#include "overlay3D_vert.h"
#include "overlay3D_frag.h"

using namespace render;

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
    _jobs.push_back(Job(new ResetGLState::JobModel()));
    _jobs.push_back(Job(new RenderDeferred::JobModel("RenderDeferred")));
    _jobs.push_back(Job(new ResolveDeferred::JobModel("ResolveDeferred")));
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
    _jobs.push_back(Job(new ResetGLState::JobModel()));

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

    renderContext->args->_context->syncCache();

    for (auto job : _jobs) {
        job.run(sceneContext, renderContext);
    }

};

void DrawOpaqueDeferred::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const ItemIDsBounds& inItems) {
    assert(renderContext->args);
    assert(renderContext->args->_viewFrustum);

    RenderArgs* args = renderContext->args;
    gpu::Batch batch;
    args->_batch = &batch;

    renderContext->_numDrawnOpaqueItems = inItems.size();

    glm::mat4 projMat;
    Transform viewMat;
    args->_viewFrustum->evalProjectionMatrix(projMat);
    args->_viewFrustum->evalViewTransform(viewMat);
    if (args->_renderMode == RenderArgs::MIRROR_RENDER_MODE) {
        viewMat.preScale(glm::vec3(-1.0f, 1.0f, 1.0f));
    }
    batch.setProjectionTransform(projMat);
    batch.setViewTransform(viewMat);

    {
        GLenum buffers[3];
        int bufferCount = 0;
        buffers[bufferCount++] = GL_COLOR_ATTACHMENT0;
        buffers[bufferCount++] = GL_COLOR_ATTACHMENT1;
        buffers[bufferCount++] = GL_COLOR_ATTACHMENT2;
        batch._glDrawBuffers(bufferCount, buffers);
        const float OPAQUE_ALPHA_THRESHOLD = 0.5f;
        args->_alphaThreshold = OPAQUE_ALPHA_THRESHOLD;
    }

    renderItems(sceneContext, renderContext, inItems, renderContext->_maxDrawnOpaqueItems);

    // Before rendering the batch make sure we re in sync with gl state
    args->_context->syncCache();
    renderContext->args->_context->syncCache();
    args->_context->render((*args->_batch));
    args->_batch = nullptr;
}

void DrawTransparentDeferred::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const ItemIDsBounds& inItems) {
    assert(renderContext->args);
    assert(renderContext->args->_viewFrustum);

    RenderArgs* args = renderContext->args;
    gpu::Batch batch;
    args->_batch = &batch;
    
    renderContext->_numDrawnTransparentItems = inItems.size();

    glm::mat4 projMat;
    Transform viewMat;
    args->_viewFrustum->evalProjectionMatrix(projMat);
    args->_viewFrustum->evalViewTransform(viewMat);
    if (args->_renderMode == RenderArgs::MIRROR_RENDER_MODE) {
        viewMat.postScale(glm::vec3(-1.0f, 1.0f, 1.0f));
    }
    batch.setProjectionTransform(projMat);
    batch.setViewTransform(viewMat);
    const float TRANSPARENT_ALPHA_THRESHOLD = 0.0f;
    
    {
        GLenum buffers[3];
        int bufferCount = 0;
        buffers[bufferCount++] = GL_COLOR_ATTACHMENT0;
        batch._glDrawBuffers(bufferCount, buffers);
        args->_alphaThreshold = TRANSPARENT_ALPHA_THRESHOLD;
    }
    
    
    renderItems(sceneContext, renderContext, inItems, renderContext->_maxDrawnTransparentItems);
    
    // Before rendering the batch make sure we re in sync with gl state
    args->_context->syncCache();
    args->_context->render((*args->_batch));
    args->_batch = nullptr;
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

    RenderArgs* args = renderContext->args;
    gpu::Batch batch;
    args->_batch = &batch;
    args->_whiteTexture = DependencyManager::get<TextureCache>()->getWhiteTexture();

    glm::mat4 projMat;
    Transform viewMat;
    args->_viewFrustum->evalProjectionMatrix(projMat);
    args->_viewFrustum->evalViewTransform(viewMat);
    if (args->_renderMode == RenderArgs::MIRROR_RENDER_MODE) {
        viewMat.postScale(glm::vec3(-1.0f, 1.0f, 1.0f));
    }
    batch.setProjectionTransform(projMat);
    batch.setViewTransform(viewMat);

    batch.setPipeline(getOpaquePipeline());
    batch.setResourceTexture(0, args->_whiteTexture);

    if (!inItems.empty()) {
        batch.clearFramebuffer(gpu::Framebuffer::BUFFER_DEPTH, glm::vec4(), 1.f, 0);
        renderItems(sceneContext, renderContext, inItems, renderContext->_maxDrawnOverlay3DItems);
    }

    // Before rendering the batch make sure we re in sync with gl state
    args->_context->syncCache();
    args->_context->render((*args->_batch));
    args->_batch = nullptr;
    args->_whiteTexture.reset();
}
