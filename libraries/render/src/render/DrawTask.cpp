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


using namespace render;

Job::~Job() {
}

template <> void render::jobRun(const FilterItems& filterItems, const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) {
    auto& scene = sceneContext->_scene;

}

template <> void render::jobRun(const RenderItems& renderItems, const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) {
    auto& scene = sceneContext->_scene;
    RenderArgs* args = renderContext->args;
    // render
    for (auto id : renderItems._items) {
        auto item = scene->getItem(id);
        item.render(args);
    }
}




template <> void render::jobRun(const DrawOpaque& job, const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) {
    // render opaques
    auto& scene = sceneContext->_scene;
    auto& items = scene->getMasterBucket().at(ItemFilter::Builder::opaqueShape());

    RenderArgs* args = renderContext->args;
    gpu::Batch theBatch;
    args->_batch = &theBatch;
    for (auto id : items) {
        auto item = scene->getItem(id);
        item.render(args);
    }

    args->_context->enqueueBatch((*args->_batch));
    args->_batch = nullptr;
}


template <> void render::jobRun(const DrawTransparent& job, const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) {
    // render transparents
    auto& scene = sceneContext->_scene;
    auto& items = scene->getMasterBucket().at(ItemFilter::Builder::transparentShape());

    RenderArgs* args = renderContext->args;
    gpu::Batch theBatch;
    args->_batch = &theBatch;
    for (auto id : items) {
        auto item = scene->getItem(id);
        item.render(args);
    }

    args->_context->enqueueBatch((*args->_batch));
    args->_batch = nullptr;
}

template <> void render::jobRun(const DrawLight& job, const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) {
    // render lights
    auto& scene = sceneContext->_scene;
    auto& items = scene->getMasterBucket().at(ItemFilter::Builder::light());
 
    RenderArgs* args = renderContext->args;
    for (auto id : items) {
        auto item = scene->getItem(id);
        item.render(args);
    }
}

DrawSceneTask::DrawSceneTask() : Task() {

    _jobs.push_back(Job(DrawOpaque()));
    _jobs.push_back(Job(DrawLight()));
    _jobs.push_back(Job(DrawTransparent()));
}

DrawSceneTask::~DrawSceneTask() {
}

void DrawSceneTask::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) {
    // sanity checks
    assert(sceneContext);
    if (!sceneContext->_scene) {
        return;
    }

    for (auto job : _jobs) {
        job.run(sceneContext, renderContext);
    }
};

