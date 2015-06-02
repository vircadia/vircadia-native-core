//
//  DrawTask.cpp
//  render/src/render
//
//  Created by Sam Gateau on 5/21/15.
//  Copyright 20154 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "DrawTask.h"

#include <PerfStat.h>

#include "gpu/Batch.h"
#include "gpu/Context.h"

#include "ViewFrustum.h"
#include "RenderArgs.h"

#include <algorithm>
#include <assert.h>

using namespace render;

DrawSceneTask::DrawSceneTask() : Task() {

    _jobs.push_back(Job(DrawOpaque()));
    _jobs.push_back(Job(DrawLight()));
    _jobs.push_back(Job(DrawTransparent()));
    _jobs.push_back(Job(ResetGLState()));
}

DrawSceneTask::~DrawSceneTask() {
}

void DrawSceneTask::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) {
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

Job::~Job() {
}

void render::cullItems(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const ItemIDs& inItems, ItemIDs& outItems) {
    PerformanceTimer perfTimer("cullItems");
    assert(renderContext->args);
    assert(renderContext->args->_viewFrustum);

    auto& scene = sceneContext->_scene;
    RenderArgs* args = renderContext->args;

    // Culling / LOD
    for (auto id : inItems) {
        auto item = scene->getItem(id);
        auto bound = item.getBound();

        if (bound.isNull()) {
            outItems.push_back(id); // One more Item to render
            args->_itemsRendered++;
            continue;
        }

        // TODO: some entity types (like lights) might want to be rendered even
        // when they are outside of the view frustum...
        bool outOfView = args->_viewFrustum->boxInFrustum(bound) == ViewFrustum::OUTSIDE;
        if (!outOfView) {
            bool bigEnoughToRender = (args->_shouldRender) ? args->_shouldRender(args, bound) : true;
            
            if (bigEnoughToRender) {
                outItems.push_back(id); // One more Item to render
                args->_itemsRendered++;
            } else {
                args->_itemsTooSmall++;
            }
        } else {
            args->_itemsOutOfView++;
        }
    }
}

struct ItemBound {
    float _centerDepth = 0.0f;
    float _nearDepth = 0.0f;
    float _farDepth = 0.0f;
    ItemID _id = 0;

    ItemBound() {}
    ItemBound(float centerDepth, float nearDepth, float farDepth, ItemID id) : _centerDepth(centerDepth), _nearDepth(nearDepth), _farDepth(farDepth), _id(id) {}
};

struct FrontToBackSort {
    bool operator() (const ItemBound& left, const ItemBound& right) {
        return (left._centerDepth < right._centerDepth);
    }
};

struct BackToFrontSort {
    bool operator() (const ItemBound& left, const ItemBound& right) {
        return (left._centerDepth > right._centerDepth);
    }
};

void render::depthSortItems(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, bool frontToBack, const ItemIDs& inItems, ItemIDs& outItems) {
    PerformanceTimer perfTimer("depthSortItems");
    assert(renderContext->args);
    assert(renderContext->args->_viewFrustum);
    
    auto& scene = sceneContext->_scene;
    RenderArgs* args = renderContext->args;

    // Allocate and simply copy
    outItems.reserve(inItems.size());


    // Make a local dataset of the center distance and closest point distance
    std::vector<ItemBound> itemBounds;
    itemBounds.reserve(outItems.size());

    for (auto id : inItems) {
        auto item = scene->getItem(id);
        auto bound = item.getBound();
        float distance = args->_viewFrustum->distanceToCamera(bound.calcCenter());
    
        itemBounds.emplace_back(ItemBound(distance, distance, distance, id));
    }

    // sort against Z
    if (frontToBack) {
        FrontToBackSort frontToBackSort;
        std::sort (itemBounds.begin(), itemBounds.end(), frontToBackSort); 
    } else {
        BackToFrontSort  backToFrontSort;
        std::sort (itemBounds.begin(), itemBounds.end(), backToFrontSort); 
    }

    // FInally once sorted result to a list of itemID
    for (auto& itemBound : itemBounds) {
        outItems.emplace_back(itemBound._id);
    }
}

void render::renderItems(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const ItemIDs& inItems) {
    PerformanceTimer perfTimer("renderItems");
    auto& scene = sceneContext->_scene;
    RenderArgs* args = renderContext->args;
    // render
    for (auto id : inItems) {
        auto item = scene->getItem(id);
        item.render(args);
    }
}


void addClearStateCommands(gpu::Batch& batch) {
    batch._glDepthMask(true);
    batch._glDepthFunc(GL_LESS);
    batch._glDisable(GL_CULL_FACE);

    batch._glActiveTexture(GL_TEXTURE0 + 1);
    batch._glBindTexture(GL_TEXTURE_2D, 0);
    batch._glActiveTexture(GL_TEXTURE0 + 2);
    batch._glBindTexture(GL_TEXTURE_2D, 0);
    batch._glActiveTexture(GL_TEXTURE0 + 3);
    batch._glBindTexture(GL_TEXTURE_2D, 0);
    batch._glActiveTexture(GL_TEXTURE0);
    batch._glBindTexture(GL_TEXTURE_2D, 0);


    // deactivate vertex arrays after drawing
    batch._glDisableClientState(GL_NORMAL_ARRAY);
    batch._glDisableClientState(GL_VERTEX_ARRAY);
    batch._glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    batch._glDisableClientState(GL_COLOR_ARRAY);
    batch._glDisableVertexAttribArray(gpu::Stream::TANGENT);
    batch._glDisableVertexAttribArray(gpu::Stream::SKIN_CLUSTER_INDEX);
    batch._glDisableVertexAttribArray(gpu::Stream::SKIN_CLUSTER_WEIGHT);
    
    // bind with 0 to switch back to normal operation
    batch._glBindBuffer(GL_ARRAY_BUFFER, 0);
    batch._glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    batch._glBindTexture(GL_TEXTURE_2D, 0);

    // Back to no program
    batch._glUseProgram(0);
}

template <> void render::jobRun(const ResetGLState& job, const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) {

    gpu::Batch theBatch;
    addClearStateCommands(theBatch);
    assert(renderContext->args);
    renderContext->args->_context->render(theBatch);
}

template <> void render::jobRun(const DrawOpaque& job, const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) {
    PerformanceTimer perfTimer("DrawOpaque");
    assert(renderContext->args);
    assert(renderContext->args->_viewFrustum);
    
    // render opaques
    auto& scene = sceneContext->_scene;
    auto& items = scene->getMasterBucket().at(ItemFilter::Builder::opaqueShape());

    ItemIDs inItems;
    inItems.reserve(items.size());
    for (auto id : items) {
        inItems.push_back(id);
    }
    ItemIDs& renderedItems = inItems;

    ItemIDs culledItems;
    if (renderContext->_cullOpaque) {
        cullItems(sceneContext, renderContext, renderedItems, culledItems);
        renderedItems = culledItems;
    }

    ItemIDs sortedItems;
    if (renderContext->_sortOpaque) {
        depthSortItems(sceneContext, renderContext, true, renderedItems, sortedItems); // Sort Front to back opaque items!
        renderedItems = sortedItems;
    }

    if (renderContext->_renderOpaque) {
        RenderArgs* args = renderContext->args;
        gpu::Batch batch;
        args->_batch = &batch;

        glm::mat4 projMat;
        Transform viewMat;
        args->_viewFrustum->evalProjectionMatrix(projMat);
        args->_viewFrustum->evalViewTransform(viewMat);
        batch.setProjectionTransform(projMat);
        batch.setViewTransform(viewMat);

        renderContext->args->_renderMode = RenderArgs::NORMAL_RENDER_MODE;
        {
            GLenum buffers[3];
            int bufferCount = 0;
            buffers[bufferCount++] = GL_COLOR_ATTACHMENT0;
            buffers[bufferCount++] = GL_COLOR_ATTACHMENT1;
            buffers[bufferCount++] = GL_COLOR_ATTACHMENT2;
            batch._glDrawBuffers(bufferCount, buffers);
        }

        renderItems(sceneContext, renderContext, renderedItems);

        args->_context->render((*args->_batch));
        args->_batch = nullptr;
    }
}


template <> void render::jobRun(const DrawTransparent& job, const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) {
    PerformanceTimer perfTimer("DrawTransparent");
    assert(renderContext->args);
    assert(renderContext->args->_viewFrustum);

    // render transparents
    auto& scene = sceneContext->_scene;
    auto& items = scene->getMasterBucket().at(ItemFilter::Builder::transparentShape());

    ItemIDs inItems;
    inItems.reserve(items.size());
    for (auto id : items) {
  //      inItems.push_back(id);
    }
    ItemIDs& renderedItems = inItems;

    ItemIDs culledItems;
    if (renderContext->_cullTransparent) {
        cullItems(sceneContext, renderContext, inItems, culledItems);
        renderedItems = culledItems;
    }

    ItemIDs sortedItems;
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
        batch.setProjectionTransform(projMat);
        batch.setViewTransform(viewMat);

        args->_renderMode = RenderArgs::NORMAL_RENDER_MODE;

        const float MOSTLY_OPAQUE_THRESHOLD = 0.75f;
        const float TRANSPARENT_ALPHA_THRESHOLD = 0.0f;

        // render translucent meshes afterwards
        {
            GLenum buffers[2];
            int bufferCount = 0;
            buffers[bufferCount++] = GL_COLOR_ATTACHMENT1;
            buffers[bufferCount++] = GL_COLOR_ATTACHMENT2;
            batch._glDrawBuffers(bufferCount, buffers);
            args->_alphaThreshold = MOSTLY_OPAQUE_THRESHOLD;
         }

        renderItems(sceneContext, renderContext, renderedItems);

        {
            GLenum buffers[3];
            int bufferCount = 0;
            buffers[bufferCount++] = GL_COLOR_ATTACHMENT0;
            batch._glDrawBuffers(bufferCount, buffers);
            args->_alphaThreshold = TRANSPARENT_ALPHA_THRESHOLD;
        }


        renderItems(sceneContext, renderContext, renderedItems);

        args->_context->render((*args->_batch));
        args->_batch = nullptr;
    }
}

