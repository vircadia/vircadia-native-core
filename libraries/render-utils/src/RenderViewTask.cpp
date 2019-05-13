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
#include "RenderCommonTask.h"
#include "RenderDeferredTask.h"
#include "RenderForwardTask.h"

#include <RenderForward.h>

void RenderShadowsAndDeferredTask::build(JobModel& task, const render::Varying& input, render::Varying& output, render::CullFunctor cullFunctor, uint8_t tagBits, uint8_t tagMask) {
    task.addJob<SetRenderMethod>("SetRenderMethodTask", render::Args::DEFERRED);

    const auto items = input.getN<DeferredForwardSwitchJob::Input>(0);
    const auto lightingModel = input.getN<DeferredForwardSwitchJob::Input>(1);
    const auto lightingStageFramesAndZones = input.getN<DeferredForwardSwitchJob::Input>(2);

    // Warning : the cull functor passed to the shadow pass should only be testing for LOD culling. If frustum culling
    // is performed, then casters not in the view frustum will be removed, which is not what we wish.
    const auto shadowTaskIn = RenderShadowTask::Input(lightingStageFramesAndZones.get<AssembleLightingStageTask::Output>().get0()[0], lightingModel).asVarying();
    const auto shadowTaskOut = task.addJob<RenderShadowTask>("RenderShadowTask", shadowTaskIn, cullFunctor, tagBits, tagMask);

    const auto renderDeferredInput = RenderDeferredTask::Input(items, lightingModel, lightingStageFramesAndZones, shadowTaskOut).asVarying();
    task.addJob<RenderDeferredTask>("RenderDeferredTask", renderDeferredInput);
}

void DeferredForwardSwitchJob::build(JobModel& task, const render::Varying& input, render::Varying& output, render::CullFunctor cullFunctor, uint8_t tagBits, uint8_t tagMask) {
    task.addBranch<RenderShadowsAndDeferredTask>("RenderShadowsAndDeferredTask", RENDER_FORWARD ? 1 : 0, input, cullFunctor, tagBits, tagMask);

    task.addBranch<RenderForwardTask>("RenderForwardTask", RENDER_FORWARD ? 0 : 1, input);
}

void RenderViewTask::build(JobModel& task, const render::Varying& input, render::Varying& output, render::CullFunctor cullFunctor, uint8_t tagBits, uint8_t tagMask) {
    const auto items = task.addJob<RenderFetchCullSortTask>("FetchCullSort", cullFunctor, tagBits, tagMask);

    // Issue the lighting model, aka the big global settings for the view 
    const auto lightingModel = task.addJob<MakeLightingModel>("LightingModel");

    // Assemble the lighting stages current frames
    const auto lightingStageFramesAndZones = task.addJob<AssembleLightingStageTask>("AssembleStages", items);

#ifndef Q_OS_ANDROID
        const auto deferredForwardIn = DeferredForwardSwitchJob::Input(items, lightingModel, lightingStageFramesAndZones).asVarying();
        task.addJob<DeferredForwardSwitchJob>("DeferredForwardSwitch", deferredForwardIn, cullFunctor, tagBits, tagMask);
#else
        const auto renderInput = RenderForwardTask::Input(items, lightingModel, lightingStageFramesAndZones).asVarying();
        task.addJob<RenderForwardTask>("RenderForwardTask", renderInput);
#endif
}
