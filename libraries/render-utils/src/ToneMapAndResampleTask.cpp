//
//  ToneMapAndResampleTask.cpp
//  libraries/render-utils/src
//
//  Created by Anna Brewer on 7/3/19.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ToneMapAndResampleTask.h"

#include <gpu/Context.h>
#include <shaders/Shaders.h>

#include "render-utils/ShaderConstants.h"
#include "StencilMaskPass.h"
#include "FramebufferCache.h"

using namespace render;
using namespace shader::gpu::program;
using namespace shader::render_utils::program;

gpu::PipelinePointer ToneMapAndResample::_pipeline;
gpu::PipelinePointer ToneMapAndResample::_mirrorPipeline;

ToneMapAndResample::ToneMapAndResample() {
    Parameters parameters;
    _parametersBuffer = gpu::BufferView(std::make_shared<gpu::Buffer>(sizeof(Parameters), (const gpu::Byte*) &parameters));
}

void ToneMapAndResample::init(RenderArgs* args) {

    // shared_ptr to gpu::State
    gpu::StatePointer blitState = gpu::StatePointer(new gpu::State());

    blitState->setDepthTest(gpu::State::DepthTest(false, false));
    blitState->setColorWriteMask(true, true, true, true);

    _pipeline = gpu::PipelinePointer(gpu::Pipeline::create(gpu::Shader::createProgram(toneMapping), blitState));
    _mirrorPipeline = gpu::PipelinePointer(gpu::Pipeline::create(gpu::Shader::createProgram(toneMapping_mirrored), blitState));
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

void ToneMapAndResample::run(const RenderContextPointer& renderContext, const Input& input, gpu::FramebufferPointer& resampledFrameBuffer) {
    assert(renderContext->args);
    assert(renderContext->args->hasViewFrustum());

    RenderArgs* args = renderContext->args;
    auto sourceFramebuffer = input;

    auto lightingBuffer = input->getRenderBuffer(0);

    resampledFrameBuffer = args->_blitFramebuffer;

    if (!lightingBuffer || !blitFramebuffer) {
        return;
    }

    if (resampledFrameBuffer != sourceFramebuffer) {

        if (!_pipeline) {
            init(args);
        }

        const auto bufferSize = resampledFrameBuffer->getSize();

        const auto srcBufferSize = glm::ivec2(lightingBuffer->getDimensions());

        glm::ivec4 destViewport{ 0, 0, bufferSize.x, bufferSize.y };

        gpu::doInBatch("Resample::run", args->_context, [&](gpu::Batch& batch) {
            batch.enableStereo(false);
            batch.setFramebuffer(resampledFrameBuffer);

            batch.setViewportTransform(destViewport);
            batch.setProjectionTransform(glm::mat4());
            batch.resetViewTransform();
            batch.setPipeline(args->_renderMode == RenderArgs::MIRROR_RENDER_MODE ? _mirrorPipeline : _pipeline);

            batch.setModelTransform(gpu::Framebuffer::evalSubregionTexcoordTransform(srcBufferSize, args->_viewport));
            batch.setUniformBuffer(render_utils::slot::buffer::ToneMappingParams, _parametersBuffer);
            batch.setResourceTexture(render_utils::slot::texture::ToneMappingColor, lightingBuffer);
            batch.draw(gpu::TRIANGLE_STRIP, 4);
        });

        // Set full final viewport
        args->_viewport = viewport;

        output = blitFramebuffer;
    }
}