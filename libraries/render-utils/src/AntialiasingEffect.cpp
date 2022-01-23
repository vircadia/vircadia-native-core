//
//  AntialiasingEffect.cpp
//  libraries/render-utils/src/
//
//  Created by Raffi Bedikian on 8/30/15
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AntialiasingEffect.h"

#include <glm/gtc/random.hpp>

#include <PathUtils.h>
#include <SharedUtil.h>
#include <gpu/Context.h>
#include <shaders/Shaders.h>
#include <graphics/ShaderConstants.h>

#include "render-utils/ShaderConstants.h"
#include "StencilMaskPass.h"
#include "TextureCache.h"
#include "DependencyManager.h"
#include "ViewFrustum.h"
#include "GeometryCache.h"
#include "FramebufferCache.h"
#include "RandomAndNoise.h"

namespace ru {
    using render_utils::slot::texture::Texture;
    using render_utils::slot::buffer::Buffer;
}

namespace gr {
    using graphics::slot::texture::Texture;
    using graphics::slot::buffer::Buffer;
}

#if !ANTIALIASING_USE_TAA

Antialiasing::Antialiasing() {
    _geometryId = DependencyManager::get<GeometryCache>()->allocateID();
}

Antialiasing::~Antialiasing() {
    auto geometryCache = DependencyManager::get<GeometryCache>();
    if (geometryCache) {
        geometryCache->releaseID(_geometryId);
    }
}

const gpu::PipelinePointer& Antialiasing::getAntialiasingPipeline() {
    if (!_antialiasingPipeline) {
        gpu::ShaderPointer program = gpu::Shader::createProgram(shader::render_utils::program::fxaa);
        gpu::StatePointer state = std::make_shared<gpu::State>();

        state->setDepthTest(false, false, gpu::LESS_EQUAL);
        PrepareStencil::testNoAA(*state);

        // Good to go add the brand new pipeline
        _antialiasingPipeline = gpu::Pipeline::create(program, state);
    }

    return _antialiasingPipeline;
}

const gpu::PipelinePointer& Antialiasing::getBlendPipeline() {
    if (!_blendPipeline) {
        gpu::ShaderPointer program = gpu::Shader::createProgram(shader::render_utils::program::fxaa_blend);
        gpu::StatePointer state = std::make_shared<gpu::State>();
        state->setDepthTest(false, false, gpu::LESS_EQUAL);
        PrepareStencil::testNoAA(*state);

        // Good to go add the brand new pipeline
        _blendPipeline = gpu::Pipeline::create(program, state);
    }
    return _blendPipeline;
}

