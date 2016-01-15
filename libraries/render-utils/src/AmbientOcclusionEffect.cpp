//
//  AmbientOcclusionEffect.cpp
//  libraries/render-utils/src/
//
//  Created by Niraj Venkat on 7/15/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#include <glm/gtc/random.hpp>

#include <algorithm> //min max and more


#include <PathUtils.h>
#include <SharedUtil.h>
#include <gpu/Context.h>
#include <gpu/StandardShaderLib.h>

#include "AmbientOcclusionEffect.h"
#include "TextureCache.h"
#include "FramebufferCache.h"
#include "DependencyManager.h"
#include "ViewFrustum.h"

#include "ssao_makePyramid_frag.h"
#include "ssao_makeOcclusion_frag.h"
#include "ssao_makeHorizontalBlur_frag.h"
#include "ssao_makeVerticalBlur_frag.h"

const int AmbientOcclusionEffect_ParamsSlot = 0;
const int AmbientOcclusionEffect_DepthMapSlot = 0;
const int AmbientOcclusionEffect_PyramidMapSlot = 0;
const int AmbientOcclusionEffect_OcclusionMapSlot = 0;

AmbientOcclusionEffect::AmbientOcclusionEffect() {
    Parameters parameters;
    _parametersBuffer = gpu::BufferView(std::make_shared<gpu::Buffer>(sizeof(Parameters), (const gpu::Byte*) &parameters));
}

const gpu::PipelinePointer& AmbientOcclusionEffect::getPyramidPipeline() {
    if (!_pyramidPipeline) {
        auto vs = gpu::StandardShaderLib::getDrawViewportQuadTransformTexcoordVS();
        auto ps = gpu::Shader::createPixel(std::string(ssao_makePyramid_frag));
        gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);

        gpu::Shader::BindingSet slotBindings;
        slotBindings.insert(gpu::Shader::Binding(std::string("ambientOcclusionParamsBuffer"), AmbientOcclusionEffect_ParamsSlot));
        slotBindings.insert(gpu::Shader::Binding(std::string("depthMap"), AmbientOcclusionEffect_DepthMapSlot));
        gpu::Shader::makeProgram(*program, slotBindings);


        gpu::StatePointer state = gpu::StatePointer(new gpu::State());

        // Stencil test all the ao passes for objects pixels only, not the background
        state->setStencilTest(true, 0xFF, gpu::State::StencilTest(0, 0xFF, gpu::NOT_EQUAL, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP));

        state->setColorWriteMask(true, false, false, false);

        // Good to go add the brand new pipeline
        _pyramidPipeline = gpu::Pipeline::create(program, state);
    }
    return _pyramidPipeline;
}

const gpu::PipelinePointer& AmbientOcclusionEffect::getOcclusionPipeline() {
    if (!_occlusionPipeline) {
        auto vs = gpu::StandardShaderLib::getDrawViewportQuadTransformTexcoordVS();
        auto ps = gpu::Shader::createPixel(std::string(ssao_makeOcclusion_frag));
        gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);

        gpu::Shader::BindingSet slotBindings;
        slotBindings.insert(gpu::Shader::Binding(std::string("ambientOcclusionParamsBuffer"), AmbientOcclusionEffect_ParamsSlot));
        slotBindings.insert(gpu::Shader::Binding(std::string("pyramidMap"), AmbientOcclusionEffect_PyramidMapSlot));
        gpu::Shader::makeProgram(*program, slotBindings);

        gpu::StatePointer state = gpu::StatePointer(new gpu::State());

        // Stencil test all the ao passes for objects pixels only, not the background
        state->setStencilTest(true, 0xFF, gpu::State::StencilTest(0, 0xFF, gpu::NOT_EQUAL, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP));

        state->setColorWriteMask(true, true, true, false);
        //state->setColorWriteMask(true, true, true, true);
        
        // Good to go add the brand new pipeline
        _occlusionPipeline = gpu::Pipeline::create(program, state);
    }
    return _occlusionPipeline;
}


const gpu::PipelinePointer& AmbientOcclusionEffect::getHBlurPipeline() {
    if (!_hBlurPipeline) {
        auto vs = gpu::StandardShaderLib::getDrawViewportQuadTransformTexcoordVS();
        auto ps = gpu::Shader::createPixel(std::string(ssao_makeHorizontalBlur_frag));
        gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);
        
        gpu::Shader::BindingSet slotBindings;
        slotBindings.insert(gpu::Shader::Binding(std::string("ambientOcclusionParamsBuffer"), AmbientOcclusionEffect_ParamsSlot));
        slotBindings.insert(gpu::Shader::Binding(std::string("occlusionMap"), AmbientOcclusionEffect_OcclusionMapSlot));
        gpu::Shader::makeProgram(*program, slotBindings);
        
        gpu::StatePointer state = gpu::StatePointer(new gpu::State());
        
        // Stencil test all the ao passes for objects pixels only, not the background
        state->setStencilTest(true, 0xFF, gpu::State::StencilTest(0, 0xFF, gpu::NOT_EQUAL, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP));
        
        state->setColorWriteMask(true, true, true, false);
        
        // Good to go add the brand new pipeline
        _hBlurPipeline = gpu::Pipeline::create(program, state);
    }
    return _hBlurPipeline;
}

