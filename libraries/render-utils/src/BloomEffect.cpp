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
    auto blurConfig = getConfig<render::BlurGaussian>("Blur");
    assert(blurConfig);
    blurConfig->setProperty("filterScale", value*10.0f);
}

Bloom::Bloom() {

}

void Bloom::configure(const Config& config) {

}

void Bloom::build(JobModel& task, const render::Varying& inputs, render::Varying& outputs) {
    const auto& blurInput = inputs;
    task.addJob<render::BlurGaussian>("Blur", blurInput);
}
