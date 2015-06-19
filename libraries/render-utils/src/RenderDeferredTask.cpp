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

#include "gpu/Batch.h"
#include "gpu/Context.h"
#include "DeferredLightingEffect.h"
#include "ViewFrustum.h"
#include "RenderArgs.h"
#include "TextureCache.h"

#include <PerfStat.h>

#include "overlay3D_vert.h"
#include "overlay3D_frag.h"

using namespace render;

template <> void render::jobRun(const PrepareDeferred& job, const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) {
    PerformanceTimer perfTimer("PrepareDeferred");
    DependencyManager::get<DeferredLightingEffect>()->prepare();
}

template <> void render::jobRun(const RenderDeferred& job, const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) {
    PerformanceTimer perfTimer("RenderDeferred");
    DependencyManager::get<DeferredLightingEffect>()->render();
//    renderContext->args->_context->syncCache();
}

template <> void render::jobRun(const ResolveDeferred& job, const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) {
    PerformanceTimer perfTimer("ResolveDeferred");
    DependencyManager::get<DeferredLightingEffect>()->copyBack(renderContext->args);
    renderContext->args->_context->syncCache();

}



RenderDeferredTask::RenderDeferredTask() : Task() {
   _jobs.push_back(Job(PrepareDeferred()));
   _jobs.push_back(Job(DrawBackground()));
   _jobs.push_back(Job(DrawOpaqueDeferred()));
   _jobs.push_back(Job(DrawLight()));
   _jobs.push_back(Job(ResetGLState()));
   _jobs.push_back(Job(RenderDeferred()));
   _jobs.push_back(Job(ResolveDeferred()));
   _jobs.push_back(Job(DrawTransparentDeferred()));
   _jobs.push_back(Job(DrawOverlay3D()));
   _jobs.push_back(Job(ResetGLState()));
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

    renderContext->args->_context->syncCache();

    for (auto job : _jobs) {
        job.run(sceneContext, renderContext);
    }
};



template <> void render::jobRun(const DrawOpaqueDeferred& job, const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) {
    PerformanceTimer perfTimer("DrawOpaqueDeferred");
    assert(renderContext->args);
    assert(renderContext->args->_viewFrustum);

    // render opaques
    auto& scene = sceneContext->_scene;
    auto& items = scene->getMasterBucket().at(ItemFilter::Builder::opaqueShape().withoutLayered());
    auto& renderDetails = renderContext->args->_details;

    ItemIDsBounds inItems;
    inItems.reserve(items.size());
    for (auto id : items) {
        inItems.emplace_back(ItemIDAndBounds(id));
    }
    ItemIDsBounds& renderedItems = inItems;

    renderContext->_numFeedOpaqueItems = renderedItems.size();

    ItemIDsBounds culledItems;
    culledItems.reserve(inItems.size());
    if (renderContext->_cullOpaque) {
        renderDetails.pointTo(RenderDetails::OPAQUE_ITEM);
        cullItems(sceneContext, renderContext, renderedItems, culledItems);
        renderDetails.pointTo(RenderDetails::OTHER_ITEM);
        renderedItems = culledItems;
    }

    renderContext->_numDrawnOpaqueItems = renderedItems.size();


    ItemIDsBounds sortedItems;
    sortedItems.reserve(culledItems.size());
    if (renderContext->_sortOpaque) {
        depthSortItems(sceneContext, renderContext, true, renderedItems, sortedItems); // Sort Front to back opaque items!
        renderedItems = sortedItems;
    }

   // ItemIDsBounds sortedItems;
 /*   ItemMaterialBucketMap stateSortedItems;
    stateSortedItems.allocateStandardMaterialBuckets();
    if (true) {
        for (auto& itemIDAndBound : renderedItems) {
            stateSortedItems.insert(itemIDAndBound.id, scene->getItem(itemIDAndBound.id).getMaterialKey());
        }
    }
*/

    if (renderContext->_renderOpaque) {
        RenderArgs* args = renderContext->args;
        gpu::Batch batch;
        args->_batch = &batch;

        glm::mat4 projMat;
        Transform viewMat;
        args->_viewFrustum->evalProjectionMatrix(projMat);
        args->_viewFrustum->evalViewTransform(viewMat);
        if (args->_renderMode == RenderArgs::MIRROR_RENDER_MODE) {
            viewMat.postScale(glm::vec3(-1.0f, 1.0f, 1.0f));
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
        }

        renderItems(sceneContext, renderContext, renderedItems, renderContext->_maxDrawnOpaqueItems);

        args->_context->render((*args->_batch));
        args->_batch = nullptr;
    }
}


