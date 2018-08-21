//
//  RenderViewTask.cpp
//  render-utils/src/
//
//  Created by Sam Gateau on 5/25/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "RenderViewTask.h"

#include "RenderShadowTask.h"
#include "RenderDeferredTask.h"
#include "RenderForwardTask.h"

void RenderViewTask::build(JobModel& task, const render::Varying& input, render::Varying& output, render::CullFunctor cullFunctor, bool isDeferred, uint8_t tagBits, uint8_t tagMask) {
   // auto items = input.get<Input>();

    const auto items = task.addJob<RenderFetchCullSortTask>("FetchCullSort", cullFunctor, tagBits, tagMask);
    assert(items.canCast<RenderFetchCullSortTask::Output>());

    if (isDeferred) {
        // Warning : the cull functor passed to the shadow pass should only be testing for LOD culling. If frustum culling
        // is performed, then casters not in the view frustum will be removed, which is not what we wish.
        const auto cascadeSceneBBoxes = task.addJob<RenderShadowTask>("RenderShadowTask", cullFunctor, tagBits, tagMask);
        const auto renderInput = RenderDeferredTask::Input(items, cascadeSceneBBoxes).asVarying();
        task.addJob<RenderDeferredTask>("RenderDeferredTask", renderInput, true);
    } else {
        task.addJob<RenderForwardTask>("Forward", items);
    }
}