const gpu::PipelinePointer& AmbientOcclusionEffect::getVBlurPipeline() {
    if (!_vBlurPipeline) {
        auto vs = gpu::StandardShaderLib::getDrawViewportQuadTransformTexcoordVS();
        auto ps = gpu::Shader::createPixel(std::string(ssao_makeVerticalBlur_frag));
        gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);
        
        gpu::Shader::BindingSet slotBindings;
        slotBindings.insert(gpu::Shader::Binding(std::string("ambientOcclusionParamsBuffer"), AmbientOcclusionEffect_ParamsSlot));
        slotBindings.insert(gpu::Shader::Binding(std::string("occlusionMap"), AmbientOcclusionEffect_OcclusionMapSlot));
        
        gpu::Shader::makeProgram(*program, slotBindings);
        
        gpu::StatePointer state = gpu::StatePointer(new gpu::State());
        
        // Stencil test all the ao passes for objects pixels only, not the background
        state->setStencilTest(true, 0xFF, gpu::State::StencilTest(0, 0xFF, gpu::NOT_EQUAL, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP));
        
        // Vertical blur write just the final result Occlusion value in the alpha channel
        state->setColorWriteMask(false, false, false, true);
        
        // Good to go add the brand new pipeline
        _vBlurPipeline = gpu::Pipeline::create(program, state);
    }
    return _vBlurPipeline;
}


void AmbientOcclusionEffect::setDepthInfo(float nearZ, float farZ) {
    _parametersBuffer.edit<Parameters>()._depthInfo = glm::vec4(nearZ*farZ, farZ -nearZ, -farZ, 0.0f);

}

void AmbientOcclusionEffect::setRadius(float radius) {
    radius = std::max(0.01f, radius);
    if (radius != getRadius()) {
        auto& current = _parametersBuffer.edit<Parameters>()._radiusInfo;
        current.x = radius;
        current.y = radius * radius;
        current.z = 1.0f / pow(radius, 6.0f);
    }
}

void AmbientOcclusionEffect::setLevel(float level) {
    level = std::max(0.01f, level);
    if (level != getLevel()) {
        auto& current = _parametersBuffer.edit<Parameters>()._radiusInfo;
        current.w = level;
    }
}

void AmbientOcclusionEffect::setDithering(bool enabled) {
    if (enabled != isDitheringEnabled()) {
        auto& current = _parametersBuffer.edit<Parameters>()._ditheringInfo;
        current.x = (float)enabled;
    }
}

void AmbientOcclusionEffect::setNumSamples(int numSamples) {
    numSamples = std::max(1.f, (float) numSamples);
    if (numSamples != getNumSamples()) {
        auto& current = _parametersBuffer.edit<Parameters>()._sampleInfo;
        current.x = numSamples;
        current.y = 1.0 / numSamples;
    }
}

void AmbientOcclusionEffect::setNumSpiralTurns(float numTurns) {
    numTurns = std::max(0.f, (float)numTurns);
    if (numTurns != getNumSpiralTurns()) {
        auto& current = _parametersBuffer.edit<Parameters>()._sampleInfo;
        current.z = numTurns;
    }
}

void AmbientOcclusionEffect::setEdgeSharpness(float sharpness) {
    sharpness = std::max(0.f, (float)sharpness);
    if (sharpness != getEdgeSharpness()) {
        auto& current = _parametersBuffer.edit<Parameters>()._blurInfo;
        current.x = sharpness;
    }
}

