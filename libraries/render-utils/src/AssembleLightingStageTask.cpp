//
//  Created by Samuel Gateau on 2018/12/06
//  Copyright 2013-2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "AssembleLightingStageTask.h"

#include <render/DrawTask.h>

void FetchCurrentFrames::run(const render::RenderContextPointer& renderContext, Output& output) {
    auto lightStage = renderContext->_scene->getStage<LightStage>();
    assert(lightStage);
    output.edit0() = std::make_shared<LightStage::Frame>(lightStage->_currentFrame);

    auto backgroundStage = renderContext->_scene->getStage<BackgroundStage>();
    assert(backgroundStage);
    output.edit1() = std::make_shared<BackgroundStage::Frame>(backgroundStage->_currentFrame);

    auto hazeStage = renderContext->_scene->getStage<HazeStage>();
    assert(hazeStage);
    output.edit2() = std::make_shared<HazeStage::Frame>(hazeStage->_currentFrame);

    auto bloomStage = renderContext->_scene->getStage<BloomStage>();
    assert(bloomStage);
    output.edit3() = std::make_shared<BloomStage::Frame>(bloomStage->_currentFrame);
}


void AssembleLightingStageTask::build(JobModel& task, const render::Varying& input, render::Varying& output) {
    const auto& fetchCullSortOut = input.get<Input>();
    const auto& items = fetchCullSortOut.get0();
    //const auto& items = input.get<Input>();

    const auto& lights = items[RenderFetchCullSortTask::LIGHT];
    const auto& metas = items[RenderFetchCullSortTask::META];

    // Clear Light, Haze, Bloom, and Skybox Stages and render zones from the general metas bucket
    const auto zones = task.addJob<ZoneRendererTask>("ZoneRenderer", metas);

    // Draw Lights just add the lights to the current list of lights to deal with. NOt really gpu job for now.
    task.addJob<render::DrawLight>("DrawLight", lights);

    // Fetch the current frame stacks from all the stages
    const auto currentStageFrames = task.addJob<FetchCurrentFrames>("FetchCurrentFrames");

    output = Output(currentStageFrames, zones);
}

