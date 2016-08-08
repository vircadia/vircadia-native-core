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

#include <limits>

#include <gpu/Context.h>
#include <gpu/StandardShaderLib.h>


const int DepthLinearPass_FrameTransformSlot = 0;
const int DepthLinearPass_DepthMapSlot = 0;
const int DepthLinearPass_NormalMapSlot = 1;

const int SurfaceGeometryPass_FrameTransformSlot = 0;
const int SurfaceGeometryPass_ParamsSlot = 1;
const int SurfaceGeometryPass_DepthMapSlot = 0;
const int SurfaceGeometryPass_NormalMapSlot = 1;

#include "surfaceGeometry_makeLinearDepth_frag.h"
#include "surfaceGeometry_downsampleDepthNormal_frag.h"

#include "surfaceGeometry_makeCurvature_frag.h"



LinearDepthFramebuffer::LinearDepthFramebuffer() {
}


void LinearDepthFramebuffer::updatePrimaryDepth(const gpu::TexturePointer& depthBuffer) {
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
            _halfFrameSize = newFrameSize >> 1;

            reset = true;
        }
    }

    if (reset) {
        clear();
    }
}

void LinearDepthFramebuffer::clear() {
    _linearDepthFramebuffer.reset();
    _linearDepthTexture.reset();
	_downsampleFramebuffer.reset();
	_halfLinearDepthTexture.reset();
	_halfNormalTexture.reset();
}

void LinearDepthFramebuffer::allocate() {

    auto width = _frameSize.x;
    auto height = _frameSize.y;

    // For Linear Depth:
    _linearDepthTexture = gpu::TexturePointer(gpu::Texture::create2D(gpu::Element(gpu::SCALAR, gpu::FLOAT, gpu::RGB), width, height,
        gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_LINEAR_MIP_POINT)));
 //   _linearDepthTexture->autoGenerateMips(5);
    _linearDepthFramebuffer = gpu::FramebufferPointer(gpu::Framebuffer::create());
    _linearDepthFramebuffer->setRenderBuffer(0, _linearDepthTexture);
    _linearDepthFramebuffer->setDepthStencilBuffer(_primaryDepthTexture, _primaryDepthTexture->getTexelFormat());

    // For Downsampling:
    _halfLinearDepthTexture = gpu::TexturePointer(gpu::Texture::create2D(gpu::Element(gpu::SCALAR, gpu::FLOAT, gpu::RGB), _halfFrameSize.x, _halfFrameSize.y,
        gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_LINEAR_MIP_POINT)));
	_halfLinearDepthTexture->autoGenerateMips(5);

    _halfNormalTexture = gpu::TexturePointer(gpu::Texture::create2D(gpu::Element(gpu::VEC3, gpu::NUINT8, gpu::RGB), _halfFrameSize.x, _halfFrameSize.y,
        gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_LINEAR_MIP_POINT)));

    _downsampleFramebuffer = gpu::FramebufferPointer(gpu::Framebuffer::create());
    _downsampleFramebuffer->setRenderBuffer(0, _halfLinearDepthTexture);
    _downsampleFramebuffer->setRenderBuffer(1, _halfNormalTexture);
}

gpu::FramebufferPointer LinearDepthFramebuffer::getLinearDepthFramebuffer() {
    if (!_linearDepthFramebuffer) {
        allocate();
    }
    return _linearDepthFramebuffer;
}

gpu::TexturePointer LinearDepthFramebuffer::getLinearDepthTexture() {
    if (!_linearDepthTexture) {
        allocate();
    }
    return _linearDepthTexture;
}

gpu::FramebufferPointer LinearDepthFramebuffer::getDownsampleFramebuffer() {
    if (!_downsampleFramebuffer) {
        allocate();
    }
    return _downsampleFramebuffer;
}

gpu::TexturePointer LinearDepthFramebuffer::getHalfLinearDepthTexture() {
    if (!_halfLinearDepthTexture) {
        allocate();
    }
    return _halfLinearDepthTexture;
}

gpu::TexturePointer LinearDepthFramebuffer::getHalfNormalTexture() {
    if (!_halfNormalTexture) {
        allocate();
    }
    return _halfNormalTexture;
}


LinearDepthPass::LinearDepthPass() {
}

void LinearDepthPass::configure(const Config& config) {
}

