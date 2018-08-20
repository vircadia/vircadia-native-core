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

#include <gpu/Context.h>

#include <shaders/Shaders.h>

#include <render/BlurTask.h>
#include <render/ResampleTask.h>
#include "render-utils/ShaderConstants.h"

#define BLOOM_BLUR_LEVEL_COUNT  3

BloomThreshold::BloomThreshold(unsigned int downsamplingFactor) {
    assert(downsamplingFactor > 0);
    _parameters.edit()._sampleCount = downsamplingFactor;
}

void BloomThreshold::configure(const Config& config) {
    if (_parameters.get()._threshold != config.threshold) {
        _parameters.edit()._threshold = config.threshold;
    }
}

void BloomThreshold::run(const render::RenderContextPointer& renderContext, const Inputs& inputs, Outputs& outputs) {
    assert(renderContext->args);
    assert(renderContext->args->hasViewFrustum());

    RenderArgs* args = renderContext->args;

    const auto frameTransform = inputs.get0();
    const auto inputFrameBuffer = inputs.get1();

    assert(inputFrameBuffer->hasColor());

    auto inputBuffer = inputFrameBuffer->getRenderBuffer(0);
    auto bufferSize = gpu::Vec2u(inputBuffer->getDimensions());
    const auto downSamplingFactor = _parameters.get()._sampleCount;

    // Downsample resolution
    bufferSize.x /= downSamplingFactor;
    bufferSize.y /= downSamplingFactor;

    if (!_outputBuffer || _outputBuffer->getSize() != bufferSize) {
        auto colorTexture = gpu::TexturePointer(gpu::Texture::createRenderBuffer(inputBuffer->getTexelFormat(), bufferSize.x, bufferSize.y,
                                                gpu::Texture::SINGLE_MIP, gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_LINEAR_MIP_POINT, gpu::Sampler::WRAP_CLAMP)));

        _outputBuffer = gpu::FramebufferPointer(gpu::Framebuffer::create("BloomThreshold"));
        _outputBuffer->setRenderBuffer(0, colorTexture);

        _parameters.edit()._deltaUV = { 1.0f / bufferSize.x, 1.0f / bufferSize.y };
    }

    if (!_pipeline) {
        gpu::ShaderPointer program = gpu::Shader::createProgram(shader::render_utils::program::bloomThreshold);
        gpu::StatePointer state = gpu::StatePointer(new gpu::State());
        _pipeline = gpu::Pipeline::create(program, state);
    }

    glm::ivec4 viewport{ 0, 0, bufferSize.x, bufferSize.y };

    gpu::doInBatch("BloomThreshold::run", args->_context, [&](gpu::Batch& batch) {
        batch.enableStereo(false);

        batch.setViewportTransform(viewport);
        batch.setProjectionTransform(glm::mat4());
        batch.resetViewTransform();
        batch.setModelTransform(gpu::Framebuffer::evalSubregionTexcoordTransform(bufferSize, viewport));
        batch.setPipeline(_pipeline);

        batch.setFramebuffer(_outputBuffer);
        batch.setResourceTexture(render_utils::slot::texture::BloomColor, inputBuffer);
        batch.setUniformBuffer(render_utils::slot::buffer::BloomParams, _parameters);
        batch.draw(gpu::TRIANGLE_STRIP, 4);
    });

    outputs = _outputBuffer;
}

BloomApply::BloomApply() {

}

void BloomApply::configure(const Config& config) {
    const auto newIntensity = config.intensity / 3.0f;

    if (_parameters.get()._intensities.x != newIntensity) {
        auto& parameters = _parameters.edit();
        parameters._intensities.x = newIntensity;
        parameters._intensities.y = newIntensity;
        parameters._intensities.z = newIntensity;
    }
}

void BloomApply::run(const render::RenderContextPointer& renderContext, const Inputs& inputs) {
    assert(renderContext->args);
    assert(renderContext->args->hasViewFrustum());
    RenderArgs* args = renderContext->args;

    static const auto BLUR0_SLOT = 0;
    static const auto BLUR1_SLOT = 1;
    static const auto BLUR2_SLOT = 2;
    static const auto PARAMETERS_SLOT = 0;

    if (!_pipeline) {
        gpu::ShaderPointer program = gpu::Shader::createProgram(shader::render_utils::program::bloomApply);
        gpu::StatePointer state = gpu::StatePointer(new gpu::State());
        state->setDepthTest(gpu::State::DepthTest(false, false));
        _pipeline = gpu::Pipeline::create(program, state);
    }

    const auto frameBuffer = inputs.get0();
    const auto framebufferSize = frameBuffer->getSize();
    const auto blur0FB = inputs.get1();
    const auto blur1FB = inputs.get2();
    const auto blur2FB = inputs.get3();
    const glm::ivec4 viewport{ 0, 0, framebufferSize.x, framebufferSize.y };

    gpu::doInBatch("BloomApply::run", args->_context, [&](gpu::Batch& batch) {
        batch.enableStereo(false);

        batch.setFramebuffer(frameBuffer);

        batch.setViewportTransform(viewport);
        batch.setProjectionTransform(glm::mat4());
        batch.resetViewTransform();
        batch.setPipeline(_pipeline);

        batch.setModelTransform(gpu::Framebuffer::evalSubregionTexcoordTransform(framebufferSize, viewport));
        batch.setResourceTexture(BLUR0_SLOT, blur0FB->getRenderBuffer(0));
        batch.setResourceTexture(BLUR1_SLOT, blur1FB->getRenderBuffer(0));
        batch.setResourceTexture(BLUR2_SLOT, blur2FB->getRenderBuffer(0));
        batch.setUniformBuffer(PARAMETERS_SLOT, _parameters);
        batch.draw(gpu::TRIANGLE_STRIP, 4);
    });
}