template <> void render::jobRun(const DrawLight& job, const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) {
    PerformanceTimer perfTimer("DrawLight");
    assert(renderContext->args);
    assert(renderContext->args->_viewFrustum);

    // render lights
    auto& scene = sceneContext->_scene;
    auto& items = scene->getMasterBucket().at(ItemFilter::Builder::light());


     ItemIDs inItems;
    inItems.reserve(items.size());
    for (auto id : items) {
        inItems.push_back(id);
    }

    ItemIDs culledItems;
    cullItems(sceneContext, renderContext, inItems, culledItems);

    RenderArgs* args = renderContext->args;
    gpu::Batch theBatch;
    args->_batch = &theBatch;
    renderItems(sceneContext, renderContext, culledItems);
    args->_context->render((*args->_batch));
    args->_batch = nullptr;
}

template <> void render::jobRun(const DrawBackground& job, const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) {
    PerformanceTimer perfTimer("DrawBackground");
    assert(renderContext->args);
    assert(renderContext->args->_viewFrustum);

    // render backgrounds
    auto& scene = sceneContext->_scene;
    auto& items = scene->getMasterBucket().at(ItemFilter::Builder::background());


    ItemIDs inItems;
    inItems.reserve(items.size());
    for (auto id : items) {
        inItems.push_back(id);
    }
    RenderArgs* args = renderContext->args;
    gpu::Batch batch;
    args->_batch = &batch;

    glm::mat4 projMat;
    Transform viewMat;
    args->_viewFrustum->evalProjectionMatrix(projMat);
    args->_viewFrustum->evalViewTransform(viewMat);
    batch.setProjectionTransform(projMat);
    batch.setViewTransform(viewMat);

    renderItems(sceneContext, renderContext, inItems);
    args->_context->render((*args->_batch));
    args->_batch = nullptr;

    // Force the context sync
    args->_context->syncCache();
}