void LinearDepthPass::run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext, const Inputs& inputs, Outputs& outputs) {
    assert(renderContext->args);
    assert(renderContext->args->hasViewFrustum());

    RenderArgs* args = renderContext->args;

    const auto frameTransform = inputs.get0();
    const auto deferredFramebuffer = inputs.get1();

    if (!_linearDepthFramebuffer) {
        _linearDepthFramebuffer = std::make_shared<LinearDepthFramebuffer>();
    }
    _linearDepthFramebuffer->updatePrimaryDepth(deferredFramebuffer->getPrimaryDepthTexture());

    auto depthBuffer = deferredFramebuffer->getPrimaryDepthTexture();
    auto normalTexture = deferredFramebuffer->getDeferredNormalTexture();

    auto linearDepthFBO = _linearDepthFramebuffer->getLinearDepthFramebuffer();
    auto linearDepthTexture = _linearDepthFramebuffer->getLinearDepthTexture();

    auto downsampleFBO = _linearDepthFramebuffer->getDownsampleFramebuffer();
    auto halfLinearDepthTexture = _linearDepthFramebuffer->getHalfLinearDepthTexture();
    auto halfNormalTexture = _linearDepthFramebuffer->getHalfNormalTexture();

    outputs.edit0() = _linearDepthFramebuffer;
    outputs.edit1() = linearDepthFBO;
    outputs.edit2() = linearDepthTexture;
    outputs.edit3() = halfLinearDepthTexture;
    outputs.edit4() = halfNormalTexture;
   
    auto linearDepthPipeline = getLinearDepthPipeline();
    auto downsamplePipeline = getDownsamplePipeline();

    auto depthViewport = args->_viewport;
    auto halfViewport = depthViewport >> 1;
    float clearLinearDepth = args->getViewFrustum().getFarClip() * 2.0f;

    gpu::doInBatch(args->_context, [=](gpu::Batch& batch) {
        _gpuTimer.begin(batch);
        batch.enableStereo(false);

        batch.setViewportTransform(depthViewport);
        batch.setProjectionTransform(glm::mat4());
        batch.setViewTransform(Transform());
        batch.setModelTransform(gpu::Framebuffer::evalSubregionTexcoordTransform(_linearDepthFramebuffer->getDepthFrameSize(), depthViewport));

        batch.setUniformBuffer(DepthLinearPass_FrameTransformSlot, frameTransform->getFrameTransformBuffer());

        // LinearDepth
        batch.setFramebuffer(linearDepthFBO);
        batch.clearColorFramebuffer(gpu::Framebuffer::BUFFER_COLOR0, glm::vec4(clearLinearDepth, 0.0f, 0.0f, 0.0f));
        batch.setPipeline(linearDepthPipeline);
        batch.setResourceTexture(DepthLinearPass_DepthMapSlot, depthBuffer);
        batch.draw(gpu::TRIANGLE_STRIP, 4);

        // Downsample
        batch.setViewportTransform(halfViewport);
       
        batch.setFramebuffer(downsampleFBO);
        batch.setResourceTexture(DepthLinearPass_DepthMapSlot, linearDepthTexture);
        batch.setResourceTexture(DepthLinearPass_NormalMapSlot, normalTexture);
        batch.setPipeline(downsamplePipeline);
        batch.draw(gpu::TRIANGLE_STRIP, 4);
        
        _gpuTimer.end(batch);
    });

    auto config = std::static_pointer_cast<Config>(renderContext->jobConfig);
    config->gpuTime = _gpuTimer.getAverage();
}


