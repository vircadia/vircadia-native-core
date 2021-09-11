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
#include <MathUtils.h>

#include <gpu/Context.h>
#include <shaders/Shaders.h>

#include "StencilMaskPass.h"

#include "render-utils/ShaderConstants.h"

namespace ru {
    using render_utils::slot::texture::Texture;
    using render_utils::slot::buffer::Buffer;
}


LinearDepthFramebuffer::LinearDepthFramebuffer() {
}

void LinearDepthFramebuffer::update(const gpu::TexturePointer& depthBuffer, const gpu::TexturePointer& normalTexture, bool isStereo) {
    //If the depth buffer or size changed, we need to delete our FBOs
    bool reset = false;
    if (_primaryDepthTexture != depthBuffer || _normalTexture != normalTexture) {
        _primaryDepthTexture = depthBuffer;
        _normalTexture = normalTexture;
        reset = true;
    }
    if (_primaryDepthTexture) {
        auto newFrameSize = glm::ivec2(_primaryDepthTexture->getDimensions());
        if (_frameSize != newFrameSize || _isStereo != isStereo) {
            _frameSize = newFrameSize;
            _halfFrameSize = _frameSize;
            if (isStereo) {
                _halfFrameSize.x >>= 1;
            }
            _halfFrameSize >>= 1;
            if (isStereo) {
                _halfFrameSize.x <<= 1;
            }
            _isStereo = isStereo;
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
    const uint16_t LINEAR_DEPTH_MAX_MIP_LEVEL = 5;
    // Point sampling of the depth, as well as the clamp to edge, are needed for the AmbientOcclusionEffect with HBAO
    const auto depthSamplerFull = gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_MIP_POINT, gpu::Sampler::WRAP_CLAMP);
    _linearDepthTexture = gpu::Texture::createRenderBuffer(gpu::Element(gpu::SCALAR, gpu::FLOAT, gpu::RED), width, height, LINEAR_DEPTH_MAX_MIP_LEVEL,
        depthSamplerFull);
    _linearDepthFramebuffer = gpu::FramebufferPointer(gpu::Framebuffer::create("linearDepth"));
    _linearDepthFramebuffer->setRenderBuffer(0, _linearDepthTexture);
    _linearDepthFramebuffer->setDepthStencilBuffer(_primaryDepthTexture, _primaryDepthTexture->getTexelFormat());

    // For Downsampling:
    const uint16_t HALF_LINEAR_DEPTH_MAX_MIP_LEVEL = 5;
    // Point sampling of the depth, as well as the clamp to edge, are needed for the AmbientOcclusionEffect with HBAO
    const auto depthSamplerHalf = gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_MIP_POINT, gpu::Sampler::WRAP_CLAMP);
    // The depth format here is half float as it increases performance in the AmbientOcclusion. But it might be needed elsewhere...
    _halfLinearDepthTexture = gpu::Texture::createRenderBuffer(gpu::Element(gpu::SCALAR, gpu::HALF, gpu::RED), _halfFrameSize.x, _halfFrameSize.y, HALF_LINEAR_DEPTH_MAX_MIP_LEVEL,
        depthSamplerHalf);

    _halfNormalTexture = gpu::Texture::createRenderBuffer(gpu::Element::COLOR_RGBA_32, _halfFrameSize.x, _halfFrameSize.y, gpu::Texture::SINGLE_MIP,
        gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_LINEAR_MIP_POINT));

    _downsampleFramebuffer = gpu::FramebufferPointer(gpu::Framebuffer::create("halfLinearDepth"));
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

