//
//  BlurTask.cpp
//  render/src/render
//
//  Created by Sam Gateau on 6/7/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "BlurTask.h"

#include <gpu/Context.h>
#include <shaders/Shaders.h>

using namespace render;

enum BlurShaderBufferSlots {
    BlurTask_ParamsSlot = 0,
};
enum BlurShaderMapSlots {
    BlurTask_SourceSlot = 0,
    BlurTask_DepthSlot,
};

BlurParams::BlurParams() {
    Params params;
    _parametersBuffer = gpu::BufferView(std::make_shared<gpu::Buffer>(sizeof(Params), (const gpu::Byte*) &params));
    setFilterGaussianTaps(3);
}

void BlurParams::setWidthHeight(int width, int height, bool isStereo) {
    auto resolutionInfo = _parametersBuffer.get<Params>().resolutionInfo;
    bool resChanged = false;
    if (width != resolutionInfo.x || height != resolutionInfo.y) {
        resChanged = true;
        _parametersBuffer.edit<Params>().resolutionInfo = glm::vec4((float) width, (float) height, 1.0f / (float) width, 1.0f / (float) height);
    }

    if (isStereo || resChanged) {
        _parametersBuffer.edit<Params>().stereoInfo = glm::vec4((float)width, (float)height, 1.0f / (float)width, 1.0f / (float)height);
    }
}

void BlurParams::setTexcoordTransform(glm::vec4 texcoordTransformViewport) {
    auto& params = _parametersBuffer.get<Params>();
    if (texcoordTransformViewport != params.texcoordTransform) {
        _parametersBuffer.edit<Params>().texcoordTransform = texcoordTransformViewport;
    }
}

void BlurParams::setFilterRadiusScale(float scale) {
    auto filterInfo = _parametersBuffer.get<Params>().filterInfo;
    if (scale != filterInfo.x) {
        _parametersBuffer.edit<Params>().filterInfo.x = scale;
    }
}

void BlurParams::setFilterNumTaps(int count) {
    assert(count <= BLUR_MAX_NUM_TAPS);
    auto filterInfo = _parametersBuffer.get<Params>().filterInfo;
    if (count != (int)filterInfo.y) {
        _parametersBuffer.edit<Params>().filterInfo.y = count;
    }
}

void BlurParams::setFilterTap(int index, float offset, float value) {
    auto filterTaps = _parametersBuffer.edit<Params>().filterTaps;
    assert(index < BLUR_MAX_NUM_TAPS);
    filterTaps[index].x = offset;
    filterTaps[index].y = value;
}

void BlurParams::setFilterGaussianTaps(int numHalfTaps, float sigma) {
    auto& params = _parametersBuffer.edit<Params>();
    const int numTaps = 2 * numHalfTaps + 1;
    assert(numTaps <= BLUR_MAX_NUM_TAPS);
    assert(sigma > 0.0f);
    const float inverseTwoSigmaSquared = float(0.5 / double(sigma*sigma));
    float totalWeight = 1.0f;
    float weight;
    float offset;
    int i;

    params.filterInfo.y = numTaps;
    params.filterTaps[0].x = 0.0f;
    params.filterTaps[0].y = 1.0f;

    for (i = 0; i < numHalfTaps; i++) {
        offset = i + 1;
        weight = (float)exp(-offset*offset * inverseTwoSigmaSquared);
        params.filterTaps[i + 1].x = offset;
        params.filterTaps[i + 1].y = weight;
        params.filterTaps[i + 1 + numHalfTaps].x = -offset;
        params.filterTaps[i + 1 + numHalfTaps].y = weight;
        totalWeight += 2 * weight;
    }

    // Tap weights will be normalized in shader because side cases on edges of screen
    // won't have the same number of taps as in the center.
}

void BlurParams::setOutputAlpha(float value) {
    value = glm::clamp(value, 0.0f, 1.0f);
    auto filterInfo = _parametersBuffer.get<Params>().filterInfo;
    if (value != filterInfo.z) {
        _parametersBuffer.edit<Params>().filterInfo.z = value;
    }
}

void BlurParams::setDepthPerspective(float oneOverTan2FOV) {
    auto depthInfo = _parametersBuffer.get<Params>().depthInfo;
    if (oneOverTan2FOV != depthInfo.w) {
        _parametersBuffer.edit<Params>().depthInfo.w = oneOverTan2FOV;
    }
}