const gpu::PipelinePointer& LinearDepthPass::getLinearDepthPipeline() {
    if (!_linearDepthPipeline) {
        auto vs = gpu::StandardShaderLib::getDrawViewportQuadTransformTexcoordVS();
        auto ps = gpu::Shader::createPixel(std::string(surfaceGeometry_makeLinearDepth_frag));
        gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);

        gpu::Shader::BindingSet slotBindings;
        slotBindings.insert(gpu::Shader::Binding(std::string("deferredFrameTransformBuffer"), DepthLinearPass_FrameTransformSlot));
        slotBindings.insert(gpu::Shader::Binding(std::string("depthMap"), DepthLinearPass_DepthMapSlot));
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


const gpu::PipelinePointer& LinearDepthPass::getDownsamplePipeline() {
    if (!_downsamplePipeline) {
        auto vs = gpu::StandardShaderLib::getDrawViewportQuadTransformTexcoordVS();
        auto ps = gpu::Shader::createPixel(std::string(surfaceGeometry_downsampleDepthNormal_frag));
        gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);

        gpu::Shader::BindingSet slotBindings;
        slotBindings.insert(gpu::Shader::Binding(std::string("deferredFrameTransformBuffer"), DepthLinearPass_FrameTransformSlot));
        slotBindings.insert(gpu::Shader::Binding(std::string("linearDepthMap"), DepthLinearPass_DepthMapSlot));
        slotBindings.insert(gpu::Shader::Binding(std::string("normalMap"), DepthLinearPass_NormalMapSlot));
        gpu::Shader::makeProgram(*program, slotBindings);


        gpu::StatePointer state = gpu::StatePointer(new gpu::State());

        state->setColorWriteMask(true, true, true, false);

        // Good to go add the brand new pipeline
        _downsamplePipeline = gpu::Pipeline::create(program, state);
    }

    return _downsamplePipeline;
}


//#define USE_STENCIL_TEST

SurfaceGeometryFramebuffer::SurfaceGeometryFramebuffer() {
}

void SurfaceGeometryFramebuffer::updateLinearDepth(const gpu::TexturePointer& linearDepthBuffer) {
    //If the depth buffer or size changed, we need to delete our FBOs
    bool reset = false;
    if ((_linearDepthTexture != linearDepthBuffer)) {
        _linearDepthTexture = linearDepthBuffer;
        reset = true;
    }
    if (_linearDepthTexture) {
        auto newFrameSize = glm::ivec2(_linearDepthTexture->getDimensions());
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
    _curvatureFramebuffer.reset();
    _curvatureTexture.reset();
    _lowCurvatureFramebuffer.reset();
    _lowCurvatureTexture.reset();
    _blurringFramebuffer.reset();
    _blurringTexture.reset();
}

gpu::TexturePointer SurfaceGeometryFramebuffer::getLinearDepthTexture() {
    return _linearDepthTexture;
}

void SurfaceGeometryFramebuffer::allocate() {

    auto width = _frameSize.x;
    auto height = _frameSize.y;

    _curvatureTexture = gpu::TexturePointer(gpu::Texture::create2D(gpu::Element::COLOR_RGBA_32, width, height, gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_LINEAR_MIP_POINT)));
    _curvatureFramebuffer = gpu::FramebufferPointer(gpu::Framebuffer::create());
    _curvatureFramebuffer->setRenderBuffer(0, _curvatureTexture);

    _lowCurvatureTexture = gpu::TexturePointer(gpu::Texture::create2D(gpu::Element::COLOR_RGBA_32, width, height, gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_LINEAR_MIP_POINT)));
    _lowCurvatureFramebuffer = gpu::FramebufferPointer(gpu::Framebuffer::create());
    _lowCurvatureFramebuffer->setRenderBuffer(0, _lowCurvatureTexture);

    _blurringTexture = gpu::TexturePointer(gpu::Texture::create2D(gpu::Element::COLOR_RGBA_32, width, height, gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_LINEAR_MIP_POINT)));
    _blurringFramebuffer = gpu::FramebufferPointer(gpu::Framebuffer::create());
    _blurringFramebuffer->setRenderBuffer(0, _blurringTexture);
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

gpu::FramebufferPointer SurfaceGeometryFramebuffer::getLowCurvatureFramebuffer() {
    if (!_lowCurvatureFramebuffer) {
        allocate();
    }
    return _lowCurvatureFramebuffer;
}

gpu::TexturePointer SurfaceGeometryFramebuffer::getLowCurvatureTexture() {
    if (!_lowCurvatureTexture) {
        allocate();
    }
    return _lowCurvatureTexture;
}

gpu::FramebufferPointer SurfaceGeometryFramebuffer::getBlurringFramebuffer() {
    if (!_blurringFramebuffer) {
        allocate();
    }
    return _blurringFramebuffer;
}

gpu::TexturePointer SurfaceGeometryFramebuffer::getBlurringTexture() {
    if (!_blurringTexture) {
        allocate();
    }
    return _blurringTexture;
}

void SurfaceGeometryFramebuffer::setResolutionLevel(int resolutionLevel) {
    if (resolutionLevel != getResolutionLevel()) {
        clear();
        _resolutionLevel = resolutionLevel;
    }
}

