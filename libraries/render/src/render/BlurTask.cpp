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
#include <gpu/StandardShaderLib.h>

#include "blurGaussianV_frag.h"
#include "blurGaussianH_frag.h"

#include "blurGaussianDepthAwareV_frag.h"
#include "blurGaussianDepthAwareH_frag.h"

using namespace render;

enum BlurShaderBufferSlots {
    BlurTask_ParamsSlot = 0,
};
enum BlurShaderMapSlots {
    BlurTask_SourceSlot = 0,
    BlurTask_DepthSlot,
};

const float BLUR_NUM_SAMPLES = 7.0f;

BlurParams::BlurParams() {
    Params params;
    _parametersBuffer = gpu::BufferView(std::make_shared<gpu::Buffer>(sizeof(Params), (const gpu::Byte*) &params));
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

void BlurParams::setTexcoordTransform(const glm::vec4 texcoordTransformViewport) {
    auto texcoordTransform = _parametersBuffer.get<Params>().texcoordTransform;
    if (texcoordTransformViewport != texcoordTransform) {
        _parametersBuffer.edit<Params>().texcoordTransform = texcoordTransform;
    }
}

void BlurParams::setFilterRadiusScale(float scale) {
    auto filterInfo = _parametersBuffer.get<Params>().filterInfo;
    if (scale != filterInfo.x) {
        _parametersBuffer.edit<Params>().filterInfo.x = scale;
        _parametersBuffer.edit<Params>().filterInfo.y = scale / BLUR_NUM_SAMPLES;
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


BlurInOutResource::BlurInOutResource(bool generateOutputFramebuffer) :
_generateOutputFramebuffer(generateOutputFramebuffer)
{

}

bool BlurInOutResource::updateResources(const gpu::FramebufferPointer& sourceFramebuffer, Resources& blurringResources) {
    if (!sourceFramebuffer) {
        return false;
    }
    if (_blurredFramebuffer && _blurredFramebuffer->getSize() != sourceFramebuffer->getSize()) {
        _blurredFramebuffer.reset();
    }

    if (!_blurredFramebuffer) {
        _blurredFramebuffer = gpu::FramebufferPointer(gpu::Framebuffer::create("blur"));

        // attach depthStencil if present in source
        //if (sourceFramebuffer->hasDepthStencil()) {
        //    _blurredFramebuffer->setDepthStencilBuffer(sourceFramebuffer->getDepthStencilBuffer(), sourceFramebuffer->getDepthStencilBufferFormat());
        //}
        auto blurringSampler = gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_LINEAR_MIP_POINT);
        auto blurringTarget = gpu::Texture::create2D(sourceFramebuffer->getRenderBuffer(0)->getTexelFormat(), sourceFramebuffer->getWidth(), sourceFramebuffer->getHeight(), gpu::Texture::SINGLE_MIP, blurringSampler);
        _blurredFramebuffer->setRenderBuffer(0, blurringTarget);
    } 

    blurringResources.sourceTexture = sourceFramebuffer->getRenderBuffer(0);
    blurringResources.blurringFramebuffer = _blurredFramebuffer;
    blurringResources.blurringTexture = _blurredFramebuffer->getRenderBuffer(0);

    if (_generateOutputFramebuffer) {
        if (_outputFramebuffer && _outputFramebuffer->getSize() != sourceFramebuffer->getSize()) {
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
            auto blurringSampler = gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_LINEAR_MIP_POINT);
            auto blurringTarget = gpu::Texture::create2D(sourceFramebuffer->getRenderBuffer(0)->getTexelFormat(), sourceFramebuffer->getWidth(), sourceFramebuffer->getHeight(), gpu::Texture::SINGLE_MIP, blurringSampler);
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

BlurGaussian::BlurGaussian(bool generateOutputFramebuffer) :
    _inOutResources(generateOutputFramebuffer)
{
    _parameters = std::make_shared<BlurParams>();
}

gpu::PipelinePointer BlurGaussian::getBlurVPipeline() {
    if (!_blurVPipeline) {
        auto vs = gpu::StandardShaderLib::getDrawUnitQuadTexcoordVS();
        auto ps = gpu::Shader::createPixel(std::string(blurGaussianV_frag));
        gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);

        gpu::Shader::BindingSet slotBindings;
        slotBindings.insert(gpu::Shader::Binding(std::string("blurParamsBuffer"), BlurTask_ParamsSlot));
        slotBindings.insert(gpu::Shader::Binding(std::string("sourceMap"), BlurTask_SourceSlot));
        gpu::Shader::makeProgram(*program, slotBindings);

        gpu::StatePointer state = gpu::StatePointer(new gpu::State());

        // Stencil test the curvature pass for objects pixels only, not the background
      //  state->setStencilTest(true, 0xFF, gpu::State::StencilTest(0, 0xFF, gpu::NOT_EQUAL, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP));

        _blurVPipeline = gpu::Pipeline::create(program, state);
    }

    return _blurVPipeline;
}

gpu::PipelinePointer BlurGaussian::getBlurHPipeline() {
    if (!_blurHPipeline) {
        auto vs = gpu::StandardShaderLib::getDrawUnitQuadTexcoordVS();
        auto ps = gpu::Shader::createPixel(std::string(blurGaussianH_frag));
        gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);

        gpu::Shader::BindingSet slotBindings;
        slotBindings.insert(gpu::Shader::Binding(std::string("blurParamsBuffer"), BlurTask_ParamsSlot));
        slotBindings.insert(gpu::Shader::Binding(std::string("sourceMap"), BlurTask_SourceSlot));
        gpu::Shader::makeProgram(*program, slotBindings);

        gpu::StatePointer state = gpu::StatePointer(new gpu::State());

        // Stencil test the curvature pass for objects pixels only, not the background
       // state->setStencilTest(true, 0xFF, gpu::State::StencilTest(0, 0xFF, gpu::NOT_EQUAL, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP));

        _blurHPipeline = gpu::Pipeline::create(program, state);
    }

    return _blurHPipeline;
}

void BlurGaussian::configure(const Config& config) {
    _parameters->setFilterRadiusScale(config.filterScale);
}


void BlurGaussian::run(const RenderContextPointer& renderContext, const gpu::FramebufferPointer& sourceFramebuffer, gpu::FramebufferPointer& blurredFramebuffer) {
    assert(renderContext->args);
    assert(renderContext->args->hasViewFrustum());

    RenderArgs* args = renderContext->args;


    BlurInOutResource::Resources blurringResources;
    if (!_inOutResources.updateResources(sourceFramebuffer, blurringResources)) {
        // early exit if no valid blurring resources
        return;
    }
    blurredFramebuffer = blurringResources.finalFramebuffer;

    auto blurVPipeline = getBlurVPipeline();
    auto blurHPipeline = getBlurHPipeline();

    _parameters->setWidthHeight(args->_viewport.z, args->_viewport.w, args->isStereo());
    glm::ivec2 textureSize(blurringResources.sourceTexture->getDimensions());
    _parameters->setTexcoordTransform(gpu::Framebuffer::evalSubregionTexcoordTransformCoefficients(textureSize, args->_viewport));

    gpu::doInBatch(args->_context, [=](gpu::Batch& batch) {
        batch.enableStereo(false);
        batch.setViewportTransform(args->_viewport);

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
    _inOutResources(generateOutputFramebuffer),
    _parameters((params ? params : std::make_shared<BlurParams>()))
{
}

gpu::PipelinePointer BlurGaussianDepthAware::getBlurVPipeline() {
    if (!_blurVPipeline) {
        auto vs = gpu::StandardShaderLib::getDrawUnitQuadTexcoordVS();
        auto ps = gpu::Shader::createPixel(std::string(blurGaussianDepthAwareV_frag));
        gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);

        gpu::Shader::BindingSet slotBindings;
        slotBindings.insert(gpu::Shader::Binding(std::string("blurParamsBuffer"), BlurTask_ParamsSlot));
        slotBindings.insert(gpu::Shader::Binding(std::string("sourceMap"), BlurTask_SourceSlot));
        slotBindings.insert(gpu::Shader::Binding(std::string("depthMap"), BlurTask_DepthSlot));
        gpu::Shader::makeProgram(*program, slotBindings);

        gpu::StatePointer state = gpu::StatePointer(new gpu::State());

        // Stencil test the curvature pass for objects pixels only, not the background
      //  state->setStencilTest(true, 0xFF, gpu::State::StencilTest(0, 0xFF, gpu::NOT_EQUAL, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP));

        _blurVPipeline = gpu::Pipeline::create(program, state);
    }

    return _blurVPipeline;
}

gpu::PipelinePointer BlurGaussianDepthAware::getBlurHPipeline() {
    if (!_blurHPipeline) {
        auto vs = gpu::StandardShaderLib::getDrawUnitQuadTexcoordVS();
        auto ps = gpu::Shader::createPixel(std::string(blurGaussianDepthAwareH_frag));
        gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);

        gpu::Shader::BindingSet slotBindings;
        slotBindings.insert(gpu::Shader::Binding(std::string("blurParamsBuffer"), BlurTask_ParamsSlot));
        slotBindings.insert(gpu::Shader::Binding(std::string("sourceMap"), BlurTask_SourceSlot));
        slotBindings.insert(gpu::Shader::Binding(std::string("depthMap"), BlurTask_DepthSlot));
        gpu::Shader::makeProgram(*program, slotBindings);

        gpu::StatePointer state = gpu::StatePointer(new gpu::State());

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

    gpu::doInBatch(args->_context, [=](gpu::Batch& batch) {
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