void BlurParams::setDepthThreshold(float threshold) {
    auto depthInfo = _parametersBuffer.get<Params>().depthInfo;
    if (threshold != depthInfo.x) {
        _parametersBuffer.edit<Params>().depthInfo.x = threshold;
    }
}

void BlurParams::setLinearDepthPosFar(float farPosDepth) {
    auto linearDepthInfo = _parametersBuffer.get<Params>().linearDepthInfo;
    if (farPosDepth != linearDepthInfo.x) {
        _parametersBuffer.edit<Params>().linearDepthInfo.x = farPosDepth;
    }
}


BlurInOutResource::BlurInOutResource(bool generateOutputFramebuffer, unsigned int downsampleFactor) :
    _downsampleFactor(downsampleFactor),
    _generateOutputFramebuffer(generateOutputFramebuffer) {
    assert(downsampleFactor > 0);
}

bool BlurInOutResource::updateResources(const gpu::FramebufferPointer& sourceFramebuffer, Resources& blurringResources) {
    if (!sourceFramebuffer) {
        return false;
    }

    auto blurBufferSize = sourceFramebuffer->getSize();
    
    blurBufferSize.x /= _downsampleFactor;
    blurBufferSize.y /= _downsampleFactor;

    if (_blurredFramebuffer && _blurredFramebuffer->getSize() != blurBufferSize) {
        _blurredFramebuffer.reset();
    }

    if (!_blurredFramebuffer) {
        _blurredFramebuffer = gpu::FramebufferPointer(gpu::Framebuffer::create("blur"));

        // attach depthStencil if present in source
        //if (sourceFramebuffer->hasDepthStencil()) {
        //    _blurredFramebuffer->setDepthStencilBuffer(sourceFramebuffer->getDepthStencilBuffer(), sourceFramebuffer->getDepthStencilBufferFormat());
        //}
        auto blurringSampler = gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_LINEAR_MIP_POINT, gpu::Sampler::WRAP_CLAMP);
        auto blurringTarget = gpu::Texture::createRenderBuffer(sourceFramebuffer->getRenderBuffer(0)->getTexelFormat(), blurBufferSize.x, blurBufferSize.y, gpu::Texture::SINGLE_MIP, blurringSampler);
        _blurredFramebuffer->setRenderBuffer(0, blurringTarget);
    } 

    blurringResources.sourceTexture = sourceFramebuffer->getRenderBuffer(0);
    blurringResources.blurringFramebuffer = _blurredFramebuffer;
    blurringResources.blurringTexture = _blurredFramebuffer->getRenderBuffer(0);

    if (_generateOutputFramebuffer) {
        if (_outputFramebuffer && _outputFramebuffer->getSize() != blurBufferSize) {
            _outputFramebuffer.reset();
        }

        // The job output the blur result in a new Framebuffer spawning here.
        // Let s make sure it s ready for this
        if (!_outputFramebuffer) {
            _outputFramebuffer = gpu::FramebufferPointer(gpu::Framebuffer::create("blurOutput"));

            // attach depthStencil if present in source
         /*   if (sourceFramebuffer->hasDepthStencil()) {
                _outputFramebuffer->setDepthStencilBuffer(sourceFramebuffer->getDepthStencilBuffer(), sourceFramebuffer->getDepthStencilBufferFormat());
            }*/
            auto blurringSampler = gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_LINEAR_MIP_POINT, gpu::Sampler::WRAP_CLAMP);
            auto blurringTarget = gpu::Texture::createRenderBuffer(sourceFramebuffer->getRenderBuffer(0)->getTexelFormat(), blurBufferSize.x, blurBufferSize.y, gpu::Texture::SINGLE_MIP, blurringSampler);
            _outputFramebuffer->setRenderBuffer(0, blurringTarget);
        }

        // Should be good to use the output Framebuffer as final
        blurringResources.finalFramebuffer = _outputFramebuffer;
    } else {
        // Just the reuse the input as output to blur itself.
        blurringResources.finalFramebuffer = sourceFramebuffer;
    }

    return true;
}

BlurGaussian::BlurGaussian() {
    _parameters = std::make_shared<BlurParams>();
}