void AmbientOcclusionEffect::run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext) {
    assert(renderContext->getArgs());
    assert(renderContext->getArgs()->_viewFrustum);

    RenderArgs* args = renderContext->getArgs();

    auto framebufferCache = DependencyManager::get<FramebufferCache>();
    auto depthBuffer = framebufferCache->getPrimaryDepthTexture();
    auto normalBuffer = framebufferCache->getDeferredNormalTexture();
    auto pyramidFBO = framebufferCache->getDepthPyramidFramebuffer();
    auto occlusionFBO = framebufferCache->getOcclusionFramebuffer();
    auto occlusionBlurredFBO = framebufferCache->getOcclusionBlurredFramebuffer();
    auto occlusionFinalFBO = framebufferCache->getDeferredFramebufferDepthColor();

    QSize framebufferSize = framebufferCache->getFrameBufferSize();
    float sMin = args->_viewport.x / (float)framebufferSize.width();
    float sWidth = args->_viewport.z / (float)framebufferSize.width();
    float tMin = args->_viewport.y / (float)framebufferSize.height();
    float tHeight = args->_viewport.w / (float)framebufferSize.height();

    // Update the depth info with near and far (same for stereo)
    setDepthInfo(args->_viewFrustum->getNearClip(), args->_viewFrustum->getFarClip());

    _parametersBuffer.edit<Parameters>()._pixelInfo = args->_viewport;
    //_parametersBuffer.edit<Parameters>()._ditheringInfo.y += 0.25f;

    // Running in stero ?
    bool isStereo = args->_context->isStereo();
    if (!isStereo) {
        // Eval the mono projection
        mat4 monoProjMat;
        args->_viewFrustum->evalProjectionMatrix(monoProjMat);
        _parametersBuffer.edit<Parameters>()._projection[0] = monoProjMat;
        _parametersBuffer.edit<Parameters>()._stereoInfo = glm::vec4(0.0f, args->_viewport.z, 0.0f, 0.0f);

    } else {

        mat4 projMats[2];
        mat4 eyeViews[2];
        args->_context->getStereoProjections(projMats);
        args->_context->getStereoViews(eyeViews);

        float halfWidth = 0.5f * sWidth;

        for (int i = 0; i < 2; i++) {
            // Compose the mono Eye space to Stereo clip space Projection Matrix
            auto sideViewMat = projMats[i] * eyeViews[i];
            _parametersBuffer.edit<Parameters>()._projection[i] = sideViewMat;
        }

        _parametersBuffer.edit<Parameters>()._stereoInfo = glm::vec4(1.0f, (float)(args->_viewport.z >> 1), 0.0f, 1.0f);

    }

    auto pyramidPipeline = getPyramidPipeline();
    auto occlusionPipeline = getOcclusionPipeline();
    auto firstHBlurPipeline = getHBlurPipeline();
    auto lastVBlurPipeline = getVBlurPipeline();
    
    gpu::doInBatch(args->_context, [=](gpu::Batch& batch) {
        batch.enableStereo(false);

        batch.setViewportTransform(args->_viewport);
        batch.setProjectionTransform(glm::mat4());
        batch.setViewTransform(Transform());

        Transform model;
        model.setTranslation(glm::vec3(sMin, tMin, 0.0));
        model.setScale(glm::vec3(sWidth, tHeight, 1.0));
        batch.setModelTransform(model);

        batch.setUniformBuffer(AmbientOcclusionEffect_ParamsSlot, _parametersBuffer);


        // Pyramid pass
        batch.setFramebuffer(pyramidFBO);
        batch.clearColorFramebuffer(gpu::Framebuffer::BUFFER_COLOR0, glm::vec4(args->_viewFrustum->getFarClip(), 0.0f, 0.0f, 0.0f));
        batch.setPipeline(pyramidPipeline);
        batch.setResourceTexture(AmbientOcclusionEffect_DepthMapSlot, depthBuffer);
        batch.draw(gpu::TRIANGLE_STRIP, 4);

        // Make pyramid mips
        batch.generateTextureMips(pyramidFBO->getRenderBuffer(0));

        // Occlusion pass
        batch.setFramebuffer(occlusionFBO);
        batch.clearColorFramebuffer(gpu::Framebuffer::BUFFER_COLOR0, glm::vec4(1.0f));
        batch.setPipeline(occlusionPipeline);
        batch.setResourceTexture(AmbientOcclusionEffect_PyramidMapSlot, pyramidFBO->getRenderBuffer(0));
        batch.draw(gpu::TRIANGLE_STRIP, 4);
        batch.setResourceTexture(AmbientOcclusionEffect_PyramidMapSlot + 1, gpu::TexturePointer());
        
        // Blur 1 pass
        batch.setFramebuffer(occlusionBlurredFBO);
        batch.setPipeline(firstHBlurPipeline);
        batch.setResourceTexture(AmbientOcclusionEffect_OcclusionMapSlot, occlusionFBO->getRenderBuffer(0));
        batch.draw(gpu::TRIANGLE_STRIP, 4);

        // Blur 2 pass
        batch.setFramebuffer(occlusionFinalFBO);
        batch.setPipeline(lastVBlurPipeline);
        batch.setResourceTexture(AmbientOcclusionEffect_OcclusionMapSlot, occlusionBlurredFBO->getRenderBuffer(0));
        batch.draw(gpu::TRIANGLE_STRIP, 4);

    });
}
