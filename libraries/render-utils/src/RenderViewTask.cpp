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

#include "AssembleLightingStageTask.h"
#include "RenderShadowTask.h"
#include "RenderDeferredTask.h"
#include "RenderForwardTask.h"

void RenderViewTask::build(JobModel& task, const render::Varying& input, render::Varying& output, render::CullFunctor cullFunctor, bool isDeferred, uint8_t tagBits, uint8_t tagMask) {
    const auto items = task.addJob<RenderFetchCullSortTask>("FetchCullSort", cullFunctor, tagBits, tagMask);
    assert(items.canCast<RenderFetchCullSortTask::Output>());

    // Issue the lighting model, aka the big global settings for the view 
    const auto lightingModel = task.addJob<MakeLightingModel>("LightingModel");

    // Assemble the lighting stages current frames
    const auto lightingStageFramesAndZones = task.addJob<AssembleLightingStageTask>("AssembleStages", items);

    if (isDeferred) {
        // Warning : the cull functor passed to the shadow pass should only be testing for LOD culling. If frustum culling
        // is performed, then casters not in the view frustum will be removed, which is not what we wish.
        const auto shadowTaskIn = RenderShadowTask::Input(lightingStageFramesAndZones.get<AssembleLightingStageTask::Output>().get0()[0], lightingModel).asVarying();
        const auto shadowTaskOut = task.addJob<RenderShadowTask>("RenderShadowTask", shadowTaskIn, cullFunctor, tagBits, tagMask);

        const auto renderInput = RenderDeferredTask::Input(items, lightingModel, lightingStageFramesAndZones, shadowTaskOut).asVarying();
        task.addJob<RenderDeferredTask>("RenderDeferredTask", renderInput);
    } else {
        const auto renderInput = RenderForwardTask::Input(items, lightingModel, lightingStageFramesAndZones).asVarying();
        task.addJob<RenderForwardTask>("Forward", renderInput);
    }
}

