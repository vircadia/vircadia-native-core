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

#include <PerfStat.h>


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
   _jobs.push_back(Job(DrawOpaque()));
   _jobs.push_back(Job(DrawLight()));
   _jobs.push_back(Job(ResetGLState()));
   _jobs.push_back(Job(RenderDeferred()));
   _jobs.push_back(Job(ResolveDeferred()));
   _jobs.push_back(Job(DrawTransparentDeferred()));
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



template <> void render::jobRun(const DrawTransparentDeferred& job, const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) {
    PerformanceTimer perfTimer("DrawTransparentDeferred");
    assert(renderContext->args);
    assert(renderContext->args->_viewFrustum);

    // render transparents
    auto& scene = sceneContext->_scene;
    auto& items = scene->getMasterBucket().at(ItemFilter::Builder::transparentShape());

    ItemIDs inItems;
    inItems.reserve(items.size());
    for (auto id : items) {
        inItems.push_back(id);
    }
    ItemIDs& renderedItems = inItems;

    renderContext->_numFeedTransparentItems = renderedItems.size();

    ItemIDs culledItems;
    if (renderContext->_cullTransparent) {
        cullItems(sceneContext, renderContext, inItems, culledItems);
        renderedItems = culledItems;
    }

    renderContext->_numDrawnTransparentItems = renderedItems.size();

    ItemIDs sortedItems;
    if (renderContext->_sortTransparent) {
        depthSortItems(sceneContext, renderContext, false, renderedItems, sortedItems); // Sort Back to front transparent items!
        renderedItems = sortedItems;
    }

    if (renderContext->_renderTransparent) {
        RenderArgs* args = renderContext->args;
        gpu::Batch batch;
        args->_batch = &batch;

        DependencyManager::get<DeferredLightingEffect>()->setupTransparent(renderContext->args);


        glm::mat4 projMat;
        Transform viewMat;
        args->_viewFrustum->evalProjectionMatrix(projMat);
        args->_viewFrustum->evalViewTransform(viewMat);
        batch.setProjectionTransform(projMat);
        batch.setViewTransform(viewMat);

        args->_renderMode = RenderArgs::NORMAL_RENDER_MODE;

        const float MOSTLY_OPAQUE_THRESHOLD = 0.75f;
        const float TRANSPARENT_ALPHA_THRESHOLD = 0.0f;

  /*      // render translucent meshes afterwards
        {
            GLenum buffers[2];
            int bufferCount = 0;
            buffers[bufferCount++] = GL_COLOR_ATTACHMENT1;
            buffers[bufferCount++] = GL_COLOR_ATTACHMENT2;
            batch._glDrawBuffers(bufferCount, buffers);
            args->_alphaThreshold = MOSTLY_OPAQUE_THRESHOLD;
         }

        renderItems(sceneContext, renderContext, renderedItems, renderContext->_maxDrawnTransparentItems);
*/
        {
            GLenum buffers[3];
            int bufferCount = 0;
            buffers[bufferCount++] = GL_COLOR_ATTACHMENT0;
            batch._glDrawBuffers(bufferCount, buffers);
            args->_alphaThreshold = TRANSPARENT_ALPHA_THRESHOLD;
        }


        renderItems(sceneContext, renderContext, renderedItems, renderContext->_maxDrawnTransparentItems);

        args->_context->render((*args->_batch));
        args->_batch = nullptr;
    }
}