void BloomDraw::run(const render::RenderContextPointer& renderContext, const Inputs& inputs) {
    assert(renderContext->args);
    assert(renderContext->args->hasViewFrustum());
    RenderArgs* args = renderContext->args;

    const auto frameBuffer = inputs.get0();
    const auto bloomFrameBuffer = inputs.get1();

    if (frameBuffer && bloomFrameBuffer) {
        const auto framebufferSize = frameBuffer->getSize();

        if (!_pipeline) {
            gpu::ShaderPointer program = gpu::Shader::createProgram(shader::gpu::program::drawTransformUnitQuadTextureOpaque);
            gpu::StatePointer state = gpu::StatePointer(new gpu::State());
            state->setDepthTest(gpu::State::DepthTest(false, false));
            state->setBlendFunction(true, gpu::State::ONE, gpu::State::BLEND_OP_ADD, gpu::State::ONE,
                                    gpu::State::ZERO, gpu::State::BLEND_OP_ADD, gpu::State::ONE);
            _pipeline = gpu::Pipeline::create(program, state);
        }

        gpu::doInBatch("BloomDraw::run", args->_context, [&](gpu::Batch& batch) {
            batch.enableStereo(false);

            batch.setFramebuffer(frameBuffer);

            batch.setViewportTransform(args->_viewport);
            batch.setProjectionTransform(glm::mat4());
            batch.resetViewTransform();
            batch.setPipeline(_pipeline);

            batch.setModelTransform(gpu::Framebuffer::evalSubregionTexcoordTransform(framebufferSize, args->_viewport));
            batch.setResourceTexture(0, bloomFrameBuffer->getRenderBuffer(0));
            batch.draw(gpu::TRIANGLE_STRIP, 4);
        });
    }
}

DebugBloom::DebugBloom() {
}

void DebugBloom::configure(const Config& config) {
    _mode = static_cast<DebugBloomConfig::Mode>(config.mode);
    assert(_mode < DebugBloomConfig::MODE_COUNT);
}

void DebugBloom::run(const render::RenderContextPointer& renderContext, const Inputs& inputs) {
    assert(renderContext->args);
    assert(renderContext->args->hasViewFrustum());
    RenderArgs* args = renderContext->args;

    const auto frameBuffer = inputs.get0();
    const auto combinedBlurBuffer = inputs.get4();
    const auto framebufferSize = frameBuffer->getSize();
    const auto level0FB = inputs.get1();
    const auto level1FB = inputs.get2();
    const auto level2FB = inputs.get3();
    const gpu::TexturePointer levelTextures[BLOOM_BLUR_LEVEL_COUNT] = {
        level0FB->getRenderBuffer(0),
        level1FB->getRenderBuffer(0),
        level2FB->getRenderBuffer(0)
    };

    static auto TEXCOORD_RECT_SLOT = 1;

    if (!_pipeline) {
        gpu::ShaderPointer program = gpu::Shader::createProgram(shader::gpu::program::drawTextureOpaqueTexcoordRect);
        gpu::StatePointer state = gpu::StatePointer(new gpu::State());
        state->setDepthTest(gpu::State::DepthTest(false));
        _pipeline = gpu::Pipeline::create(program, state);
    }

    gpu::doInBatch("DebugBloom::run", args->_context, [&](gpu::Batch& batch) {
        batch.enableStereo(false);

        batch.setFramebuffer(frameBuffer);

        batch.setViewportTransform(args->_viewport);
        batch.setProjectionTransform(glm::mat4());
        batch.resetViewTransform();
        batch.setPipeline(_pipeline);

        Transform modelTransform;
        if (_mode == DebugBloomConfig::MODE_ALL_LEVELS) {
            batch._glUniform4f(TEXCOORD_RECT_SLOT, 0.0f, 0.0f, 1.f, 1.f);

            modelTransform = gpu::Framebuffer::evalSubregionTexcoordTransform(framebufferSize, args->_viewport / 2);
            modelTransform.postTranslate(glm::vec3(-1.0f, 1.0f, 0.0f));
            batch.setModelTransform(modelTransform);
            batch.setResourceTexture(0, levelTextures[0]);
            batch.draw(gpu::TRIANGLE_STRIP, 4);

            modelTransform.postTranslate(glm::vec3(2.0f, 0.0f, 0.0f));
            batch.setModelTransform(modelTransform);
            batch.setResourceTexture(0, levelTextures[1]);
            batch.draw(gpu::TRIANGLE_STRIP, 4);

            modelTransform.postTranslate(glm::vec3(-2.0f, -2.0f, 0.0f));
            batch.setModelTransform(modelTransform);
            batch.setResourceTexture(0, levelTextures[2]);
            batch.draw(gpu::TRIANGLE_STRIP, 4);

            modelTransform.postTranslate(glm::vec3(2.0f, 0.0f, 0.0f));
            batch.setModelTransform(modelTransform);
            batch.setResourceTexture(0, combinedBlurBuffer->getRenderBuffer(0));
            batch.draw(gpu::TRIANGLE_STRIP, 4);
        } else {
            auto viewport = args->_viewport;
            auto blurLevel = _mode - DebugBloomConfig::MODE_LEVEL0;

            viewport.z /= 2;

            batch._glUniform4f(TEXCOORD_RECT_SLOT, 0.5f, 0.0f, 0.5f, 1.f);

            modelTransform = gpu::Framebuffer::evalSubregionTexcoordTransform(framebufferSize, viewport);
            modelTransform.postTranslate(glm::vec3(-1.0f, 0.0f, 0.0f));
            batch.setModelTransform(modelTransform);
            batch.setResourceTexture(0, levelTextures[blurLevel]);
            batch.draw(gpu::TRIANGLE_STRIP, 4);
        }
    });
}

