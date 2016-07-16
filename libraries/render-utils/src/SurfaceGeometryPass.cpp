//
//  SurfaceGeometryPass.cpp
//  libraries/render-utils/src/
//
//  Created by Sam Gateau 6/3/2016.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "SurfaceGeometryPass.h"

#include <gpu/Context.h>
#include <gpu/StandardShaderLib.h>


const int SurfaceGeometryPass_FrameTransformSlot = 0;
const int SurfaceGeometryPass_ParamsSlot = 1;
const int SurfaceGeometryPass_DepthMapSlot = 0;
const int SurfaceGeometryPass_NormalMapSlot = 1;

#include "surfaceGeometry_makeLinearDepth_frag.h"

#include "surfaceGeometry_makeCurvature_frag.h"



SurfaceGeometryFramebuffer::SurfaceGeometryFramebuffer() {
}


void SurfaceGeometryFramebuffer::updatePrimaryDepth(const gpu::TexturePointer& depthBuffer) {
    //If the depth buffer or size changed, we need to delete our FBOs
    bool reset = false;
    if ((_primaryDepthTexture != depthBuffer)) {
        _primaryDepthTexture = depthBuffer;
        reset = true;
    }
    if (_primaryDepthTexture) {
        auto newFrameSize = glm::ivec2(_primaryDepthTexture->getDimensions());
        if (_frameSize != newFrameSize) {
            _frameSize = newFrameSize;
            reset = true;
        }
    }

    if (reset) {
        clear();
    }
}

void SurfaceGeometryFramebuffer::clear() {
    _linearDepthFramebuffer.reset();
    _linearDepthTexture.reset();
    _curvatureFramebuffer.reset();
    _curvatureTexture.reset();
}

void SurfaceGeometryFramebuffer::allocate() {

    auto width = _frameSize.x;
    auto height = _frameSize.y;

    // For Linear Depth:
    _linearDepthTexture = gpu::TexturePointer(gpu::Texture::create2D(gpu::Element(gpu::SCALAR, gpu::FLOAT, gpu::RGB), width, height, gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_LINEAR_MIP_POINT)));
    _linearDepthFramebuffer = gpu::FramebufferPointer(gpu::Framebuffer::create());
    _linearDepthFramebuffer->setRenderBuffer(0, _linearDepthTexture);
    _linearDepthFramebuffer->setDepthStencilBuffer(_primaryDepthTexture, _primaryDepthTexture->getTexelFormat());

    _curvatureTexture = gpu::TexturePointer(gpu::Texture::create2D(gpu::Element::COLOR_RGBA_32, width >> getResolutionLevel(), height >> getResolutionLevel(), gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_LINEAR_MIP_POINT)));
    _curvatureFramebuffer = gpu::FramebufferPointer(gpu::Framebuffer::create());
    _curvatureFramebuffer->setRenderBuffer(0, _curvatureTexture);
//    _curvatureFramebuffer->setDepthStencilBuffer(_primaryDepthTexture, _primaryDepthTexture->getTexelFormat());
}

gpu::FramebufferPointer SurfaceGeometryFramebuffer::getLinearDepthFramebuffer() {
    if (!_linearDepthFramebuffer) {
        allocate();
    }
    return _linearDepthFramebuffer;
}

gpu::TexturePointer SurfaceGeometryFramebuffer::getLinearDepthTexture() {
    if (!_linearDepthTexture) {
        allocate();
    }
    return _linearDepthTexture;
}

gpu::FramebufferPointer SurfaceGeometryFramebuffer::getCurvatureFramebuffer() {
    if (!_curvatureFramebuffer) {
        allocate();
    }
    return _curvatureFramebuffer;
}

gpu::TexturePointer SurfaceGeometryFramebuffer::getCurvatureTexture() {
    if (!_curvatureTexture) {
        allocate();
    }
    return _curvatureTexture;
}

void SurfaceGeometryFramebuffer::setResolutionLevel(int resolutionLevel) {
    if (resolutionLevel != getResolutionLevel()) {
        clear();
        _resolutionLevel = resolutionLevel;
    }
}