gpu::PipelinePointer BlurGaussian::getBlurVPipeline() {
    if (!_blurVPipeline) {
        gpu::ShaderPointer program = gpu::Shader::createProgram(shader::render::program::blurGaussianV);
        gpu::StatePointer state = std::make_shared<gpu::State>();

        // Stencil test the curvature pass for objects pixels only, not the background
      //  state->setStencilTest(true, 0xFF, gpu::State::StencilTest(0, 0xFF, gpu::NOT_EQUAL, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP));

        _blurVPipeline = gpu::Pipeline::create(program, state);
    }

    return _blurVPipeline;
}

gpu::PipelinePointer BlurGaussian::getBlurHPipeline() {
    if (!_blurHPipeline) {
        gpu::ShaderPointer program = gpu::Shader::createProgram(shader::render::program::blurGaussianH);
        gpu::StatePointer state = std::make_shared<gpu::State>();

        // Stencil test the curvature pass for objects pixels only, not the background
       // state->setStencilTest(true, 0xFF, gpu::State::StencilTest(0, 0xFF, gpu::NOT_EQUAL, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP));

        _blurHPipeline = gpu::Pipeline::create(program, state);
    }

    return _blurHPipeline;
}

void BlurGaussian::configure(const Config& config) {
    auto state = getBlurHPipeline()->getState();

    _parameters->setFilterRadiusScale(config.filterScale);
    _parameters->setOutputAlpha(config.mix);
    if (config.mix < 1.0f) {
        state->setBlendFunction(config.mix < 1.0f, gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA,
                                gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA);
    } else {
        state->setBlendFunction(false);
    }
}


void BlurGaussian::run(const RenderContextPointer& renderContext, const Inputs& inputs, gpu::FramebufferPointer& blurredFramebuffer) {
    assert(renderContext->args);
    assert(renderContext->args->hasViewFrustum());

    RenderArgs* args = renderContext->args;

    const auto sourceFramebuffer = inputs.get0();
    _inOutResources._generateOutputFramebuffer = inputs.get1();
    _inOutResources._downsampleFactor = inputs.get2();
    _parameters->setFilterGaussianTaps(inputs.get3(), inputs.get4());

    BlurInOutResource::Resources blurringResources;
    if (!_inOutResources.updateResources(sourceFramebuffer, blurringResources)) {
        // early exit if no valid blurring resources
        return;
    }
    blurredFramebuffer = blurringResources.finalFramebuffer;

    auto blurVPipeline = getBlurVPipeline();
    auto blurHPipeline = getBlurHPipeline();
    glm::ivec4 viewport { 0, 0, blurredFramebuffer->getWidth(), blurredFramebuffer->getHeight() };

    glm::ivec2 textureSize = blurredFramebuffer->getSize();
    _parameters->setWidthHeight(blurredFramebuffer->getWidth(), blurredFramebuffer->getHeight(), args->isStereo());
    _parameters->setTexcoordTransform(gpu::Framebuffer::evalSubregionTexcoordTransformCoefficients(textureSize, viewport));

    gpu::doInBatch("BlurGaussian::run", args->_context, [=](gpu::Batch& batch) {
        batch.enableStereo(false);
        batch.setViewportTransform(viewport);

        batch.setUniformBuffer(BlurTask_ParamsSlot, _parameters->_parametersBuffer);

        batch.setFramebuffer(blurringResources.blurringFramebuffer);
        batch.clearColorFramebuffer(gpu::Framebuffer::BUFFER_COLOR0, glm::vec4(0.0));

        batch.setPipeline(blurVPipeline);
        batch.setResourceTexture(BlurTask_SourceSlot, blurringResources.sourceTexture);
        batch.draw(gpu::TRIANGLE_STRIP, 4);

        batch.setFramebuffer(blurringResources.finalFramebuffer);
        if (_inOutResources._generateOutputFramebuffer) {
            batch.clearColorFramebuffer(gpu::Framebuffer::BUFFER_COLOR0, glm::vec4(0.0));
        }

        batch.setPipeline(blurHPipeline);
        batch.setResourceTexture(BlurTask_SourceSlot, blurringResources.blurringTexture);
        batch.draw(gpu::TRIANGLE_STRIP, 4);

        batch.setResourceTexture(BlurTask_SourceSlot, nullptr);
        batch.setUniformBuffer(BlurTask_ParamsSlot, nullptr);
    });
}



BlurGaussianDepthAware::BlurGaussianDepthAware(bool generateOutputFramebuffer, const BlurParamsPointer& params) :
    _inOutResources(generateOutputFramebuffer, 1U),
    _parameters((params ? params : std::make_shared<BlurParams>()))
{
}

