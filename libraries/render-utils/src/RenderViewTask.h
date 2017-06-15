//
//  RenderViewTask.h
//  render-utils/src/
//
//  Created by Sam Gateau on 5/25/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once
#ifndef hifi_RenderViewTask_h
#define hifi_RenderViewTask_h

#include <render/Engine.h>
#include <render/RenderFetchCullSortTask.h>


class RenderViewTask {
public:
    using Input = RenderFetchCullSortTask::Output;
    using JobModel = render::Task::ModelI<RenderViewTask, Input>;

    RenderViewTask() {}

    void build(JobModel& task, const render::Varying& inputs, render::Varying& outputs, render::CullFunctor cullFunctor, bool isDeferred);

};


#endif // hifi_RenderViewTask_h