void Antialiasing::run(const render::RenderContextPointer& renderContext, const gpu::FramebufferPointer& sourceBuffer) {
    assert(renderContext->args);
    assert(renderContext->args->hasViewFrustum());

    RenderArgs* args = renderContext->args;

    gpu::doInBatch("Antialiasing::run", args->_context, [&](gpu::Batch& batch) {
        batch.enableStereo(false);
        batch.setViewportTransform(args->_viewport);

        if (!_paramsBuffer) {
            _paramsBuffer = std::make_shared<gpu::Buffer>(sizeof(glm::vec4), nullptr);
        }

        {
            int width = args->_viewport.z; 
            int height = args->_viewport.w;
            if (_antialiasingBuffer && _antialiasingBuffer->getSize() != uvec2(width, height)) {
                _antialiasingBuffer.reset();
            }

            if (!_antialiasingBuffer) {
                // Link the antialiasing FBO to texture
                _antialiasingBuffer = gpu::FramebufferPointer(gpu::Framebuffer::create("antialiasing"));
                auto format = gpu::Element::COLOR_SRGBA_32;
                auto defaultSampler = gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_POINT);
                _antialiasingTexture = gpu::Texture::createRenderBuffer(format, width, height, gpu::Texture::SINGLE_MIP, defaultSampler);
                _antialiasingBuffer->setRenderBuffer(0, _antialiasingTexture);
                glm::vec2 fbExtent { args->_viewport.z, args->_viewport.w };
                glm::vec2 inverseFbExtent = 1.0f / fbExtent;
                _paramsBuffer->setSubData(0, glm::vec4(inverseFbExtent, 0.0, 0.0));
            }
        }


        glm::mat4 projMat;
        Transform viewMat;
        args->getViewFrustum().evalProjectionMatrix(projMat);
        args->getViewFrustum().evalViewTransform(viewMat);
        batch.setProjectionTransform(projMat);
        batch.setViewTransform(viewMat, true);
        batch.setModelTransform(Transform());

        // FXAA step
        auto pipeline = getAntialiasingPipeline();
        batch.setResourceTexture(0, sourceBuffer->getRenderBuffer(0));
        batch.setFramebuffer(_antialiasingBuffer);
        batch.setPipeline(pipeline);
        batch.setUniformBuffer(0, _paramsBuffer);
        batch.draw(gpu::TRIANGLE_STRIP, 4);

        // Blend step
        batch.setResourceTexture(0, _antialiasingTexture);
        batch.setFramebuffer(sourceBuffer);
        batch.setPipeline(getBlendPipeline());
        batch.draw(gpu::TRIANGLE_STRIP, 4);
    });
}
#else

void AntialiasingConfig::setAAMode(int mode) {
    _mode = std::min((int)AntialiasingConfig::MODE_COUNT, std::max(0, mode)); // Just use unsigned?
    emit dirty();
}

Antialiasing::Antialiasing(bool isSharpenEnabled) : 
    _isSharpenEnabled{ isSharpenEnabled } {
}

Antialiasing::~Antialiasing() {
    _antialiasingBuffers.reset();
    _antialiasingTextures[0].reset();
    _antialiasingTextures[1].reset();
}

const gpu::PipelinePointer& Antialiasing::getAntialiasingPipeline(const render::RenderContextPointer& renderContext) {
   
    if (!_antialiasingPipeline) {
        gpu::ShaderPointer program = gpu::Shader::createProgram(shader::render_utils::program::taa);
        gpu::StatePointer state = std::make_shared<gpu::State>();
        
        PrepareStencil::testNoAA(*state);

        // Good to go add the brand new pipeline
        _antialiasingPipeline = gpu::Pipeline::create(program, state);
    }
    
    return _antialiasingPipeline;
}

const gpu::PipelinePointer& Antialiasing::getBlendPipeline() {
    if (!_blendPipeline) {
        gpu::ShaderPointer program = gpu::Shader::createProgram(shader::render_utils::program::fxaa_blend);
        gpu::StatePointer state = std::make_shared<gpu::State>();
        PrepareStencil::testNoAA(*state);
        // Good to go add the brand new pipeline
        _blendPipeline = gpu::Pipeline::create(program, state);
    }
    return _blendPipeline;
}

const gpu::PipelinePointer& Antialiasing::getDebugBlendPipeline() {
    if (!_debugBlendPipeline) {
        gpu::ShaderPointer program = gpu::Shader::createProgram(shader::render_utils::program::taa_blend);
        gpu::StatePointer state = std::make_shared<gpu::State>();
        PrepareStencil::testNoAA(*state);


        // Good to go add the brand new pipeline
        _debugBlendPipeline = gpu::Pipeline::create(program, state);
    }
    return _debugBlendPipeline;
}

