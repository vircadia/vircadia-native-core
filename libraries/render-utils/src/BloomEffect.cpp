//
//  BloomEffect.cpp
//  render-utils/src/
//
//  Created by Olivier Prat on 09/25/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "BloomEffect.h"

#include <render/BlurTask.h>

void BloomConfig::setMix(float value) {

}

void BloomConfig::setSize(float value) {
    auto& blurJob = static_cast<render::Task::TaskConcept*>(_task)->_jobs.front();
    auto& gaussianBlur = blurJob.edit<render::BlurGaussian>();
    auto gaussianBlurParams = gaussianBlur.getParameters();
    gaussianBlurParams->setGaussianFilterTaps((BLUR_MAX_NUM_TAPS - 1) / 2, value*7.0f);
}

Bloom::Bloom() {

}

void Bloom::configure(const Config& config) {
    auto blurConfig = config.getConfig<render::BlurGaussian>();
    blurConfig->setProperty("filterScale", 2.5f);
}

void Bloom::build(JobModel& task, const render::Varying& inputs, render::Varying& outputs) {
    const auto& blurInput = inputs;
    task.addJob<render::BlurGaussian>("Blur", blurInput);
}
