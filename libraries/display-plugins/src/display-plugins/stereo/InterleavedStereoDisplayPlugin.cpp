//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "InterleavedStereoDisplayPlugin.h"

#include <gpu/Pipeline.h>
#include <gpu/Batch.h>
#include <gpu/Context.h>
#include <shaders/Shaders.h>

const QString InterleavedStereoDisplayPlugin::NAME("3D TV - Interleaved");

void InterleavedStereoDisplayPlugin::customizeContext() {
    StereoDisplayPlugin::customizeContext();
    if (!_interleavedPresentPipeline) {
        gpu::ShaderPointer program = gpu::Shader::createProgram(shader::display_plugins::program::InterleavedSrgbToLinear);
        gpu::StatePointer state = std::make_shared<gpu::State>();
        state->setDepthTest(gpu::State::DepthTest(false));
        _interleavedPresentPipeline = gpu::Pipeline::create(program, state);
    }
}

void InterleavedStereoDisplayPlugin::uncustomizeContext() {
    _interleavedPresentPipeline.reset();
    StereoDisplayPlugin::uncustomizeContext();
}

glm::uvec2 InterleavedStereoDisplayPlugin::getRecommendedRenderSize() const {
    uvec2 result = Parent::getRecommendedRenderSize();
    result.x *= 2;
    result.y /= 2;
    return result;
}

void InterleavedStereoDisplayPlugin::internalPresent() {
    render([&](gpu::Batch& batch) {
        batch.enableStereo(false);
        batch.resetViewTransform();
        batch.setFramebuffer(gpu::FramebufferPointer());
        batch.setViewportTransform(ivec4(uvec2(0), getSurfacePixels()));
        batch.setResourceTexture(0, _currentFrame->framebuffer->getRenderBuffer(0));
        batch.setPipeline(_interleavedPresentPipeline);
        batch.draw(gpu::TRIANGLE_STRIP, 4);
    });
    swapBuffers();
}