void Antialiasing::configure(const Config& config) {
    _mode = (AntialiasingConfig::Mode) config.getAAMode();

    _sharpen = config.sharpen * 0.25f;
    if (!_isSharpenEnabled) {
        _sharpen = 0.0f;
    }
    _params.edit().blend = config.blend * config.blend;
    _params.edit().covarianceGamma = config.covarianceGamma;

    _params.edit().setConstrainColor(config.constrainColor);
    _params.edit().setFeedbackColor(config.feedbackColor);

    _params.edit().debugShowVelocityThreshold = config.debugShowVelocityThreshold;

    _params.edit().regionInfo.x = config.debugX;
    _params.edit().regionInfo.z = config.debugFXAAX;

    _params.edit().setDebug(config.debug);
    _params.edit().setShowDebugCursor(config.showCursorPixel);
    _params.edit().setDebugCursor(config.debugCursorTexcoord);
    _params.edit().setDebugOrbZoom(config.debugOrbZoom);

    _params.edit().setShowClosestFragment(config.showClosestFragment);
}


void Antialiasing::run(const render::RenderContextPointer& renderContext, const Inputs& inputs) {
    assert(renderContext->args);
    assert(renderContext->args->hasViewFrustum());
    
    RenderArgs* args = renderContext->args;

    auto& deferredFrameTransform = inputs.get0();
    auto& sourceBuffer = inputs.get1();
    auto& linearDepthBuffer = inputs.get2();
    auto& velocityBuffer = inputs.get3();
    
    int width = sourceBuffer->getWidth();
    int height = sourceBuffer->getHeight();

    if (_antialiasingBuffers && _antialiasingBuffers->get(0) && _antialiasingBuffers->get(0)->getSize() != uvec2(width, height)) {
        _antialiasingBuffers.reset();
        _antialiasingTextures[0].reset();
        _antialiasingTextures[1].reset();
    }


    if (!_antialiasingBuffers) {
        std::vector<gpu::FramebufferPointer> antiAliasingBuffers;
        // Link the antialiasing FBO to texture
        auto format = sourceBuffer->getRenderBuffer(0)->getTexelFormat();
        auto defaultSampler = gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_LINEAR, gpu::Sampler::WRAP_CLAMP);
        for (int i = 0; i < 2; i++) {
            antiAliasingBuffers.emplace_back(gpu::Framebuffer::create("antialiasing"));
            const auto& antiAliasingBuffer = antiAliasingBuffers.back();
            _antialiasingTextures[i] = gpu::Texture::createRenderBuffer(format, width, height, gpu::Texture::SINGLE_MIP, defaultSampler);
            antiAliasingBuffer->setRenderBuffer(0, _antialiasingTextures[i]);
        }
        _antialiasingBuffers = std::make_shared<gpu::FramebufferSwapChain>(antiAliasingBuffers);
    }
    
    gpu::doInBatch("Antialiasing::run", args->_context, [&](gpu::Batch& batch) {
        batch.enableStereo(false);
        batch.setViewportTransform(args->_viewport);

        // TAA step
        getAntialiasingPipeline(renderContext);
        batch.setResourceFramebufferSwapChainTexture(ru::Texture::TaaHistory, _antialiasingBuffers, 0);
        batch.setResourceTexture(ru::Texture::TaaSource, sourceBuffer->getRenderBuffer(0));
        batch.setResourceTexture(ru::Texture::TaaVelocity, velocityBuffer->getVelocityTexture());
        // This is only used during debug
        batch.setResourceTexture(ru::Texture::TaaDepth, linearDepthBuffer->getLinearDepthTexture());

        batch.setUniformBuffer(ru::Buffer::TaaParams, _params);
        batch.setUniformBuffer(ru::Buffer::DeferredFrameTransform, deferredFrameTransform->getFrameTransformBuffer());
        
        batch.setFramebufferSwapChain(_antialiasingBuffers, 1);
        batch.setPipeline(getAntialiasingPipeline(renderContext));
        batch.draw(gpu::TRIANGLE_STRIP, 4);

        // Blend step
        batch.setResourceTexture(ru::Texture::TaaSource, nullptr);

        batch.setFramebuffer(sourceBuffer);
        if (_params->isDebug()) {
            batch.setPipeline(getDebugBlendPipeline());
            batch.setResourceFramebufferSwapChainTexture(ru::Texture::TaaNext, _antialiasingBuffers, 1);
        }  else {
            batch.setPipeline(getBlendPipeline());
            // Must match the bindg point in the fxaa_blend.slf shader
            batch.setResourceFramebufferSwapChainTexture(0, _antialiasingBuffers, 1);
            // Disable sharpen if FXAA
            if (!_blendParamsBuffer) {
                _blendParamsBuffer = std::make_shared<gpu::Buffer>(sizeof(glm::vec4), nullptr);
            }
            _blendParamsBuffer->setSubData(0, _sharpen * _params.get().regionInfo.z);
            batch.setUniformBuffer(0, _blendParamsBuffer);
        }
        batch.draw(gpu::TRIANGLE_STRIP, 4);
        batch.advance(_antialiasingBuffers);
        
        batch.setUniformBuffer(ru::Buffer::TaaParams, nullptr);
        batch.setUniformBuffer(ru::Buffer::DeferredFrameTransform, nullptr);

        batch.setResourceTexture(ru::Texture::TaaDepth, nullptr);
        batch.setResourceTexture(ru::Texture::TaaHistory, nullptr);
        batch.setResourceTexture(ru::Texture::TaaVelocity, nullptr);
        batch.setResourceTexture(ru::Texture::TaaNext, nullptr);
    });
}