void BloomConfig::setIntensity(float value) {
    auto task = static_cast<render::Task::TaskConcept*>(_task);
    auto blurJobIt = task->editJob("BloomApply");
    assert(blurJobIt != task->_jobs.end());
    blurJobIt->getConfiguration()->setProperty("intensity", value);
}

float BloomConfig::getIntensity() const {
    auto task = static_cast<render::Task::TaskConcept*>(_task);
    auto blurJobIt = task->getJob("BloomApply");
    assert(blurJobIt != task->_jobs.end());
    return blurJobIt->getConfiguration()->property("intensity").toFloat();
}

void BloomConfig::setSize(float value) {
    std::string blurName{ "BloomBlurN" };
    auto sigma = 0.5f+value*3.5f;
    auto task = static_cast<render::Task::TaskConcept*>(_task);

    for (auto i = 0; i < BLOOM_BLUR_LEVEL_COUNT; i++) {
        blurName.back() = '0' + i;
        auto blurJobIt = task->editJob(blurName);
        assert(blurJobIt != task->_jobs.end());
        auto& gaussianBlur = blurJobIt->edit<render::BlurGaussian>();
        auto gaussianBlurParams = gaussianBlur.getParameters();
        gaussianBlurParams->setFilterGaussianTaps(9, sigma);
    }
    auto blurJobIt = task->getJob("BloomApply");
    assert(blurJobIt != task->_jobs.end());
    blurJobIt->getConfiguration()->setProperty("sigma", sigma);
}

Bloom::Bloom() {

}

void Bloom::configure(const Config& config) {
    std::string blurName{ "BloomBlurN" };

    for (auto i = 0; i < BLOOM_BLUR_LEVEL_COUNT; i++) {
        blurName.back() = '0' + i;
        auto blurConfig = config.getConfig<render::BlurGaussian>(blurName);
        blurConfig->setProperty("filterScale", 1.0f);
    }
}

void Bloom::build(JobModel& task, const render::Varying& inputs, render::Varying& outputs) {
    // Start by computing threshold of color buffer input at quarter resolution
    const auto bloomInputBuffer = task.addJob<BloomThreshold>("BloomThreshold", inputs, 4U);

    // Multi-scale blur, each new blur is half resolution of the previous pass
    const auto blurFB0 = task.addJob<render::BlurGaussian>("BloomBlur0", bloomInputBuffer, true);
    const auto blurFB1 = task.addJob<render::BlurGaussian>("BloomBlur1", blurFB0, true, 2U);
    const auto blurFB2 = task.addJob<render::BlurGaussian>("BloomBlur2", blurFB1, true, 2U);

    const auto& input = inputs.get<Inputs>();
    const auto& frameBuffer = input[1];

    // Mix all blur levels at quarter resolution
    const auto applyInput = BloomApply::Inputs(bloomInputBuffer, blurFB0, blurFB1, blurFB2).asVarying();
    task.addJob<BloomApply>("BloomApply", applyInput);
    // And then blend result in additive manner on top of final color buffer
    const auto drawInput = BloomDraw::Inputs(frameBuffer, bloomInputBuffer).asVarying();
    task.addJob<BloomDraw>("BloomDraw", drawInput);

    const auto debugInput = DebugBloom::Inputs(frameBuffer, blurFB0, blurFB1, blurFB2, bloomInputBuffer).asVarying();
    task.addJob<DebugBloom>("DebugBloom", debugInput);
}
