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

#include "gpu/StandardShaderLib.h"
#include "AmbientOcclusionEffect.h"
#include "TextureCache.h"
#include "FramebufferCache.h"
#include "DependencyManager.h"
#include "ViewFrustum.h"
#include "GeometryCache.h"

#include "ssao_makePyramid_frag.h"
#include "ssao_makeOcclusion_frag.h"
#include "ssao_makeHorizontalBlur_frag.h"
#include "ssao_makeVerticalBlur_frag.h"

const int AmbientOcclusionEffect_ParamsSlot = 0;
const int AmbientOcclusionEffect_DeferredTransformSlot = 1;
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
        slotBindings.insert(gpu::Shader::Binding(std::string("deferredTransformBuffer"), AmbientOcclusionEffect_DeferredTransformSlot));
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
        slotBindings.insert(gpu::Shader::Binding(std::string("deferredTransformBuffer"), AmbientOcclusionEffect_DeferredTransformSlot));
        slotBindings.insert(gpu::Shader::Binding(std::string("pyramidMap"), AmbientOcclusionEffect_PyramidMapSlot));
        
        slotBindings.insert(gpu::Shader::Binding(std::string("normalMap"), AmbientOcclusionEffect_PyramidMapSlot + 1));
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
        slotBindings.insert(gpu::Shader::Binding(std::string("deferredTransformBuffer"), AmbientOcclusionEffect_DeferredTransformSlot));
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
        slotBindings.insert(gpu::Shader::Binding(std::string("deferredTransformBuffer"), AmbientOcclusionEffect_DeferredTransformSlot));
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

void AmbientOcclusionEffect::updateDeferredTransformBuffer(const render::RenderContextPointer& renderContext) {
    // Allocate the parameters buffer used by all the deferred shaders
    if (!_deferredTransformBuffer[0]._buffer) {
        DeferredTransform parameters;
        _deferredTransformBuffer[0] = gpu::BufferView(std::make_shared<gpu::Buffer>(sizeof(DeferredTransform), (const gpu::Byte*) &parameters));
        _deferredTransformBuffer[1] = gpu::BufferView(std::make_shared<gpu::Buffer>(sizeof(DeferredTransform), (const gpu::Byte*) &parameters));
    }

    RenderArgs* args = renderContext->getArgs();

    // THe main viewport is assumed to be the mono viewport (or the 2 stereo faces side by side within that viewport)
    auto framebufferCache = DependencyManager::get<FramebufferCache>();
    QSize framebufferSize = framebufferCache->getFrameBufferSize();
    auto monoViewport = args->_viewport;
    float sMin = args->_viewport.x / (float)framebufferSize.width();
    float sWidth = args->_viewport.z / (float)framebufferSize.width();
    float tMin = args->_viewport.y / (float)framebufferSize.height();
    float tHeight = args->_viewport.w / (float)framebufferSize.height();

    // The view frustum is the mono frustum base
    auto viewFrustum = args->_viewFrustum;

    // Eval the mono projection
    mat4 monoProjMat;
    viewFrustum->evalProjectionMatrix(monoProjMat);

    // The mono view transform
    Transform monoViewTransform;
    viewFrustum->evalViewTransform(monoViewTransform);

    // THe mono view matrix coming from the mono view transform
    glm::mat4 monoViewMat;
    monoViewTransform.getMatrix(monoViewMat);

    // Running in stero ?
    bool isStereo = args->_context->isStereo();
    int numPasses = 1;
    
    mat4 projMats[2];
    Transform viewTransforms[2];
    ivec4 viewports[2];
    vec4 clipQuad[2];
    vec2 screenBottomLeftCorners[2];
    vec2 screenTopRightCorners[2];
    vec4 fetchTexcoordRects[2];

    DeferredTransform deferredTransforms[2];

    if (isStereo) {
        numPasses = 2;

        mat4 eyeViews[2];
        args->_context->getStereoProjections(projMats);
        args->_context->getStereoViews(eyeViews);

        float halfWidth = 0.5f * sWidth;

        for (int i = 0; i < numPasses; i++) {
            // In stereo, the 2 sides are layout side by side in the mono viewport and their width is half
            int sideWidth = monoViewport.z >> 1;
            viewports[i] = ivec4(monoViewport.x + (i * sideWidth), monoViewport.y, sideWidth, monoViewport.w);

            deferredTransforms[i].projection = projMats[i];

            auto sideViewMat = monoViewMat * glm::inverse(eyeViews[i]);
            viewTransforms[i].evalFromRawMatrix(sideViewMat);
            deferredTransforms[i].viewInverse = sideViewMat;

            deferredTransforms[i].stereoSide = (i == 0 ? -1.0f : 1.0f);

            clipQuad[i] = glm::vec4(sMin + i * halfWidth, tMin, halfWidth, tHeight);
            screenBottomLeftCorners[i] = glm::vec2(-1.0f + i * 1.0f, -1.0f);
            screenTopRightCorners[i] = glm::vec2(i * 1.0f, 1.0f);

            fetchTexcoordRects[i] = glm::vec4(sMin + i * halfWidth, tMin, halfWidth, tHeight);
        }
    } else {

        viewports[0] = monoViewport;
        projMats[0] = monoProjMat;

        deferredTransforms[0].projection = monoProjMat;

        deferredTransforms[0].viewInverse = monoViewMat;
        viewTransforms[0] = monoViewTransform;

        deferredTransforms[0].stereoSide = 0.0f;

        clipQuad[0] = glm::vec4(sMin, tMin, sWidth, tHeight);
        screenBottomLeftCorners[0] = glm::vec2(-1.0f, -1.0f);
        screenTopRightCorners[0] = glm::vec2(1.0f, 1.0f);

        fetchTexcoordRects[0] = glm::vec4(sMin, tMin, sWidth, tHeight);
    }

    _deferredTransformBuffer[0]._buffer->setSubData(0, sizeof(DeferredTransform), (const gpu::Byte*) &deferredTransforms[0]);
    _deferredTransformBuffer[1]._buffer->setSubData(0, sizeof(DeferredTransform), (const gpu::Byte*) &deferredTransforms[1]);

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


    updateDeferredTransformBuffer(renderContext);

    // Eval the mono projection
    mat4 monoProjMat;
    args->_viewFrustum->evalProjectionMatrix(monoProjMat);

    setDepthInfo(args->_viewFrustum->getNearClip(), args->_viewFrustum->getFarClip());
    _parametersBuffer.edit<Parameters>()._projection[0] = monoProjMat;
    _parametersBuffer.edit<Parameters>()._pixelInfo = args->_viewport;

    auto pyramidPipeline = getPyramidPipeline();
    auto occlusionPipeline = getOcclusionPipeline();
    auto firstHBlurPipeline = getHBlurPipeline();
    auto lastVBlurPipeline = getVBlurPipeline();
    
    gpu::doInBatch(args->_context, [=](gpu::Batch& batch) {
        batch.enableStereo(false);

        batch.setUniformBuffer(AmbientOcclusionEffect_DeferredTransformSlot, _deferredTransformBuffer[0]);


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
        batch.setFramebuffer(occlusionFBO);
        batch.generateTextureMips(pyramidFBO->getRenderBuffer(0));

        // Occlusion pass
        batch.setPipeline(occlusionPipeline);
        
       // batch.setFramebuffer(occlusionFinalFBO);
        batch.setResourceTexture(AmbientOcclusionEffect_PyramidMapSlot, pyramidFBO->getRenderBuffer(0));
        batch.setResourceTexture(AmbientOcclusionEffect_PyramidMapSlot + 1, normalBuffer);
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
