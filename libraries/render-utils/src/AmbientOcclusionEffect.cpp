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
#include "RenderUtilsLogging.h"

#include "DeferredLightingEffect.h"
#include "AmbientOcclusionEffect.h"
#include "TextureCache.h"
#include "FramebufferCache.h"
#include "DependencyManager.h"
#include "ViewFrustum.h"

#include "ssao_makePyramid_frag.h"
#include "ssao_makeOcclusion_frag.h"
#include "ssao_makeHorizontalBlur_frag.h"
#include "ssao_makeVerticalBlur_frag.h"

class GaussianDistribution {
public:
    
    static double integral(float x, float deviation) {
        return 0.5 * erf((double)x / ((double)deviation * sqrt(2.0)));
    }
    
    static double rangeIntegral(float x0, float x1, float deviation) {
        return integral(x1, deviation) - integral(x0, deviation);
    }
    
    static std::vector<float> evalSampling(int samplingRadius, float deviation) {
        std::vector<float> coefs(samplingRadius + 1, 0.0f);
        
        // corner case when radius is 0 or under
        if (samplingRadius <= 0) {
            coefs[0] = 1.0f;
            return coefs;
        }
        
        // Evaluate all the samples range integral of width 1 from center until the penultimate one
        float halfWidth = 0.5f;
        double sum = 0.0;
        for (int i = 0; i < samplingRadius; i++) {
            float x = (float) i;
            double sample = rangeIntegral(x - halfWidth, x + halfWidth, deviation);
            coefs[i] = sample;
            sum += sample;
        }
        
        // last sample goes to infinity
        float lastSampleX0 = (float) samplingRadius - halfWidth;
        float largeEnough = lastSampleX0 + 1000.0f * deviation;
        double sample = rangeIntegral(lastSampleX0, largeEnough, deviation);
        coefs[samplingRadius] = sample;
        sum += sample;
        
        return coefs;
    }
    
    static void evalSampling(float* coefs, unsigned int coefsLength, int samplingRadius, float deviation) {
        auto coefsVector = evalSampling(samplingRadius, deviation);
        if (coefsLength> coefsVector.size() + 1) {
            unsigned int coefsNum = 0;
            for (auto s : coefsVector) {
                coefs[coefsNum] = s;
                coefsNum++;
            }
            for (;coefsNum < coefsLength; coefsNum++) {
                coefs[coefsNum] = 0.0f;
            }
        }
    }
};

const int AmbientOcclusionEffect_FrameTransformSlot = 0;
const int AmbientOcclusionEffect_ParamsSlot = 1;
const int AmbientOcclusionEffect_DepthMapSlot = 0;
const int AmbientOcclusionEffect_PyramidMapSlot = 0;
const int AmbientOcclusionEffect_OcclusionMapSlot = 0;

AmbientOcclusionEffect::AmbientOcclusionEffect() {
    FrameTransform frameTransform;
    _frameTransformBuffer = gpu::BufferView(std::make_shared<gpu::Buffer>(sizeof(FrameTransform), (const gpu::Byte*) &frameTransform));
    Parameters parameters;
    _parametersBuffer = gpu::BufferView(std::make_shared<gpu::Buffer>(sizeof(Parameters), (const gpu::Byte*) &parameters));
}