SurfaceGeometryPass::SurfaceGeometryPass() {
    Parameters parameters;
    _parametersBuffer = gpu::BufferView(std::make_shared<gpu::Buffer>(sizeof(Parameters), (const gpu::Byte*) &parameters));
}

void SurfaceGeometryPass::configure(const Config& config) {

    if (config.depthThreshold != getCurvatureDepthThreshold()) {
        _parametersBuffer.edit<Parameters>().curvatureInfo.x = config.depthThreshold;
    }

    if (config.basisScale != getCurvatureBasisScale()) {
        _parametersBuffer.edit<Parameters>().curvatureInfo.y = config.basisScale;
    }

    if (config.curvatureScale != getCurvatureScale()) {
        _parametersBuffer.edit<Parameters>().curvatureInfo.w = config.curvatureScale;
    }

    if (!_surfaceGeometryFramebuffer) {
        _surfaceGeometryFramebuffer = std::make_shared<SurfaceGeometryFramebuffer>();
    }
    _surfaceGeometryFramebuffer->setResolutionLevel(config.resolutionLevel);
}


void SurfaceGeometryPass::run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext, const Inputs& inputs, Outputs& curvatureAndDepth) {
    assert(renderContext->args);
    assert(renderContext->args->hasViewFrustum());

    RenderArgs* args = renderContext->args;

    const auto frameTransform = inputs.get0();
    const auto deferredFramebuffer = inputs.get1();

    if (!_surfaceGeometryFramebuffer) {
        _surfaceGeometryFramebuffer = std::make_shared<SurfaceGeometryFramebuffer>();
    }
    _surfaceGeometryFramebuffer->updatePrimaryDepth(deferredFramebuffer->getPrimaryDepthTexture());

    auto depthBuffer = deferredFramebuffer->getPrimaryDepthTexture();
    auto normalTexture = deferredFramebuffer->getDeferredNormalTexture();

    auto linearDepthFBO = _surfaceGeometryFramebuffer->getLinearDepthFramebuffer();
    auto linearDepthTexture = _surfaceGeometryFramebuffer->getLinearDepthTexture();
    auto curvatureFBO = _surfaceGeometryFramebuffer->getCurvatureFramebuffer();
    auto curvatureTexture = _surfaceGeometryFramebuffer->getCurvatureTexture();

    curvatureAndDepth.edit0() = _surfaceGeometryFramebuffer;
    curvatureAndDepth.edit1() = curvatureFBO;
    curvatureAndDepth.edit2() = linearDepthTexture;

    auto linearDepthPipeline = getLinearDepthPipeline();
    auto curvaturePipeline = getCurvaturePipeline();
    
  //   gpu::doInBatch(args->_context, [&](gpu::Batch& batch) {
  //   _gpuTimer.begin(batch);
  //   });
    
    auto depthViewport = args->_viewport;
    auto curvatureViewport = depthViewport >> _surfaceGeometryFramebuffer->getResolutionLevel();

    gpu::doInBatch(args->_context, [=](gpu::Batch& batch) {
        _gpuTimer.begin(batch);
        batch.enableStereo(false);

        batch.setViewportTransform(depthViewport);
        batch.setProjectionTransform(glm::mat4());
        batch.setViewTransform(Transform());
        batch.setModelTransform(gpu::Framebuffer::evalSubregionTexcoordTransform(_surfaceGeometryFramebuffer->getDepthFrameSize(), depthViewport));

        batch.setUniformBuffer(SurfaceGeometryPass_FrameTransformSlot, frameTransform->getFrameTransformBuffer());
        batch.setUniformBuffer(SurfaceGeometryPass_ParamsSlot, _parametersBuffer);

        // Pyramid pass
        batch.setFramebuffer(linearDepthFBO);
        batch.clearColorFramebuffer(gpu::Framebuffer::BUFFER_COLOR0, glm::vec4(args->getViewFrustum().getFarClip(), 0.0f, 0.0f, 0.0f));
        batch.setPipeline(linearDepthPipeline);
        batch.setResourceTexture(SurfaceGeometryPass_DepthMapSlot, depthBuffer);
        batch.draw(gpu::TRIANGLE_STRIP, 4);

        batch.setViewportTransform(curvatureViewport);
        batch.setModelTransform(gpu::Framebuffer::evalSubregionTexcoordTransform(_surfaceGeometryFramebuffer->getCurvatureFrameSize(), curvatureViewport));

        // Curvature pass
        batch.setFramebuffer(curvatureFBO);
        batch.clearColorFramebuffer(gpu::Framebuffer::BUFFER_COLOR0, glm::vec4(0.0));
        batch.setPipeline(curvaturePipeline);
        batch.setResourceTexture(SurfaceGeometryPass_DepthMapSlot, linearDepthTexture);
        batch.setResourceTexture(SurfaceGeometryPass_NormalMapSlot, normalTexture);
        batch.draw(gpu::TRIANGLE_STRIP, 4);
        batch.setResourceTexture(SurfaceGeometryPass_DepthMapSlot, nullptr);
        batch.setResourceTexture(SurfaceGeometryPass_NormalMapSlot, nullptr);

        _gpuTimer.end(batch);
    });
    
   // gpu::doInBatch(args->_context, [&](gpu::Batch& batch) {
   //  _gpuTimer.end(batch);
   //  });

    auto config = std::static_pointer_cast<Config>(renderContext->jobConfig);
    config->gpuTime = _gpuTimer.getAverage();
}