template <> void render::jobRun(const DrawTransparentDeferred& job, const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) {
    PerformanceTimer perfTimer("DrawTransparentDeferred");
    assert(renderContext->args);
    assert(renderContext->args->_viewFrustum);

    // render transparents
    auto& scene = sceneContext->_scene;
    auto& items = scene->getMasterBucket().at(ItemFilter::Builder::transparentShape().withoutLayered());
    auto& renderDetails = renderContext->args->_details;
    
    ItemIDsBounds inItems;
    inItems.reserve(items.size());
    for (auto id : items) {
        inItems.push_back(id);
    }
    ItemIDsBounds& renderedItems = inItems;

    renderContext->_numFeedTransparentItems = renderedItems.size();

    ItemIDsBounds culledItems;
    if (renderContext->_cullTransparent) {
        renderDetails.pointTo(RenderDetails::TRANSLUCENT_ITEM);
        cullItems(sceneContext, renderContext, inItems, culledItems);
        renderDetails.pointTo(RenderDetails::OTHER_ITEM);
        renderedItems = culledItems;
    }
    
    renderContext->_numDrawnTransparentItems = renderedItems.size();

    ItemIDsBounds sortedItems;
    if (renderContext->_sortTransparent) {
        depthSortItems(sceneContext, renderContext, false, renderedItems, sortedItems); // Sort Back to front transparent items!
        renderedItems = sortedItems;
    }

    if (renderContext->_renderTransparent) {
        RenderArgs* args = renderContext->args;
        gpu::Batch batch;
        args->_batch = &batch;

        


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


        renderItems(sceneContext, renderContext, renderedItems, renderContext->_maxDrawnTransparentItems);

        // Before rendering the batch make sure we re in sync with gl state
        args->_context->syncCache();
        args->_context->render((*args->_batch));
        args->_batch = nullptr;

        // reset blend function to standard...
       // glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_CONSTANT_ALPHA, GL_ONE);
    }
}

gpu::PipelinePointer DrawOverlay3D::_opaquePipeline;
const gpu::PipelinePointer& DrawOverlay3D::getOpaquePipeline() {
    if (!_opaquePipeline) {
        auto vs = gpu::ShaderPointer(gpu::Shader::createVertex(std::string(overlay3D_vert)));
        auto ps = gpu::ShaderPointer(gpu::Shader::createPixel(std::string(overlay3D_frag)));
        
        auto program = gpu::ShaderPointer(gpu::Shader::createProgram(vs, ps));

        auto state = gpu::StatePointer(new gpu::State());
        state->setDepthTest(true, true, gpu::LESS_EQUAL);

        _opaquePipeline.reset(gpu::Pipeline::create(program, state));
    }
    return _opaquePipeline;
}

template <> void render::jobRun(const DrawOverlay3D& job, const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) {
      PerformanceTimer perfTimer("DrawOverlay3D");
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
    batch.setPipeline(job.getOpaquePipeline());
    batch.setUniformTexture(0, args->_whiteTexture);

    if (!inItems.empty()) {
        batch.clearFramebuffer(gpu::Framebuffer::BUFFER_DEPTH, glm::vec4(), 1.f, 0);
        renderItems(sceneContext, renderContext, inItems);
    }

    // Before rendering the batch make sure we re in sync with gl state
    args->_context->syncCache();
    args->_context->render((*args->_batch));
    args->_batch = nullptr;
    args->_whiteTexture.reset();
}
