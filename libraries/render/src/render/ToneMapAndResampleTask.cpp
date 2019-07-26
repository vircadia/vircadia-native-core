//
//  ResampleTask.cpp
//  render/src/render
//
//  Various to Resample or downsample textures into framebuffers.
//
//  Created by Olivier Prat on 10/09/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "ToneMapAndResampleTask.h"

#include <gpu/Context.h>
#include <shaders/Shaders.h>

using namespace render;
using namespace shader::gpu::program;
/*
gpu::PipelinePointer HalfDownsample::_pipeline;

HalfDownsample::HalfDownsample() {

}

void HalfDownsample::configure(const Config& config) {

}

gpu::FramebufferPointer HalfDownsample::getResampledFrameBuffer(const gpu::FramebufferPointer& sourceFramebuffer) {
    auto resampledFramebufferSize = sourceFramebuffer->getSize();

    resampledFramebufferSize.x /= 2U;
    resampledFramebufferSize.y /= 2U;

    if (!_destinationFrameBuffer || resampledFramebufferSize != _destinationFrameBuffer->getSize()) {
        _destinationFrameBuffer = gpu::FramebufferPointer(gpu::Framebuffer::create("HalfOutput"));

        auto sampler = gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_LINEAR_MIP_POINT);
        auto target = gpu::Texture::createRenderBuffer(sourceFramebuffer->getRenderBuffer(0)->getTexelFormat(), resampledFramebufferSize.x, resampledFramebufferSize.y, gpu::Texture::SINGLE_MIP, sampler);
        _destinationFrameBuffer->setRenderBuffer(0, target);
    }
    return _destinationFrameBuffer;
}

void HalfDownsample::run(const RenderContextPointer& renderContext, const gpu::FramebufferPointer& sourceFramebuffer, gpu::FramebufferPointer& resampledFrameBuffer) {
    assert(renderContext->args);
    assert(renderContext->args->hasViewFrustum());
    RenderArgs* args = renderContext->args;

    resampledFrameBuffer = getResampledFrameBuffer(sourceFramebuffer);

    if (!_pipeline) {
        gpu::ShaderPointer program = gpu::Shader::createProgram(shader::gpu::program::drawTransformUnitQuadTextureOpaque);
        gpu::StatePointer state = gpu::StatePointer(new gpu::State());
        state->setDepthTest(gpu::State::DepthTest(false, false));
        _pipeline = gpu::Pipeline::create(program, state);
    }

    const auto bufferSize = resampledFrameBuffer->getSize();
    glm::ivec4 viewport{ 0, 0, bufferSize.x, bufferSize.y };

    gpu::doInBatch("HalfDownsample::run", args->_context, [&](gpu::Batch& batch) {
        batch.enableStereo(false);

        batch.setFramebuffer(resampledFrameBuffer);

        batch.setViewportTransform(viewport);
        batch.setProjectionTransform(glm::mat4());
        batch.resetViewTransform();
        batch.setPipeline(_pipeline);

        batch.setModelTransform(gpu::Framebuffer::evalSubregionTexcoordTransform(bufferSize, viewport));
        batch.setResourceTexture(0, sourceFramebuffer->getRenderBuffer(0));
        batch.draw(gpu::TRIANGLE_STRIP, 4);
    });
}
*/
/*
gpu::PipelinePointer Resample::_pipeline;

void Resample::configure(const Config& config) {
    _factor = config.factor;
}

gpu::FramebufferPointer Resample::getResampledFrameBuffer(const gpu::FramebufferPointer& sourceFramebuffer) {
    if (_factor == 1.0f) {
        return sourceFramebuffer;
    }

    auto resampledFramebufferSize = glm::uvec2(glm::vec2(sourceFramebuffer->getSize()) * _factor);

    if (!_destinationFrameBuffer || resampledFramebufferSize != _destinationFrameBuffer->getSize()) {
        _destinationFrameBuffer = gpu::FramebufferPointer(gpu::Framebuffer::create("ResampledOutput"));

        auto sampler = gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_LINEAR);
        auto target = gpu::Texture::createRenderBuffer(sourceFramebuffer->getRenderBuffer(0)->getTexelFormat(), resampledFramebufferSize.x, resampledFramebufferSize.y, gpu::Texture::SINGLE_MIP, sampler);
        _destinationFrameBuffer->setRenderBuffer(0, target);
    }
    return _destinationFrameBuffer;
}

void Resample::run(const RenderContextPointer& renderContext, const gpu::FramebufferPointer& sourceFramebuffer, gpu::FramebufferPointer& resampledFrameBuffer) {
    assert(renderContext->args);
    assert(renderContext->args->hasViewFrustum());
    RenderArgs* args = renderContext->args;

    resampledFrameBuffer = getResampledFrameBuffer(sourceFramebuffer);
    if (resampledFrameBuffer != sourceFramebuffer) {
        if (!_pipeline) {
            gpu::ShaderPointer program = gpu::Shader::createProgram(shader::gpu::program::drawTransformUnitQuadTextureOpaque);
            gpu::StatePointer state = gpu::StatePointer(new gpu::State());
            state->setDepthTest(gpu::State::DepthTest(false, false));
            _pipeline = gpu::Pipeline::create(program, state);
        }

        const auto bufferSize = resampledFrameBuffer->getSize();
        glm::ivec4 viewport{ 0, 0, bufferSize.x, bufferSize.y };

        gpu::doInBatch("Resample::run", args->_context, [&](gpu::Batch& batch) {
            batch.enableStereo(false);

            batch.setFramebuffer(resampledFrameBuffer);

            batch.setViewportTransform(viewport);
            batch.setProjectionTransform(glm::mat4());
            batch.resetViewTransform();
            batch.setPipeline(_pipeline);

            batch.setModelTransform(gpu::Framebuffer::evalSubregionTexcoordTransform(bufferSize, viewport));
            batch.setResourceTexture(0, sourceFramebuffer->getRenderBuffer(0));
            batch.draw(gpu::TRIANGLE_STRIP, 4);
        });

        // Set full final viewport
        args->_viewport = viewport;
    }
}
*/
ToneMapAndResample::ToneMapAndResample() {
    Parameters parameters;
    _parametersBuffer = gpu::BufferView(std::make_shared<gpu::Buffer>(sizeof(Parameters), (const gpu::Byte*) &parameters));
}

