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


#include <glm/gtc/random.hpp>

#include <PathUtils.h>
#include <SharedUtil.h>
#include <gpu/Context.h>
#include <gpu/StandardShaderLib.h>

#include "AntialiasingEffect.h"
#include "StencilMaskPass.h"
#include "TextureCache.h"
#include "DependencyManager.h"
#include "ViewFrustum.h"
#include "GeometryCache.h"
#include "FramebufferCache.h"
/*
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
        auto vs = gpu::Shader::createVertex(std::string(fxaa_vert));
        auto ps = gpu::Shader::createPixel(std::string(fxaa_frag));
        gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);

        gpu::Shader::BindingSet slotBindings;
        slotBindings.insert(gpu::Shader::Binding(std::string("colorTexture"), 0));

        gpu::Shader::makeProgram(*program, slotBindings);

        _texcoordOffsetLoc = program->getUniforms().findLocation("texcoordOffset");

        gpu::StatePointer state = gpu::StatePointer(new gpu::State());

        PrepareStencil::testMask(*state);

        state->setDepthTest(false, false, gpu::LESS_EQUAL);

        // Good to go add the brand new pipeline
        _antialiasingPipeline = gpu::Pipeline::create(program, state);
    }

    return _antialiasingPipeline;
}

const gpu::PipelinePointer& Antialiasing::getBlendPipeline() {
    if (!_blendPipeline) {
        auto vs = gpu::Shader::createVertex(std::string(fxaa_vert));
        auto ps = gpu::Shader::createPixel(std::string(fxaa_blend_frag));
        gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);

        gpu::Shader::BindingSet slotBindings;
        slotBindings.insert(gpu::Shader::Binding(std::string("colorTexture"), 0));

        gpu::Shader::makeProgram(*program, slotBindings);

        gpu::StatePointer state = gpu::StatePointer(new gpu::State());

        state->setDepthTest(false, false, gpu::LESS_EQUAL);
        PrepareStencil::testMask(*state);

        // Good to go add the brand new pipeline
        _blendPipeline = gpu::Pipeline::create(program, state);
    }
    return _blendPipeline;
}

void Antialiasing::run(const render::RenderContextPointer& renderContext, const gpu::FramebufferPointer& sourceBuffer) {
    assert(renderContext->args);
    assert(renderContext->args->hasViewFrustum());

    RenderArgs* args = renderContext->args;

    gpu::doInBatch(args->_context, [&](gpu::Batch& batch) {
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
        getBlendPipeline();
        batch.setResourceTexture(0, _antialiasingTexture);
        batch.setFramebuffer(sourceBuffer);
        batch.setPipeline(getBlendPipeline());

        DependencyManager::get<GeometryCache>()->renderQuad(batch, bottomLeft, topRight, texCoordTopLeft, texCoordBottomRight, color, _geometryId);
    });
}
*/

#include "fxaa_frag.h"
#include "fxaa_blend_frag.h"


Antialiasing::Antialiasing() {
}

Antialiasing::~Antialiasing() {
}

const gpu::PipelinePointer& Antialiasing::getAntialiasingPipeline() {
   
    if (!_antialiasingPipeline) {
        
        auto vs = gpu::StandardShaderLib::getDrawUnitQuadTexcoordVS();
        auto ps = gpu::Shader::createPixel(std::string(fxaa_frag));
        gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);
        
        gpu::Shader::BindingSet slotBindings;
       // slotBindings.insert(gpu::Shader::Binding(std::string("historyTexture"), 0));
        slotBindings.insert(gpu::Shader::Binding(std::string("colorTexture"), 0));
        
        gpu::Shader::makeProgram(*program, slotBindings);
        
        gpu::StatePointer state = gpu::StatePointer(new gpu::State());
        
        PrepareStencil::testMask(*state);
        state->setBlendFunction(true, gpu::State::BlendArg::SRC_ALPHA, gpu::State::BlendOp::BLEND_OP_ADD, gpu::State::BlendArg::INV_SRC_ALPHA);

        // Good to go add the brand new pipeline
        _antialiasingPipeline = gpu::Pipeline::create(program, state);
    }
    
    return _antialiasingPipeline;
}

const gpu::PipelinePointer& Antialiasing::getBlendPipeline() {
    if (!_blendPipeline) {
        auto vs = gpu::StandardShaderLib::getDrawUnitQuadTexcoordVS();
        auto ps = gpu::Shader::createPixel(std::string(fxaa_blend_frag));
        gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);
        
        gpu::Shader::BindingSet slotBindings;
        slotBindings.insert(gpu::Shader::Binding(std::string("colorTexture"), 0));
        
        gpu::Shader::makeProgram(*program, slotBindings);
        
        gpu::StatePointer state = gpu::StatePointer(new gpu::State());
        PrepareStencil::testMask(*state);

    
        // Good to go add the brand new pipeline
        _blendPipeline = gpu::Pipeline::create(program, state);
    }
    return _blendPipeline;
}

