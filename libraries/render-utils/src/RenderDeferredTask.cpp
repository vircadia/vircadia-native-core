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

#include "gpu/Context.h"

#include <PerfStat.h>

using namespace render;


template <> void render::jobRun(const PrepareDeferred& job, const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext) {
    PerformanceTimer perfTimer("PrepareDeferred");
    DependencyManager::get<DeferredLightingEffect>()->prepare();
}

template <> void render::jobRun(const ResolveDeferred& job, const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext) {
    PerformanceTimer perfTimer("ResolveDeferred");
    DependencyManager::get<DeferredLightingEffect>()->render();
}


RenderDeferredTask::RenderDeferredTask() : Task() {
   _jobs.push_back(Job(PrepareDeferred()));
   _jobs.push_back(Job(DrawBackground()));
   _jobs.push_back(Job(DrawOpaque()));
   _jobs.push_back(Job(DrawLight()));
   _jobs.push_back(Job(DrawTransparent()));
   _jobs.push_back(Job(ResolveDeferred()));

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
