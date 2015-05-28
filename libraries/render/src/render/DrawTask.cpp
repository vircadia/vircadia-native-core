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


DrawSceneTask::~DrawSceneTask() {
}

void DrawSceneTask::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) {
    // sanity checks
    assert(sceneContext);
    if (!sceneContext->_scene) {
        return;
    }
    auto& scene = sceneContext->_scene;

    auto& itemBucketMap = scene->getMasterBucket();
    
    RenderArgs* args = renderContext->args;
    gpu::Batch theBatch;

    args->_batch = &theBatch;

    // render opaques
    auto filter = ItemFilter::Builder::opaqueShape();
    auto& opaqueShapeItems = itemBucketMap.at(filter);

    for (auto id : opaqueShapeItems) {
        auto item = scene->getItem(id);
        item.render(args);
    }

    args->_context->enqueueBatch((*args->_batch));
    args->_batch = nullptr;
};