gpu::PipelinePointer BlurGaussianDepthAware::getBlurVPipeline() {
    if (!_blurVPipeline) {
        gpu::ShaderPointer program = gpu::Shader::createProgram(shader::render::program::blurGaussianDepthAwareV);
        gpu::StatePointer state = std::make_shared<gpu::State>();

        // Stencil test the curvature pass for objects pixels only, not the background
      //  state->setStencilTest(true, 0xFF, gpu::State::StencilTest(0, 0xFF, gpu::NOT_EQUAL, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP));

        _blurVPipeline = gpu::Pipeline::create(program, state);
    }

    return _blurVPipeline;
}

gpu::PipelinePointer BlurGaussianDepthAware::getBlurHPipeline() {
    if (!_blurHPipeline) {
        gpu::ShaderPointer program = gpu::Shader::createProgram(shader::render::program::blurGaussianDepthAwareH);
        gpu::StatePointer state = std::make_shared<gpu::State>();

        // Stencil test the curvature pass for objects pixels only, not the background
    //    state->setStencilTest(true, 0xFF, gpu::State::StencilTest(0, 0xFF, gpu::NOT_EQUAL, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP));

        _blurHPipeline = gpu::Pipeline::create(program, state);
    }

    return _blurHPipeline;
}

void BlurGaussianDepthAware::configure(const Config& config) {
    _parameters->setFilterRadiusScale(config.filterScale);
    _parameters->setDepthThreshold(config.depthThreshold);
}


void BlurGaussianDepthAware::run(const RenderContextPointer& renderContext, const Inputs& SourceAndDepth, gpu::FramebufferPointer& blurredFramebuffer) {
    assert(renderContext->args);
    assert(renderContext->args->hasViewFrustum());

    RenderArgs* args = renderContext->args;

    auto& sourceFramebuffer = SourceAndDepth.get0();
    auto& depthTexture = SourceAndDepth.get1();

    BlurInOutResource::Resources blurringResources;
    if (!_inOutResources.updateResources(sourceFramebuffer, blurringResources)) {
        // early exit if no valid blurring resources
        return;
    }
    
    blurredFramebuffer = blurringResources.finalFramebuffer;

    auto blurVPipeline = getBlurVPipeline();
    auto blurHPipeline = getBlurHPipeline();

    auto sourceViewport = args->_viewport;

    _parameters->setWidthHeight(sourceViewport.z, sourceViewport.w, args->isStereo());
    glm::ivec2 textureSize(blurringResources.sourceTexture->getDimensions());
    _parameters->setTexcoordTransform(gpu::Framebuffer::evalSubregionTexcoordTransformCoefficients(textureSize, sourceViewport));
    _parameters->setDepthPerspective(args->getViewFrustum().getProjection()[1][1]);
    _parameters->setLinearDepthPosFar(args->getViewFrustum().getFarClip());

    gpu::doInBatch("BlurGaussianDepthAware::run", args->_context, [=](gpu::Batch& batch) {
        batch.enableStereo(false);
        batch.setViewportTransform(sourceViewport);

        batch.setUniformBuffer(BlurTask_ParamsSlot, _parameters->_parametersBuffer);

        batch.setResourceTexture(BlurTask_DepthSlot, depthTexture);

        batch.setFramebuffer(blurringResources.blurringFramebuffer);
        // batch.clearColorFramebuffer(gpu::Framebuffer::BUFFER_COLOR0, glm::vec4(0.0));

        batch.setPipeline(blurVPipeline);
        batch.setResourceTexture(BlurTask_SourceSlot, blurringResources.sourceTexture);
        batch.draw(gpu::TRIANGLE_STRIP, 4);

        batch.setFramebuffer(blurringResources.finalFramebuffer);
        if (_inOutResources._generateOutputFramebuffer) {
            // batch.clearColorFramebuffer(gpu::Framebuffer::BUFFER_COLOR0, glm::vec4(0.0));
        }

        batch.setPipeline(blurHPipeline);
        batch.setResourceTexture(BlurTask_SourceSlot, blurringResources.blurringTexture);
        batch.draw(gpu::TRIANGLE_STRIP, 4);

        batch.setResourceTexture(BlurTask_SourceSlot, nullptr);
        batch.setResourceTexture(BlurTask_DepthSlot, nullptr);
        batch.setUniformBuffer(BlurTask_ParamsSlot, nullptr);
    });
}


