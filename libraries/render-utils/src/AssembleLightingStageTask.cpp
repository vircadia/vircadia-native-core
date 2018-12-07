//
//  Created by Samuel Gateau on 2018/12/06
//  Copyright 2013-2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "AssembleLightingStageTask.h"


void FetchCurrentFrames::run(const render::RenderContextPointer& renderContext, Outputs& outputs) {
    auto lightStage = renderContext->_scene->getStage<LightStage>();
    assert(lightStage);
    outputs.edit0() = std::make_shared<LightStage::Frame>(lightStage->_currentFrame);

    auto backgroundStage = renderContext->_scene->getStage<BackgroundStage>();
    assert(backgroundStage);
    outputs.edit1() = std::make_shared<BackgroundStage::Frame>(backgroundStage->_currentFrame);

    auto hazeStage = renderContext->_scene->getStage<HazeStage>();
    assert(hazeStage);
    outputs.edit2() = std::make_shared<HazeStage::Frame>(hazeStage->_currentFrame);

    auto bloomStage = renderContext->_scene->getStage<BloomStage>();
    assert(bloomStage);
    outputs.edit3() = std::make_shared<BloomStage::Frame>(bloomStage->_currentFrame);
}


void RenderUpdateLightingStagesTask::build(JobModel& task, const render::Varying& input, render::Varying& output) {
    const auto& items = input.get<Inputs>();

    const auto& metas = items[RenderFetchCullSortTask::META];

    // Clear Light, Haze, Bloom, and Skybox Stages and render zones from the general metas bucket
    const auto zones = task.addJob<ZoneRendererTask>("ZoneRenderer", metas);

    // Draw Lights just add the lights to the current list of lights to deal with. NOt really gpu job for now.
    task.addJob<DrawLight>("DrawLight", lights);

    // Fetch the current frame stacks from all the stages
    const auto currentStageFrames = task.addJob<FetchCurrentFrames>("FetchCurrentFrames");

    const auto lightFrame = currentStageFrames.getN<FetchCurrentFrames::Outputs>(0);
    const auto backgroundFrame = currentStageFrames.getN<FetchCurrentFrames::Outputs>(1);
    const auto hazeFrame = currentStageFrames.getN<FetchCurrentFrames::Outputs>(2);
    const auto bloomFrame = currentStageFrames.getN<FetchCurrentFrames::Outputs>(3);

    outputs.edit0() = currentStageFrames;
    outputs.edit1() = zones;
}

