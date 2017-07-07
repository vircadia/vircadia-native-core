//
//  UpdateSceneTask.cpp
//  render-utils/src/
//
//  Created by Sam Gateau on 6/21/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "UpdateSceneTask.h"

#include <render/SceneTask.h>
#include "LightStage.h"
#include "BackgroundStage.h"
#include <render/TransitionStage.h>
#include "DeferredLightingEffect.h"

void UpdateSceneTask::build(JobModel& task, const render::Varying& input, render::Varying& output) {
    task.addJob<LightStageSetup>("LightStageSetup");
    task.addJob<BackgroundStageSetup>("BackgroundStageSetup");
    task.addJob<render::TransitionStageSetup>("TransitionStageSetup");

    task.addJob<DefaultLightingSetup>("DefaultLightingSetup");

    task.addJob<render::PerformSceneTransaction>("PerformSceneTransaction");
}