void Antialiasing::configure(const Config& config) {
    _params.edit().debugX = config.debugX;
    _params.edit().blend = config.blend;
}


void Antialiasing::run(const render::RenderContextPointer& renderContext, const gpu::FramebufferPointer& sourceBuffer) {
    assert(renderContext->args);
    assert(renderContext->args->hasViewFrustum());
    
    RenderArgs* args = renderContext->args;
    
    int width = sourceBuffer->getWidth();
    int height = sourceBuffer->getHeight();

    if (_antialiasingBuffer) {
        if (_antialiasingBuffer->getSize() != uvec2(width, height)) {// || (sourceBuffer && (_antialiasingBuffer->getRenderBuffer(1) != sourceBuffer->getRenderBuffer(0)))) {
            _antialiasingBuffer.reset();
        }
    }

    if (!_antialiasingBuffer) {
        // Link the antialiasing FBO to texture
        _antialiasingBuffer = gpu::FramebufferPointer(gpu::Framebuffer::create("antialiasing"));
        auto format = gpu::Element::COLOR_SRGBA_32; // DependencyManager::get<FramebufferCache>()->getLightingTexture()->getTexelFormat();
        auto defaultSampler = gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_POINT);
        _antialiasingTexture = gpu::Texture::createRenderBuffer(format, width, height, gpu::Texture::SINGLE_MIP, defaultSampler);
        _antialiasingBuffer->setRenderBuffer(0, _antialiasingTexture);
    }

    gpu::doInBatch(args->_context, [&](gpu::Batch& batch) {
        batch.enableStereo(false);
        batch.setViewportTransform(args->_viewport);

        // TAA step
        getAntialiasingPipeline();
        batch.setResourceTexture(0, sourceBuffer->getRenderBuffer(0));
        batch.setFramebuffer(_antialiasingBuffer);
        batch.setPipeline(getAntialiasingPipeline());
        
        batch.setUniformBuffer(0, _params._buffer);

        batch.draw(gpu::TRIANGLE_STRIP, 4);

        // Blend step
        batch.setResourceTexture(0, _antialiasingTexture);
        batch.setFramebuffer(sourceBuffer);
        batch.setPipeline(getBlendPipeline());
        batch.draw(gpu::TRIANGLE_STRIP, 4);
    });
}

JitterSample::SampleSequence::SampleSequence(){
    // Halton sequence (2,3)
    offsets[0] = {  1.0f / 2.0f,  1.0f / 3.0f };
    offsets[1] = {  1.0f / 4.0f,  2.0f / 3.0f };
    offsets[2] = {  3.0f / 4.0f,  1.0f / 9.0f };
    offsets[3] = {  1.0f / 8.0f,  4.0f / 9.0f };
    offsets[4] = {  5.0f / 8.0f,  7.0f / 9.0f };
    offsets[5] = {  3.0f / 8.0f,  2.0f / 9.0f };
    offsets[6] = {  7.0f / 8.0f,  5.0f / 9.0f };
    offsets[7] = {  1.0f / 16.0f, 8.0f / 9.0f };

    for (int i = 0; i < SEQUENCE_LENGTH; i++) {
        offsets[i] = offsets[i] - vec2(0.5f);
    }
}

void JitterSample::configure(const Config& config) {
    _freeze = config.freeze;
    _scale = config.scale;
}
void JitterSample::run(const render::RenderContextPointer& renderContext, JitterBuffer& jitterBuffer) {
    auto& current = _jitterBuffer.edit().currentIndex;
    if (!_freeze) {
        current = (current + 1) % SampleSequence::SEQUENCE_LENGTH;
    }
    auto viewFrustum = renderContext->args->getViewFrustum();
    auto projMat = viewFrustum.getProjection();
    auto theNear = viewFrustum.getNearClip();
    
    auto jit = _scale * jitterBuffer.get().offsets[current];
    auto width = (float) renderContext->args->_viewport.z;
    auto height = (float) renderContext->args->_viewport.w;

    auto jx = -4.0 * jit.x / width;
    auto jy = -4.0 * jit.y / height;
    
    projMat[2][0] += jx;
    projMat[2][1] += jy;
    
    viewFrustum.setProjection(projMat);
    
    renderContext->args->pushViewFrustum(viewFrustum);
    
    jitterBuffer = _jitterBuffer;
}