void AmbientOcclusionEffect::configure(const Config& config) {
    DependencyManager::get<DeferredLightingEffect>()->setAmbientOcclusionEnabled(config.enabled);

    bool shouldUpdateGaussian = false;

    const double RADIUS_POWER = 6.0;
    const auto& radius = config.radius;
    if (radius != getRadius()) {
        auto& current = _parametersBuffer.edit<Parameters>().radiusInfo;
        current.x = radius;
        current.y = radius * radius;
        current.z = (float)(1.0 / pow((double)radius, RADIUS_POWER));
    }

    if (config.obscuranceLevel != getObscuranceLevel()) {
        auto& current = _parametersBuffer.edit<Parameters>().radiusInfo;
        current.w = config.obscuranceLevel;
    }

    if (config.falloffBias != getFalloffBias()) {
        auto& current = _parametersBuffer.edit<Parameters>().ditheringInfo;
        current.z = config.falloffBias;
    }

    if (config.edgeSharpness != getEdgeSharpness()) {
        auto& current = _parametersBuffer.edit<Parameters>().blurInfo;
        current.x = config.edgeSharpness;
    }

    if (config.blurDeviation != getBlurDeviation()) {
        auto& current = _parametersBuffer.edit<Parameters>().blurInfo;
        current.z = config.blurDeviation;
        shouldUpdateGaussian = true;
    }

    if (config.numSpiralTurns != getNumSpiralTurns()) {
        auto& current = _parametersBuffer.edit<Parameters>().sampleInfo;
        current.z = config.numSpiralTurns;
    }

    if (config.numSamples != getNumSamples()) {
        auto& current = _parametersBuffer.edit<Parameters>().sampleInfo;
        current.x = config.numSamples;
        current.y = 1.0f / config.numSamples;
    }

    const auto& resolutionLevel = config.resolutionLevel;
    if (resolutionLevel != getResolutionLevel()) {
        auto& current = _parametersBuffer.edit<Parameters>().resolutionInfo;
        current.x = (float)resolutionLevel;

        // Communicate the change to the Framebuffer cache
        DependencyManager::get<FramebufferCache>()->setAmbientOcclusionResolutionLevel(resolutionLevel);
    }

    if (config.blurRadius != getBlurRadius()) {
        auto& current = _parametersBuffer.edit<Parameters>().blurInfo;
        current.y = (float)config.blurRadius;
        shouldUpdateGaussian = true;
    }

    if (config.ditheringEnabled != isDitheringEnabled()) {
        auto& current = _parametersBuffer.edit<Parameters>().ditheringInfo;
        current.x = (float)config.ditheringEnabled;
    }

    if (config.borderingEnabled != isBorderingEnabled()) {
        auto& current = _parametersBuffer.edit<Parameters>().ditheringInfo;
        current.w = (float)config.borderingEnabled;
    }

    if (shouldUpdateGaussian) {
        updateGaussianDistribution();
    }
}

const gpu::PipelinePointer& AmbientOcclusionEffect::getPyramidPipeline() {
    if (!_pyramidPipeline) {
        auto vs = gpu::StandardShaderLib::getDrawViewportQuadTransformTexcoordVS();
        auto ps = gpu::Shader::createPixel(std::string(ssao_makePyramid_frag));
        gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);

        gpu::Shader::BindingSet slotBindings;
        slotBindings.insert(gpu::Shader::Binding(std::string("ambientOcclusionFrameTransformBuffer"), AmbientOcclusionEffect_FrameTransformSlot));
        slotBindings.insert(gpu::Shader::Binding(std::string("ambientOcclusionParamsBuffer"), AmbientOcclusionEffect_ParamsSlot));
        slotBindings.insert(gpu::Shader::Binding(std::string("depthMap"), AmbientOcclusionEffect_DepthMapSlot));
        gpu::Shader::makeProgram(*program, slotBindings);


        gpu::StatePointer state = gpu::StatePointer(new gpu::State());

        // Stencil test the pyramid passe for objects pixels only, not the background
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
        slotBindings.insert(gpu::Shader::Binding(std::string("ambientOcclusionFrameTransformBuffer"), AmbientOcclusionEffect_FrameTransformSlot));
        slotBindings.insert(gpu::Shader::Binding(std::string("ambientOcclusionParamsBuffer"), AmbientOcclusionEffect_ParamsSlot));
        slotBindings.insert(gpu::Shader::Binding(std::string("pyramidMap"), AmbientOcclusionEffect_PyramidMapSlot));
        gpu::Shader::makeProgram(*program, slotBindings);

        gpu::StatePointer state = gpu::StatePointer(new gpu::State());

        state->setColorWriteMask(true, true, true, false);

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
        slotBindings.insert(gpu::Shader::Binding(std::string("ambientOcclusionFrameTransformBuffer"), AmbientOcclusionEffect_FrameTransformSlot));
        slotBindings.insert(gpu::Shader::Binding(std::string("ambientOcclusionParamsBuffer"), AmbientOcclusionEffect_ParamsSlot));
        slotBindings.insert(gpu::Shader::Binding(std::string("occlusionMap"), AmbientOcclusionEffect_OcclusionMapSlot));
        gpu::Shader::makeProgram(*program, slotBindings);
        
        gpu::StatePointer state = gpu::StatePointer(new gpu::State());

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
        slotBindings.insert(gpu::Shader::Binding(std::string("ambientOcclusionFrameTransformBuffer"), AmbientOcclusionEffect_FrameTransformSlot));
        slotBindings.insert(gpu::Shader::Binding(std::string("ambientOcclusionParamsBuffer"), AmbientOcclusionEffect_ParamsSlot));
        slotBindings.insert(gpu::Shader::Binding(std::string("occlusionMap"), AmbientOcclusionEffect_OcclusionMapSlot));
        
        gpu::Shader::makeProgram(*program, slotBindings);
        
        gpu::StatePointer state = gpu::StatePointer(new gpu::State());
        
        // Vertical blur write just the final result Occlusion value in the alpha channel
        state->setColorWriteMask(true, true, true, false);

        // Good to go add the brand new pipeline
        _vBlurPipeline = gpu::Pipeline::create(program, state);
    }
    return _vBlurPipeline;
}