gpu::TexturePointer LinearDepthFramebuffer::getNormalTexture() {
    return _normalTexture;
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

void LinearDepthPass::run(const render::RenderContextPointer& renderContext, const Inputs& inputs, Outputs& outputs) {
    assert(renderContext->args);
    assert(renderContext->args->hasViewFrustum());

    RenderArgs* args = renderContext->args;

    const auto frameTransform = inputs.get0();
    const auto deferredFramebuffer = inputs.get1();

    if (!_gpuTimer) {
        _gpuTimer = std::make_shared < gpu::RangeTimer>(__FUNCTION__);
    }

    if (!_linearDepthFramebuffer) {
        _linearDepthFramebuffer = std::make_shared<LinearDepthFramebuffer>();
    }

    auto depthBuffer = deferredFramebuffer->getPrimaryDepthTexture();
    auto normalTexture = deferredFramebuffer->getDeferredNormalTexture();

    _linearDepthFramebuffer->update(depthBuffer, normalTexture, args->isStereo());

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
   
    auto linearDepthPipeline = getLinearDepthPipeline(renderContext);
    auto downsamplePipeline = getDownsamplePipeline(renderContext);

    auto depthViewport = args->_viewport;
    auto halfViewport = depthViewport >> 1;
    float clearLinearDepth = args->getViewFrustum().getFarClip() * 2.0f;

    gpu::doInBatch("LinearDepthPass::run", args->_context, [=](gpu::Batch& batch) {
        PROFILE_RANGE_BATCH(batch, "LinearDepthPass");
        _gpuTimer->begin(batch);

        batch.enableStereo(false);

        batch.setProjectionTransform(glm::mat4());
        batch.resetViewTransform();

        batch.setUniformBuffer(ru::Buffer::DeferredFrameTransform, frameTransform->getFrameTransformBuffer());

        // LinearDepth
        batch.setViewportTransform(depthViewport);
        batch.setFramebuffer(linearDepthFBO);
        batch.clearColorFramebuffer(gpu::Framebuffer::BUFFER_COLOR0, glm::vec4(clearLinearDepth, 0.0f, 0.0f, 0.0f));
        batch.setPipeline(linearDepthPipeline);
        batch.setModelTransform(gpu::Framebuffer::evalSubregionTexcoordTransform(_linearDepthFramebuffer->getDepthFrameSize(), depthViewport));
        batch.setResourceTexture(ru::Texture::SurfaceGeometryDepth, depthBuffer);
        batch.draw(gpu::TRIANGLE_STRIP, 4);

        // Downsample
        batch.setViewportTransform(halfViewport);
        batch.setFramebuffer(downsampleFBO);
        batch.setResourceTexture(ru::Texture::SurfaceGeometryDepth, linearDepthTexture);
        batch.setResourceTexture(ru::Texture::SurfaceGeometryNormal, normalTexture);
        batch.setPipeline(downsamplePipeline);
        batch.setModelTransform(gpu::Framebuffer::evalSubregionTexcoordTransform(_linearDepthFramebuffer->getDepthFrameSize() >> 1, halfViewport));
        batch.draw(gpu::TRIANGLE_STRIP, 4);

        _gpuTimer->end(batch);
    });

    auto config = std::static_pointer_cast<Config>(renderContext->jobConfig);
    config->setGPUBatchRunTime(_gpuTimer->getGPUAverage(), _gpuTimer->getBatchAverage());
}


const gpu::PipelinePointer& LinearDepthPass::getLinearDepthPipeline(const render::RenderContextPointer& renderContext) {
    gpu::ShaderPointer program;
    if (!_linearDepthPipeline) {
        program = gpu::Shader::createProgram(shader::render_utils::program::surfaceGeometry_makeLinearDepth);

        gpu::StatePointer state = std::make_shared<gpu::State>();

        // Stencil test the curvature pass for objects pixels only, not the background
        PrepareStencil::testShape(*state);

        state->setColorWriteMask(true, false, false, false);

        // Good to go add the brand new pipeline
        _linearDepthPipeline = gpu::Pipeline::create(program, state);
    }


    return _linearDepthPipeline;
}


const gpu::PipelinePointer& LinearDepthPass::getDownsamplePipeline(const render::RenderContextPointer& renderContext) {
    if (!_downsamplePipeline) {
        gpu::ShaderPointer program = gpu::Shader::createProgram(shader::render_utils::program::surfaceGeometry_downsampleDepthNormal);

        gpu::StatePointer state = std::make_shared<gpu::State>();
        PrepareStencil::testShape(*state);

        state->setColorWriteMask(true, true, true, false);

        // Good to go add the brand new pipeline
        _downsamplePipeline = gpu::Pipeline::create(program, state);
    }

    return _downsamplePipeline;
}


//#define USE_STENCIL_TEST

SurfaceGeometryFramebuffer::SurfaceGeometryFramebuffer() {
}

void SurfaceGeometryFramebuffer::update(const gpu::TexturePointer& linearDepthBuffer) {
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

    _curvatureTexture = gpu::Texture::createRenderBuffer(gpu::Element::COLOR_RGBA_32, width, height, gpu::Texture::SINGLE_MIP, gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_LINEAR_MIP_POINT));
    _curvatureFramebuffer = gpu::FramebufferPointer(gpu::Framebuffer::create("surfaceGeometry::curvature"));
    _curvatureFramebuffer->setRenderBuffer(0, _curvatureTexture);

    _lowCurvatureTexture = gpu::Texture::createRenderBuffer(gpu::Element::COLOR_RGBA_32, width, height, gpu::Texture::SINGLE_MIP, gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_LINEAR_MIP_POINT));
    _lowCurvatureFramebuffer = gpu::FramebufferPointer(gpu::Framebuffer::create("surfaceGeometry::lowCurvature"));
    _lowCurvatureFramebuffer->setRenderBuffer(0, _lowCurvatureTexture);

    _blurringTexture = gpu::Texture::createRenderBuffer(gpu::Element::COLOR_RGBA_32, width, height, gpu::Texture::SINGLE_MIP, gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_LINEAR_MIP_POINT));
    _blurringFramebuffer = gpu::FramebufferPointer(gpu::Framebuffer::create("surfaceGeometry::blurring"));
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