void ToneMapAndResample::setExposure(float exposure) {
    auto& params = _parametersBuffer.get<Parameters>();
    if (params._exposure != exposure) {
        _parametersBuffer.edit<Parameters>()._exposure = exposure;
        _parametersBuffer.edit<Parameters>()._twoPowExposure = pow(2.0, exposure);
    }
}

void ToneMapAndResample::setToneCurve(ToneCurve curve) {
    auto& params = _parametersBuffer.get<Parameters>();
    if (params._toneCurve != (int)curve) {
        _parametersBuffer.edit<Parameters>()._toneCurve = (int)curve;
    }
}

void ToneMapAndResample::configure(const Config& config) {
    setExposure(config.exposure);
    setToneCurve((ToneCurve)config.curve);
}

gpu::PipelinePointer ToneMapAndResample::_pipeline;
gpu::PipelinePointer ToneMapAndResample::_mirrorPipeline;

void ToneMapAndResample::run(const RenderContextPointer& renderContext, const Input& input, gpu::FramebufferPointer& resampledFrameBuffer) {
    assert(renderContext->args);
    assert(renderContext->args->hasViewFrustum());
    RenderArgs* args = renderContext->args;
    auto sourceFramebuffer = input;

    //auto lightingBuffer = input->getRenderBuffer(0);

    resampledFrameBuffer = args->_blitFramebuffer;
/*
    if (!_pipeline) {
        init(args);
    }

    if (!lightingBuffer || !blitFramebuffer) {
        return;
    }
    */

    if (resampledFrameBuffer != sourceFramebuffer) {

        if (!_pipeline) {
            gpu::StatePointer state = gpu::StatePointer(new gpu::State());

            state->setDepthTest(gpu::State::DepthTest(false, false));
            //blitState->setColorWriteMask(true, true, true, true);

            _pipeline = gpu::Pipeline::create(gpu::Shader::createProgram(drawTransformUnitQuadTextureOpaque), state);
            _mirrorPipeline = gpu::Pipeline::create(gpu::Shader::createProgram(DrawTextureMirroredX), state);
        }

        const auto bufferSize = resampledFrameBuffer->getSize();

        //auto srcBufferSize = glm::ivec2(lightingBuffer->getDimensions());

        glm::ivec4 viewport{ 0, 0, bufferSize.x, bufferSize.y };

        gpu::doInBatch("Resample::run", args->_context, [&](gpu::Batch& batch) {
            batch.enableStereo(false);
            batch.setFramebuffer(resampledFrameBuffer);

            batch.setViewportTransform(viewport);
            batch.setProjectionTransform(glm::mat4());
            batch.resetViewTransform();
            batch.setPipeline(args->_renderMode == RenderArgs::MIRROR_RENDER_MODE ? _mirrorPipeline : _pipeline);

            // viewport = args->_viewport ??
            batch.setModelTransform(gpu::Framebuffer::evalSubregionTexcoordTransform(bufferSize, viewport));
            batch.setResourceTexture(0, sourceFramebuffer->getRenderBuffer(0));
            batch.draw(gpu::TRIANGLE_STRIP, 4);
        });

        // Set full final viewport
        args->_viewport = viewport;
    }
}