void AmbientOcclusionEffect::setDepthInfo(float nearZ, float farZ) {
    _frameTransformBuffer.edit<FrameTransform>().depthInfo = glm::vec4(nearZ*farZ, farZ -nearZ, -farZ, 0.0f);
}

void AmbientOcclusionEffect::updateGaussianDistribution() {
    auto coefs = _parametersBuffer.edit<Parameters>()._gaussianCoefs;
    GaussianDistribution::evalSampling(coefs, Parameters::GAUSSIAN_COEFS_LENGTH, getBlurRadius(), getBlurDeviation());
}

void AmbientOcclusionEffect::run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext) {
#ifdef FIX_THE_FRAMEBUFFER_CACHE
    assert(renderContext->args);
    assert(renderContext->args->hasViewFrustum());

    RenderArgs* args = renderContext->args;

    // FIXME: Different render modes should have different tasks
    if (args->_renderMode != RenderArgs::DEFAULT_RENDER_MODE) {
        return;
    }

    auto framebufferCache = DependencyManager::get<FramebufferCache>();
    auto depthBuffer = framebufferCache->getPrimaryDepthTexture();
    auto normalBuffer = framebufferCache->getDeferredNormalTexture();
    auto pyramidFBO = framebufferCache->getDepthPyramidFramebuffer();
    auto occlusionFBO = framebufferCache->getOcclusionFramebuffer();
    auto occlusionBlurredFBO = framebufferCache->getOcclusionBlurredFramebuffer();

    QSize framebufferSize = framebufferCache->getFrameBufferSize();
    float sMin = args->_viewport.x / (float)framebufferSize.width();
    float sWidth = args->_viewport.z / (float)framebufferSize.width();
    float tMin = args->_viewport.y / (float)framebufferSize.height();
    float tHeight = args->_viewport.w / (float)framebufferSize.height();

    auto resolutionLevel = getResolutionLevel();

    // Update the depth info with near and far (same for stereo)
    setDepthInfo(args->getViewFrustum().getNearClip(), args->getViewFrustum().getFarClip());

    _frameTransformBuffer.edit<FrameTransform>().pixelInfo = args->_viewport;
    //_parametersBuffer.edit<Parameters>()._ditheringInfo.y += 0.25f;

    // Running in stero ?
    bool isStereo = args->_context->isStereo();
    if (!isStereo) {
        // Eval the mono projection
        mat4 monoProjMat;
        args->getViewFrustum().evalProjectionMatrix(monoProjMat);
        _frameTransformBuffer.edit<FrameTransform>().projection[0] = monoProjMat;
        _frameTransformBuffer.edit<FrameTransform>().stereoInfo = glm::vec4(0.0f, (float)args->_viewport.z, 0.0f, 0.0f);

    } else {

        mat4 projMats[2];
        mat4 eyeViews[2];
        args->_context->getStereoProjections(projMats);
        args->_context->getStereoViews(eyeViews);

        for (int i = 0; i < 2; i++) {
            // Compose the mono Eye space to Stereo clip space Projection Matrix
            auto sideViewMat = projMats[i] * eyeViews[i];
            _frameTransformBuffer.edit<FrameTransform>().projection[i] = sideViewMat;
        }

        _frameTransformBuffer.edit<FrameTransform>().stereoInfo = glm::vec4(1.0f, (float)(args->_viewport.z >> 1), 0.0f, 1.0f);

    }

    auto pyramidPipeline = getPyramidPipeline();
    auto occlusionPipeline = getOcclusionPipeline();
    auto firstHBlurPipeline = getHBlurPipeline();
    auto lastVBlurPipeline = getVBlurPipeline();
    
    gpu::doInBatch(args->_context, [=](gpu::Batch& batch) {
        batch.enableStereo(false);

        _gpuTimer.begin(batch);

        batch.setViewportTransform(args->_viewport);
        batch.setProjectionTransform(glm::mat4());
        batch.setViewTransform(Transform());

        Transform model;
        model.setTranslation(glm::vec3(sMin, tMin, 0.0f));
        model.setScale(glm::vec3(sWidth, tHeight, 1.0f));
        batch.setModelTransform(model);

        batch.setUniformBuffer(AmbientOcclusionEffect_FrameTransformSlot, _frameTransformBuffer);
        batch.setUniformBuffer(AmbientOcclusionEffect_ParamsSlot, _parametersBuffer);


        // Pyramid pass
        batch.setFramebuffer(pyramidFBO);
        batch.clearColorFramebuffer(gpu::Framebuffer::BUFFER_COLOR0, glm::vec4(args->getViewFrustum().getFarClip(), 0.0f, 0.0f, 0.0f));
        batch.setPipeline(pyramidPipeline);
        batch.setResourceTexture(AmbientOcclusionEffect_DepthMapSlot, depthBuffer);
        batch.draw(gpu::TRIANGLE_STRIP, 4);

        // Make pyramid mips
        batch.generateTextureMips(pyramidFBO->getRenderBuffer(0));

        // Adjust Viewport for rendering resolution
        if (resolutionLevel > 0) {
            glm::ivec4 viewport(args->_viewport.x, args->_viewport.y, args->_viewport.z >> resolutionLevel, args->_viewport.w >> resolutionLevel);
            batch.setViewportTransform(viewport);
        }

        // Occlusion pass
        batch.setFramebuffer(occlusionFBO);
        batch.clearColorFramebuffer(gpu::Framebuffer::BUFFER_COLOR0, glm::vec4(1.0f));
        batch.setPipeline(occlusionPipeline);
        batch.setResourceTexture(AmbientOcclusionEffect_PyramidMapSlot, pyramidFBO->getRenderBuffer(0));
        batch.draw(gpu::TRIANGLE_STRIP, 4);

        
        if (getBlurRadius() > 0) {
            // Blur 1st pass
            batch.setFramebuffer(occlusionBlurredFBO);
            batch.setPipeline(firstHBlurPipeline);
            batch.setResourceTexture(AmbientOcclusionEffect_OcclusionMapSlot, occlusionFBO->getRenderBuffer(0));
            batch.draw(gpu::TRIANGLE_STRIP, 4);

            // Blur 2nd pass
            batch.setFramebuffer(occlusionFBO);
            batch.setPipeline(lastVBlurPipeline);
            batch.setResourceTexture(AmbientOcclusionEffect_OcclusionMapSlot, occlusionBlurredFBO->getRenderBuffer(0));
            batch.draw(gpu::TRIANGLE_STRIP, 4);
        }
        
        _gpuTimer.end(batch);
    });

    // Update the timer
    std::static_pointer_cast<Config>(renderContext->jobConfig)->gpuTime = _gpuTimer.getAverage();
#endif
}
