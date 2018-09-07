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


namespace ru {
    using render_utils::slot::uniform::Uniform;
    using render_utils::slot::texture::Texture;
    using render_utils::slot::buffer::Buffer;
}

namespace gr {
    using graphics::slot::texture::Texture;
    using graphics::slot::buffer::Buffer;
}

#define ANTIALIASING_USE_TAA    1

#if !ANTIALIASING_USE_TAA
#include "fxaa_vert.h"
#include "fxaa_frag.h"
#include "fxaa_blend_frag.h"


Antialiasing::Antialiasing() {
    _geometryId = DependencyManager::get<GeometryCache>()->allocateID();
}

Antialiasing::~Antialiasing() {
    auto geometryCache = DependencyManager::get<GeometryCache>();
    if (geometryCache) {
        geometryCache->releaseID(_geometryId);
    }
}

const gpu::PipelinePointer& Antialiasing::getAntialiasingPipeline(RenderArgs* args) {
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
    }

    if (!_antialiasingPipeline) {
        auto vs = fxaa_vert::getShader();
        auto ps = fxaa_frag::getShader();
        gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);

        _texcoordOffsetLoc = program->getUniforms().findLocation("texcoordOffset");

        gpu::StatePointer state = gpu::StatePointer(new gpu::State());

        state->setDepthTest(false, false, gpu::LESS_EQUAL);
        PrepareStencil::testNoAA(*state);

        // Good to go add the brand new pipeline
        _antialiasingPipeline = gpu::Pipeline::create(program, state);
    }

    return _antialiasingPipeline;
}

const gpu::PipelinePointer& Antialiasing::getBlendPipeline() {
    if (!_blendPipeline) {
        auto vs = fxaa_vert::getShader();
        auto ps = fxaa_blend_frag::getShader();
        gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);
        gpu::StatePointer state = gpu::StatePointer(new gpu::State());
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

        // FIXME: NEED to simplify that code to avoid all the GeometryCahce call, this is purely pixel manipulation
        float fbWidth = renderContext->args->_viewport.z;
        float fbHeight = renderContext->args->_viewport.w;
        // float sMin = args->_viewport.x / fbWidth;
        // float sWidth = args->_viewport.z / fbWidth;
        // float tMin = args->_viewport.y / fbHeight;
        // float tHeight = args->_viewport.w / fbHeight;

        glm::mat4 projMat;
        Transform viewMat;
        args->getViewFrustum().evalProjectionMatrix(projMat);
        args->getViewFrustum().evalViewTransform(viewMat);
        batch.setProjectionTransform(projMat);
        batch.setViewTransform(viewMat, true);
        batch.setModelTransform(Transform());

        // FXAA step
        auto pipeline = getAntialiasingPipeline(renderContext->args);
        batch.setResourceTexture(0, sourceBuffer->getRenderBuffer(0));
        batch.setFramebuffer(_antialiasingBuffer);
        batch.setPipeline(pipeline);

        // initialize the view-space unpacking uniforms using frustum data
        float left, right, bottom, top, nearVal, farVal;
        glm::vec4 nearClipPlane, farClipPlane;

        args->getViewFrustum().computeOffAxisFrustum(left, right, bottom, top, nearVal, farVal, nearClipPlane, farClipPlane);

        // float depthScale = (farVal - nearVal) / farVal;
        // float nearScale = -1.0f / nearVal;
        // float depthTexCoordScaleS = (right - left) * nearScale / sWidth;
        // float depthTexCoordScaleT = (top - bottom) * nearScale / tHeight;
        // float depthTexCoordOffsetS = left * nearScale - sMin * depthTexCoordScaleS;
        // float depthTexCoordOffsetT = bottom * nearScale - tMin * depthTexCoordScaleT;

        batch._glUniform2f(_texcoordOffsetLoc, 1.0f / fbWidth, 1.0f / fbHeight);

        glm::vec4 color(0.0f, 0.0f, 0.0f, 1.0f);
        glm::vec2 bottomLeft(-1.0f, -1.0f);
        glm::vec2 topRight(1.0f, 1.0f);
        glm::vec2 texCoordTopLeft(0.0f, 0.0f);
        glm::vec2 texCoordBottomRight(1.0f, 1.0f);
        DependencyManager::get<GeometryCache>()->renderQuad(batch, bottomLeft, topRight, texCoordTopLeft, texCoordBottomRight, color, _geometryId);


        // Blend step
        batch.setResourceTexture(0, _antialiasingTexture);
        batch.setFramebuffer(sourceBuffer);
        batch.setPipeline(getBlendPipeline());

        DependencyManager::get<GeometryCache>()->renderQuad(batch, bottomLeft, topRight, texCoordTopLeft, texCoordBottomRight, color, _geometryId);
    });
}
#else

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
        gpu::StatePointer state = gpu::StatePointer(new gpu::State());
        
        PrepareStencil::testNoAA(*state);

        // Good to go add the brand new pipeline
        _antialiasingPipeline = gpu::Pipeline::create(program, state);
    }
    
    return _antialiasingPipeline;
}

const gpu::PipelinePointer& Antialiasing::getBlendPipeline() {
    if (!_blendPipeline) {
        gpu::ShaderPointer program = gpu::Shader::createProgram(shader::render_utils::program::fxaa_blend);
        gpu::StatePointer state = gpu::StatePointer(new gpu::State());
        PrepareStencil::testNoAA(*state);
        // Good to go add the brand new pipeline
        _blendPipeline = gpu::Pipeline::create(program, state);
    }
    return _blendPipeline;
}

const gpu::PipelinePointer& Antialiasing::getDebugBlendPipeline() {
    if (!_debugBlendPipeline) {
        gpu::ShaderPointer program = gpu::Shader::createProgram(shader::render_utils::program::taa_blend);
        gpu::StatePointer state = gpu::StatePointer(new gpu::State());
        PrepareStencil::testNoAA(*state);


        // Good to go add the brand new pipeline
        _debugBlendPipeline = gpu::Pipeline::create(program, state);
    }
    return _debugBlendPipeline;
}

void Antialiasing::configure(const Config& config) {
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
            batch._glUniform1f(ru::Uniform::TaaSharpenIntensity, _sharpen * _params.get().regionInfo.z);
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

int JitterSampleConfig::cycleStopPauseRun() {
    _state = (_state + 1) % 3;
    switch (_state) {
        case 0: {
            return none();
            break;
        }
        case 1: {
            return pause();
            break;
        }
        case 2:
        default: {
            return play();
            break;
        }
    }
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

template <int B> 
class Halton {
public:

    float eval(int index) const {
        float f = 1.0f;
        float r = 0.0f;
        float invB = 1.0f / (float)B;
        index++; // Indices start at 1, not 0

        while (index > 0) {
            f = f * invB;
            r = r + f * (float)(index % B);
            index = index / B;

        }

        return r;
    }

};


JitterSample::SampleSequence::SampleSequence(){
    // Halton sequence (2,3)
    Halton<2> genX;
    Halton<3> genY;

    for (int i = 0; i < SEQUENCE_LENGTH; i++) {
        offsets[i] = glm::vec2(genX.eval(i), genY.eval(i));
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

    jitter.x = 0.0f;
    jitter.y = 0.0f;
    if (current >= 0) {
        jitter = _sampleSequence.offsets[current];
    }
}

#endif
