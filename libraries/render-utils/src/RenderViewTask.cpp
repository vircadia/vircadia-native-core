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



void RenderViewTask::build(JobModel& task, const render::Varying& input, render::Varying& output, render::CullFunctor cullFunctor, bool isDeferred) {
   // auto items = input.get<Input>();

    task.addJob<RenderShadowTask>("RenderShadowTask", cullFunctor);

    const auto items = task.addJob<RenderFetchCullSortTask>("FetchCullSort", cullFunctor);
    assert(items.canCast<RenderFetchCullSortTask::Output>());

    if (isDeferred) {
        task.addJob<RenderDeferredTask>("RenderDeferredTask", items);
    } else {
        task.addJob<RenderForwardTask>("Forward", items);
    }
}