void JitterSampleConfig::setIndex(int current) {
    _index = (current) % JitterSample::SEQUENCE_LENGTH;    
    emit dirty();
}

void JitterSampleConfig::setState(int state) {
    _state = (state) % 3;
    switch (_state) {
    case 0: {
        none();
        break;
    }
    case 1: {
        pause();
        break;
    }
    case 2:
    default: {
        play();
        break;
    }
    }
    emit dirty();
}

int JitterSampleConfig::cycleStopPauseRun() {
    setState((_state + 1) % 3);
    return _state;
}

int JitterSampleConfig::prev() {
    setIndex(_index - 1);
    return _index;
}

int JitterSampleConfig::next() {
    setIndex(_index + 1);
    return _index;
}

int JitterSampleConfig::none() {
    _state = 0;
    stop = true;
    freeze = false;
    setIndex(-1);
    return _state;
}

int JitterSampleConfig::pause() {
    _state = 1;
    stop = false;
    freeze = true;
    setIndex(0);
    return _state;
}


int JitterSampleConfig::play() {
    _state = 2;
    stop = false;
    freeze = false;
    setIndex(0);
    return _state;
}

JitterSample::SampleSequence::SampleSequence(){
    // Halton sequence (2,3)

    for (int i = 0; i < SEQUENCE_LENGTH; i++) {
        offsets[i] = glm::vec2(halton::evaluate<2>(i), halton::evaluate<3>(i));
        offsets[i] -= vec2(0.5f);
    }
    offsets[SEQUENCE_LENGTH] = glm::vec2(0.0f);
}

void JitterSample::configure(const Config& config) {
    _freeze = config.stop || config.freeze;
    if (config.freeze) {
        auto pausedIndex = config.getIndex();
        if (_sampleSequence.currentIndex != pausedIndex) {
            _sampleSequence.currentIndex = pausedIndex;
        }
    } else if (config.stop) {
        _sampleSequence.currentIndex = -1;
    } else {
        _sampleSequence.currentIndex = config.getIndex();
    }
    _scale = config.scale;
}

void JitterSample::run(const render::RenderContextPointer& renderContext, Output& jitter) {
    auto& current = _sampleSequence.currentIndex;
    if (!_freeze) {
        if (current >= 0) {
            current = (current + 1) % SEQUENCE_LENGTH;
        } else {
            current = -1;
        }
    }

    if (current >= 0) {
        jitter = _sampleSequence.offsets[current];
    } else {
        jitter = glm::vec2(0.0f);
    }
}

#endif
