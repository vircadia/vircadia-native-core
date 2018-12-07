//
//  Created by Samuel Gateau on 2018/12/06
//  Copyright 2013-2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AssembleLightingStageTask_h
#define hifi_AssembleLightingStageTask_h

#include <render/RenderFetchCullSortTask.h>
#include "LightingModel.h"

#include "LightStage.h"
#include "BackgroundStage.h"
#include "HazeStage.h"
#include "BloomStage.h"

#include "ZoneRenderer.h"


class FetchCurrentFrames {
public:
    using Outputs = render::VaryingSet4<LightStage::FramePointer, BackgroundStage::FramePointer, HazeStage::FramePointer, BloomStage::FramePointer>;
    using JobModel = render::Job::ModelO<FetchCurrentFrames, Outputs>;

    FetchCurrentFrames() {}

    void run(const render::RenderContextPointer& renderContext, Outputs& outputs);
};

class AssembleLightingStageTask {
public:
    using Inputs = RenderFetchCullSortTask::BucketList;
    using Outputs = render::VaryingSet2<FetchCurrentFrames::Outputs, ZoneRendererTask::Outputs>;
    using JobModel = render::Task::ModelIO<FetchCurrentFrames, Inputs, Outputs>;

    AssembleLightingStageTask() {}

    void build(JobModel& task, const render::Varying& input, render::Varying& output);
};

#endif
