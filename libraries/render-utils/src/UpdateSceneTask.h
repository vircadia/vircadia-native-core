//
//  UpdateSceneTask.h
//  render-utils/src/
//
//  Created by Sam Gateau on 6/21/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once
#ifndef hifi_UpdateSceneTask_h
#define hifi_UpdateSceneTask_h

#include <render/Engine.h>
#include <render/RenderFetchCullSortTask.h>


class UpdateSceneTask {
public:
    using JobModel = render::Task::Model<UpdateSceneTask>;

    UpdateSceneTask() {}

    void build(JobModel& task, const render::Varying& inputs, render::Varying& outputs);

};


#endif // hifi_UpdateSceneTask_h