SurfaceGeometryPass::SurfaceGeometryPass() :
    _diffusePass(false)
{
    Parameters parameters;
    _parametersBuffer = gpu::BufferView(std::make_shared<gpu::Buffer>(sizeof(Parameters), (const gpu::Byte*) &parameters));
}

void SurfaceGeometryPass::configure(const Config& config) {
    const float CM_TO_M = 0.01f;

    if ((config.depthThreshold * CM_TO_M) != getCurvatureDepthThreshold()) {
        _parametersBuffer.edit<Parameters>().curvatureInfo.x = config.depthThreshold * CM_TO_M;
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
    if (config.resolutionLevel != getResolutionLevel()) {
        _parametersBuffer.edit<Parameters>().resolutionInfo.w = config.resolutionLevel;
    }

    auto filterRadius = (getResolutionLevel() > 0 ? config.diffuseFilterScale / 2.0f : config.diffuseFilterScale);
    _diffusePass.getParameters()->setFilterRadiusScale(filterRadius);
    _diffusePass.getParameters()->setDepthThreshold(config.diffuseDepthThreshold);
    
}


void SurfaceGeometryPass::run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext, const Inputs& inputs, Outputs& outputs) {
    assert(renderContext->args);
    assert(renderContext->args->hasViewFrustum());

    RenderArgs* args = renderContext->args;

    const auto frameTransform = inputs.get0();
    const auto deferredFramebuffer = inputs.get1();
    const auto linearDepthFramebuffer = inputs.get2();


    auto linearDepthTexture = linearDepthFramebuffer->getLinearDepthTexture();
    auto normalTexture = deferredFramebuffer->getDeferredNormalTexture();
    auto sourceViewport = args->_viewport;
    auto curvatureViewport = sourceViewport;

    if (_surfaceGeometryFramebuffer->getResolutionLevel() > 0) {
        linearDepthTexture = linearDepthFramebuffer->getHalfLinearDepthTexture();
        normalTexture = linearDepthFramebuffer->getHalfNormalTexture();
        curvatureViewport = curvatureViewport >> _surfaceGeometryFramebuffer->getResolutionLevel();
    }

    if (!_surfaceGeometryFramebuffer) {
        _surfaceGeometryFramebuffer = std::make_shared<SurfaceGeometryFramebuffer>();
    }
    _surfaceGeometryFramebuffer->updateLinearDepth(linearDepthTexture);

    auto curvatureFramebuffer = _surfaceGeometryFramebuffer->getCurvatureFramebuffer();
    auto curvatureTexture = _surfaceGeometryFramebuffer->getCurvatureTexture();
#ifdef USE_STENCIL_TEST
    if (curvatureFramebuffer->getDepthStencilBuffer() != deferredFramebuffer->getPrimaryDepthTexture()) {
        curvatureFramebuffer->setDepthStencilBuffer(deferredFramebuffer->getPrimaryDepthTexture(), deferredFramebuffer->getPrimaryDepthTexture()->getTexelFormat());
    }
#endif

    auto lowCurvatureFramebuffer = _surfaceGeometryFramebuffer->getLowCurvatureFramebuffer();
    auto lowCurvatureTexture = _surfaceGeometryFramebuffer->getLowCurvatureTexture();
 
    auto blurringFramebuffer = _surfaceGeometryFramebuffer->getBlurringFramebuffer();
    auto blurringTexture = _surfaceGeometryFramebuffer->getBlurringTexture();

    outputs.edit0() = _surfaceGeometryFramebuffer;
    outputs.edit1() = curvatureFramebuffer;
    outputs.edit2() = curvatureFramebuffer;
    outputs.edit3() = lowCurvatureFramebuffer;

    auto curvaturePipeline = getCurvaturePipeline();
    auto diffuseVPipeline = _diffusePass.getBlurVPipeline();
    auto diffuseHPipeline = _diffusePass.getBlurHPipeline();

    _diffusePass.getParameters()->setWidthHeight(curvatureViewport.z, curvatureViewport.w, args->_context->isStereo());
    glm::ivec2 textureSize(curvatureTexture->getDimensions());
    _diffusePass.getParameters()->setTexcoordTransform(gpu::Framebuffer::evalSubregionTexcoordTransformCoefficients(textureSize, curvatureViewport));
    _diffusePass.getParameters()->setDepthPerspective(args->getViewFrustum().getProjection()[1][1]);
    _diffusePass.getParameters()->setLinearDepthPosFar(args->getViewFrustum().getFarClip());

 
    gpu::doInBatch(args->_context, [=](gpu::Batch& batch) {
        _gpuTimer.begin(batch);
        batch.enableStereo(false);

        batch.setProjectionTransform(glm::mat4());
        batch.setViewTransform(Transform());

        batch.setViewportTransform(curvatureViewport);
        batch.setModelTransform(gpu::Framebuffer::evalSubregionTexcoordTransform(_surfaceGeometryFramebuffer->getSourceFrameSize(), curvatureViewport));

        // Curvature pass
        batch.setUniformBuffer(SurfaceGeometryPass_FrameTransformSlot, frameTransform->getFrameTransformBuffer());
        batch.setUniformBuffer(SurfaceGeometryPass_ParamsSlot, _parametersBuffer);
        batch.setFramebuffer(curvatureFramebuffer);
        // We can avoid the clear by drawing the same clear vallue from the makeCurvature shader. same performances or no worse     
#ifdef USE_STENCIL_TEST
        // Except if stenciling out
        batch.clearColorFramebuffer(gpu::Framebuffer::BUFFER_COLOR0, glm::vec4(0.0));
#endif
        batch.setPipeline(curvaturePipeline);
        batch.setResourceTexture(SurfaceGeometryPass_DepthMapSlot, linearDepthTexture);
        batch.setResourceTexture(SurfaceGeometryPass_NormalMapSlot, normalTexture);
        batch.draw(gpu::TRIANGLE_STRIP, 4);


        batch.setResourceTexture(SurfaceGeometryPass_DepthMapSlot, nullptr);
        batch.setResourceTexture(SurfaceGeometryPass_NormalMapSlot, nullptr);
        batch.setUniformBuffer(SurfaceGeometryPass_ParamsSlot, nullptr);
        batch.setUniformBuffer(SurfaceGeometryPass_FrameTransformSlot, nullptr);

        // Diffusion pass
        const int BlurTask_ParamsSlot = 0;
        const int BlurTask_SourceSlot = 0;
        const int BlurTask_DepthSlot = 1;
        batch.setUniformBuffer(BlurTask_ParamsSlot, _diffusePass.getParameters()->_parametersBuffer);

        batch.setResourceTexture(BlurTask_DepthSlot, linearDepthTexture);

        batch.setFramebuffer(blurringFramebuffer);     
        batch.setPipeline(diffuseVPipeline);
        batch.setResourceTexture(BlurTask_SourceSlot, curvatureTexture);
        batch.draw(gpu::TRIANGLE_STRIP, 4);

        batch.setFramebuffer(curvatureFramebuffer);
        batch.setPipeline(diffuseHPipeline);
        batch.setResourceTexture(BlurTask_SourceSlot, blurringTexture);
        batch.draw(gpu::TRIANGLE_STRIP, 4);

        batch.setFramebuffer(blurringFramebuffer);     
        batch.setPipeline(diffuseVPipeline);
        batch.setResourceTexture(BlurTask_SourceSlot, curvatureTexture);
        batch.draw(gpu::TRIANGLE_STRIP, 4);

        batch.setFramebuffer(lowCurvatureFramebuffer);
        batch.setPipeline(diffuseHPipeline);
        batch.setResourceTexture(BlurTask_SourceSlot, blurringTexture);
        batch.draw(gpu::TRIANGLE_STRIP, 4);

        batch.setResourceTexture(BlurTask_SourceSlot, nullptr);
        batch.setResourceTexture(BlurTask_DepthSlot, nullptr);
        batch.setUniformBuffer(BlurTask_ParamsSlot, nullptr);

        _gpuTimer.end(batch);
    });
       
 
    auto config = std::static_pointer_cast<Config>(renderContext->jobConfig);
    config->gpuTime = _gpuTimer.getAverage();
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

#ifdef USE_STENCIL_TEST
        // Stencil test the curvature pass for objects pixels only, not the background
        state->setStencilTest(true, 0xFF, gpu::State::StencilTest(0, 0xFF, gpu::NOT_EQUAL, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP));
#endif
        // Good to go add the brand new pipeline
        _curvaturePipeline = gpu::Pipeline::create(program, state);
    }

    return _curvaturePipeline;
}