const gpu::PipelinePointer& SurfaceGeometryPass::getLinearDepthPipeline() {
    if (!_linearDepthPipeline) {
        auto vs = gpu::StandardShaderLib::getDrawViewportQuadTransformTexcoordVS();
        auto ps = gpu::Shader::createPixel(std::string(surfaceGeometry_makeLinearDepth_frag));
        gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);
        
        gpu::Shader::BindingSet slotBindings;
        slotBindings.insert(gpu::Shader::Binding(std::string("deferredFrameTransformBuffer"), SurfaceGeometryPass_FrameTransformSlot));
        slotBindings.insert(gpu::Shader::Binding(std::string("linearDepthMap"), SurfaceGeometryPass_DepthMapSlot));
        gpu::Shader::makeProgram(*program, slotBindings);
        
        
        gpu::StatePointer state = gpu::StatePointer(new gpu::State());
        
        // Stencil test the curvature pass for objects pixels only, not the background
        state->setStencilTest(true, 0xFF, gpu::State::StencilTest(0, 0xFF, gpu::NOT_EQUAL, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP));
        
        state->setColorWriteMask(true, false, false, false);
        
        // Good to go add the brand new pipeline
        _linearDepthPipeline = gpu::Pipeline::create(program, state);
    }
    
    return _linearDepthPipeline;
}

const gpu::PipelinePointer& SurfaceGeometryPass::getCurvaturePipeline() {
    if (!_curvaturePipeline) {
        auto vs = gpu::StandardShaderLib::getDrawViewportQuadTransformTexcoordVS();
        auto ps = gpu::Shader::createPixel(std::string(surfaceGeometry_makeCurvature_frag));
        gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);

        gpu::Shader::BindingSet slotBindings;
        slotBindings.insert(gpu::Shader::Binding(std::string("deferredFrameTransformBuffer"), SurfaceGeometryPass_FrameTransformSlot));
        slotBindings.insert(gpu::Shader::Binding(std::string("surfaceGeometryParamsBuffer"), SurfaceGeometryPass_ParamsSlot));
        slotBindings.insert(gpu::Shader::Binding(std::string("depthMap"), SurfaceGeometryPass_DepthMapSlot));
        slotBindings.insert(gpu::Shader::Binding(std::string("normalMap"), SurfaceGeometryPass_NormalMapSlot));
        gpu::Shader::makeProgram(*program, slotBindings);


        gpu::StatePointer state = gpu::StatePointer(new gpu::State());

        // Stencil test the curvature pass for objects pixels only, not the background
        state->setStencilTest(true, 0xFF, gpu::State::StencilTest(0, 0xFF, gpu::NOT_EQUAL, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP));

        // Good to go add the brand new pipeline
        _curvaturePipeline = gpu::Pipeline::create(program, state);
    }

    return _curvaturePipeline;
}