void SurfaceGeometryPass::run(const render::RenderContextPointer& renderContext, const Inputs& inputs, Outputs& outputs) {
    assert(renderContext->args);
    assert(renderContext->args->hasViewFrustum());

    RenderArgs* args = renderContext->args;

    if (!_gpuTimer) {
        _gpuTimer = std::make_shared < gpu::RangeTimer>(__FUNCTION__);
    }

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
    _surfaceGeometryFramebuffer->update(linearDepthTexture);

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

    auto curvaturePipeline = getCurvaturePipeline(renderContext);
    auto diffuseVPipeline = _diffusePass.getBlurVPipeline();
    auto diffuseHPipeline = _diffusePass.getBlurHPipeline();

    _diffusePass.getParameters()->setWidthHeight(curvatureViewport.z, curvatureViewport.w, args->isStereo());
    glm::ivec2 textureSize(curvatureTexture->getDimensions());
    _diffusePass.getParameters()->setTexcoordTransform(gpu::Framebuffer::evalSubregionTexcoordTransformCoefficients(textureSize, curvatureViewport));
    _diffusePass.getParameters()->setDepthPerspective(args->getViewFrustum().getProjection()[1][1]);
    _diffusePass.getParameters()->setLinearDepthPosFar(args->getViewFrustum().getFarClip());

 
    gpu::doInBatch("SurfaceGeometryPass::run", args->_context, [=](gpu::Batch& batch) {
        _gpuTimer->begin(batch);
        batch.enableStereo(false);

        batch.setProjectionTransform(glm::mat4());
        batch.resetViewTransform();

        batch.setViewportTransform(curvatureViewport);
        batch.setModelTransform(gpu::Framebuffer::evalSubregionTexcoordTransform(_surfaceGeometryFramebuffer->getSourceFrameSize(), curvatureViewport));

        // Curvature pass
        batch.setUniformBuffer(ru::Buffer::DeferredFrameTransform, frameTransform->getFrameTransformBuffer());
        batch.setUniformBuffer(ru::Buffer::SurfaceGeometryParams, _parametersBuffer);
        batch.setFramebuffer(curvatureFramebuffer);
        // We can avoid the clear by drawing the same clear vallue from the makeCurvature shader. same performances or no worse     
#ifdef USE_STENCIL_TEST
        // Except if stenciling out
        batch.clearColorFramebuffer(gpu::Framebuffer::BUFFER_COLOR0, glm::vec4(0.0));
#endif
        batch.setPipeline(curvaturePipeline);
        batch.setResourceTexture(ru::Texture::SurfaceGeometryDepth, linearDepthTexture);
        batch.setResourceTexture(ru::Texture::SurfaceGeometryNormal, normalTexture);
        batch.draw(gpu::TRIANGLE_STRIP, 4);


        batch.setResourceTexture(ru::Texture::SurfaceGeometryDepth, nullptr);
        batch.setResourceTexture(ru::Texture::SurfaceGeometryNormal, nullptr);
        batch.setUniformBuffer(ru::Buffer::SurfaceGeometryParams, nullptr);
        batch.setUniformBuffer(ru::Buffer::DeferredFrameTransform, nullptr);

        // Diffusion pass
        batch.setUniformBuffer(ru::Buffer::BlurParams, _diffusePass.getParameters()->_parametersBuffer);

        batch.setResourceTexture(ru::Texture::BlurDepth, linearDepthTexture);

        batch.setFramebuffer(blurringFramebuffer);     
        batch.setPipeline(diffuseVPipeline);
        batch.setResourceTexture(ru::Texture::BlurSource, curvatureTexture);
        batch.draw(gpu::TRIANGLE_STRIP, 4);

        batch.setFramebuffer(curvatureFramebuffer);
        batch.setPipeline(diffuseHPipeline);
        batch.setResourceTexture(ru::Texture::BlurSource, blurringTexture);
        batch.draw(gpu::TRIANGLE_STRIP, 4);

        batch.setFramebuffer(blurringFramebuffer);     
        batch.setPipeline(diffuseVPipeline);
        batch.setResourceTexture(ru::Texture::BlurSource, curvatureTexture);
        batch.draw(gpu::TRIANGLE_STRIP, 4);

        batch.setFramebuffer(lowCurvatureFramebuffer);
        batch.setPipeline(diffuseHPipeline);
        batch.setResourceTexture(ru::Texture::BlurSource, blurringTexture);
        batch.draw(gpu::TRIANGLE_STRIP, 4);

        batch.setResourceTexture(ru::Texture::BlurSource, nullptr);
        batch.setResourceTexture(ru::Texture::BlurDepth, nullptr);
        batch.setUniformBuffer(ru::Buffer::BlurParams, nullptr);

        _gpuTimer->end(batch);
    });
       
 
    auto config = std::static_pointer_cast<Config>(renderContext->jobConfig);
    config->setGPUBatchRunTime(_gpuTimer->getGPUAverage(), _gpuTimer->getBatchAverage());
}

const gpu::PipelinePointer& SurfaceGeometryPass::getCurvaturePipeline(const render::RenderContextPointer& renderContext) {
    if (!_curvaturePipeline) {
        gpu::ShaderPointer program = gpu::Shader::createProgram(shader::render_utils::program::surfaceGeometry_makeCurvature);

        gpu::StatePointer state = std::make_shared<gpu::State>();

#ifdef USE_STENCIL_TEST
        // Stencil test the curvature pass for objects pixels only, not the background
        PrepareStencil::testShape(*state);
#endif
        // Good to go add the brand new pipeline
        _curvaturePipeline = gpu::Pipeline::create(program, state);
    }

    return _curvaturePipeline;
}
