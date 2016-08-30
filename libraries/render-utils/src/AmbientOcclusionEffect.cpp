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
#include "ssao_debugOcclusion_frag.h"
#include "ssao_makeHorizontalBlur_frag.h"
#include "ssao_makeVerticalBlur_frag.h"


AmbientOcclusionFramebuffer::AmbientOcclusionFramebuffer() {
}

void AmbientOcclusionFramebuffer::updateLinearDepth(const gpu::TexturePointer& linearDepthBuffer) {
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

void AmbientOcclusionFramebuffer::clear() {
    _occlusionFramebuffer.reset();
    _occlusionTexture.reset();
    _occlusionBlurredFramebuffer.reset();
    _occlusionBlurredTexture.reset();
}

gpu::TexturePointer AmbientOcclusionFramebuffer::getLinearDepthTexture() {
    return _linearDepthTexture;
}

void AmbientOcclusionFramebuffer::allocate() {
    
    auto width = _frameSize.x;
    auto height = _frameSize.y;
    
    _occlusionTexture = gpu::TexturePointer(gpu::Texture::create2D(gpu::Element::COLOR_RGBA_32, width, height, gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_LINEAR_MIP_POINT)));
    _occlusionFramebuffer = gpu::FramebufferPointer(gpu::Framebuffer::create());
    _occlusionFramebuffer->setRenderBuffer(0, _occlusionTexture);
   
    _occlusionBlurredTexture = gpu::TexturePointer(gpu::Texture::create2D(gpu::Element::COLOR_RGBA_32, width, height, gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_LINEAR_MIP_POINT)));
    _occlusionBlurredFramebuffer = gpu::FramebufferPointer(gpu::Framebuffer::create());
    _occlusionBlurredFramebuffer->setRenderBuffer(0, _occlusionBlurredTexture);
}

gpu::FramebufferPointer AmbientOcclusionFramebuffer::getOcclusionFramebuffer() {
    if (!_occlusionFramebuffer) {
        allocate();
    }
    return _occlusionFramebuffer;
}

gpu::TexturePointer AmbientOcclusionFramebuffer::getOcclusionTexture() {
    if (!_occlusionTexture) {
        allocate();
    }
    return _occlusionTexture;
}

gpu::FramebufferPointer AmbientOcclusionFramebuffer::getOcclusionBlurredFramebuffer() {
    if (!_occlusionBlurredFramebuffer) {
        allocate();
    }
    return _occlusionBlurredFramebuffer;
}

gpu::TexturePointer AmbientOcclusionFramebuffer::getOcclusionBlurredTexture() {
    if (!_occlusionBlurredTexture) {
        allocate();
    }
    return _occlusionBlurredTexture;
}


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
const int AmbientOcclusionEffect_CameraCorrectionSlot = 2;
const int AmbientOcclusionEffect_LinearDepthMapSlot = 0;
const int AmbientOcclusionEffect_OcclusionMapSlot = 0;

AmbientOcclusionEffect::AmbientOcclusionEffect() {
}

void AmbientOcclusionEffect::configure(const Config& config) {
    DependencyManager::get<DeferredLightingEffect>()->setAmbientOcclusionEnabled(config.enabled);

    bool shouldUpdateGaussian = false;

    const double RADIUS_POWER = 6.0;
    const auto& radius = config.radius;
    if (radius != _parametersBuffer->getRadius()) {
        auto& current = _parametersBuffer->radiusInfo;
        current.x = radius;
        current.y = radius * radius;
        current.z = (float)(1.0 / pow((double)radius, RADIUS_POWER));
    }

    if (config.obscuranceLevel != _parametersBuffer->getObscuranceLevel()) {
        auto& current = _parametersBuffer->radiusInfo;
        current.w = config.obscuranceLevel;
    }

    if (config.falloffBias != _parametersBuffer->getFalloffBias()) {
        auto& current = _parametersBuffer->ditheringInfo;
        current.z = config.falloffBias;
    }

    if (config.edgeSharpness != _parametersBuffer->getEdgeSharpness()) {
        auto& current = _parametersBuffer->blurInfo;
        current.x = config.edgeSharpness;
    }

    if (config.blurDeviation != _parametersBuffer->getBlurDeviation()) {
        auto& current = _parametersBuffer->blurInfo;
        current.z = config.blurDeviation;
        shouldUpdateGaussian = true;
    }

    if (config.numSpiralTurns != _parametersBuffer->getNumSpiralTurns()) {
        auto& current = _parametersBuffer->sampleInfo;
        current.z = config.numSpiralTurns;
    }

    if (config.numSamples != _parametersBuffer->getNumSamples()) {
        auto& current = _parametersBuffer->sampleInfo;
        current.x = config.numSamples;
        current.y = 1.0f / config.numSamples;
    }

    if (config.fetchMipsEnabled != _parametersBuffer->isFetchMipsEnabled()) {
        auto& current = _parametersBuffer->sampleInfo;
        current.w = (float)config.fetchMipsEnabled;
    }

    if (!_framebuffer) {
        _framebuffer = std::make_shared<AmbientOcclusionFramebuffer>();
    }
    
    if (config.perspectiveScale != _parametersBuffer->getPerspectiveScale()) {
        _parametersBuffer->resolutionInfo.z = config.perspectiveScale;
    }
    if (config.resolutionLevel != _parametersBuffer->getResolutionLevel()) {
        auto& current = _parametersBuffer->resolutionInfo;
        current.x = (float) config.resolutionLevel;
    }
 
    if (config.blurRadius != _parametersBuffer->getBlurRadius()) {
        auto& current = _parametersBuffer->blurInfo;
        current.y = (float)config.blurRadius;
        shouldUpdateGaussian = true;
    }

    if (config.ditheringEnabled != _parametersBuffer->isDitheringEnabled()) {
        auto& current = _parametersBuffer->ditheringInfo;
        current.x = (float)config.ditheringEnabled;
    }

    if (config.borderingEnabled != _parametersBuffer->isBorderingEnabled()) {
        auto& current = _parametersBuffer->ditheringInfo;
        current.w = (float)config.borderingEnabled;
    }

    if (shouldUpdateGaussian) {
        updateGaussianDistribution();
    }
}

const gpu::PipelinePointer& AmbientOcclusionEffect::getOcclusionPipeline() {
    if (!_occlusionPipeline) {
        auto vs = gpu::StandardShaderLib::getDrawViewportQuadTransformTexcoordVS();
        auto ps = gpu::Shader::createPixel(std::string(ssao_makeOcclusion_frag));
        gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);

        gpu::Shader::BindingSet slotBindings;
        slotBindings.insert(gpu::Shader::Binding(std::string("deferredFrameTransformBuffer"), AmbientOcclusionEffect_FrameTransformSlot));
        slotBindings.insert(gpu::Shader::Binding(std::string("ambientOcclusionParamsBuffer"), AmbientOcclusionEffect_ParamsSlot));
        slotBindings.insert(gpu::Shader::Binding(std::string("cameraCorrectionBuffer"), AmbientOcclusionEffect_CameraCorrectionSlot));
        
        slotBindings.insert(gpu::Shader::Binding(std::string("pyramidMap"), AmbientOcclusionEffect_LinearDepthMapSlot));
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
        slotBindings.insert(gpu::Shader::Binding(std::string("cameraCorrectionBuffer"), AmbientOcclusionEffect_CameraCorrectionSlot));
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
        slotBindings.insert(gpu::Shader::Binding(std::string("cameraCorrectionBuffer"), AmbientOcclusionEffect_CameraCorrectionSlot));
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

void AmbientOcclusionEffect::updateGaussianDistribution() {
    auto coefs = _parametersBuffer->_gaussianCoefs;
    GaussianDistribution::evalSampling(coefs, Parameters::GAUSSIAN_COEFS_LENGTH, _parametersBuffer->getBlurRadius(), _parametersBuffer->getBlurDeviation());
}

void AmbientOcclusionEffect::run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext, const Inputs& inputs, Outputs& outputs) {
    assert(renderContext->args);
    assert(renderContext->args->hasViewFrustum());

    RenderArgs* args = renderContext->args;

    const auto& frameTransform = inputs.get0();
    const auto& linearDepthFramebuffer = inputs.get2();
    
    auto linearDepthTexture = linearDepthFramebuffer->getLinearDepthTexture();
    auto sourceViewport = args->_viewport;
    auto occlusionViewport = sourceViewport;

    if (!_framebuffer) {
        _framebuffer = std::make_shared<AmbientOcclusionFramebuffer>();
    }
    
    if (_parametersBuffer->getResolutionLevel() > 0) {
        linearDepthTexture = linearDepthFramebuffer->getHalfLinearDepthTexture();
        occlusionViewport = occlusionViewport >> _parametersBuffer->getResolutionLevel();
    }

    _framebuffer->updateLinearDepth(linearDepthTexture);
  
    
    auto occlusionFBO = _framebuffer->getOcclusionFramebuffer();
    auto occlusionBlurredFBO = _framebuffer->getOcclusionBlurredFramebuffer();
    
    outputs.edit0() = _framebuffer;
    outputs.edit1() = _parametersBuffer;

    auto framebufferSize = _framebuffer->getSourceFrameSize();
    
    float sMin = occlusionViewport.x / (float)framebufferSize.x;
    float sWidth = occlusionViewport.z / (float)framebufferSize.x;
    float tMin = occlusionViewport.y / (float)framebufferSize.y;
    float tHeight = occlusionViewport.w / (float)framebufferSize.y;


    auto occlusionPipeline = getOcclusionPipeline();
    auto firstHBlurPipeline = getHBlurPipeline();
    auto lastVBlurPipeline = getVBlurPipeline();
    
    gpu::doInBatch(args->_context, [=](gpu::Batch& batch) {
        batch.enableStereo(false);

        _gpuTimer.begin(batch);

        batch.setViewportTransform(occlusionViewport);
        batch.setProjectionTransform(glm::mat4());
        batch.resetViewTransform();

        Transform model;
        model.setTranslation(glm::vec3(sMin, tMin, 0.0f));
        model.setScale(glm::vec3(sWidth, tHeight, 1.0f));
        batch.setModelTransform(model);

        batch.setUniformBuffer(AmbientOcclusionEffect_FrameTransformSlot, frameTransform->getFrameTransformBuffer());
        batch.setUniformBuffer(AmbientOcclusionEffect_ParamsSlot, _parametersBuffer);

      
        // We need this with the mips levels  
        batch.generateTextureMips(_framebuffer->getLinearDepthTexture());
        
        // Occlusion pass
        batch.setFramebuffer(occlusionFBO);
        batch.clearColorFramebuffer(gpu::Framebuffer::BUFFER_COLOR0, glm::vec4(1.0f));
        batch.setPipeline(occlusionPipeline);
        batch.setResourceTexture(AmbientOcclusionEffect_LinearDepthMapSlot, _framebuffer->getLinearDepthTexture());
        batch.draw(gpu::TRIANGLE_STRIP, 4);

        
        if (_parametersBuffer->getBlurRadius() > 0) {
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
        
        
        batch.setResourceTexture(AmbientOcclusionEffect_LinearDepthMapSlot, nullptr);
        batch.setResourceTexture(AmbientOcclusionEffect_OcclusionMapSlot, nullptr);
        
        _gpuTimer.end(batch);
    });

    // Update the timer
    auto config = std::static_pointer_cast<Config>(renderContext->jobConfig);
    config->gpuTime = _gpuTimer.getAverageGPU();
    config->gpuBatchTime = _gpuTimer.getAverageCPU();
}



DebugAmbientOcclusion::DebugAmbientOcclusion() {
}

void DebugAmbientOcclusion::configure(const Config& config) {

    _showCursorPixel = config.showCursorPixel;

    auto cursorPos = glm::vec2(_parametersBuffer->pixelInfo);
    if (cursorPos != config.debugCursorTexcoord) {
        _parametersBuffer->pixelInfo = glm::vec4(config.debugCursorTexcoord, 0.0f, 0.0f);
    }
}

const gpu::PipelinePointer& DebugAmbientOcclusion::getDebugPipeline() {
    if (!_debugPipeline) {
        auto vs = gpu::StandardShaderLib::getDrawViewportQuadTransformTexcoordVS();
        auto ps = gpu::Shader::createPixel(std::string(ssao_debugOcclusion_frag));
        gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);

        gpu::Shader::BindingSet slotBindings;
        slotBindings.insert(gpu::Shader::Binding(std::string("deferredFrameTransformBuffer"), AmbientOcclusionEffect_FrameTransformSlot));
        slotBindings.insert(gpu::Shader::Binding(std::string("ambientOcclusionParamsBuffer"), AmbientOcclusionEffect_ParamsSlot));
        slotBindings.insert(gpu::Shader::Binding(std::string("debugAmbientOcclusionBuffer"), 2));
        slotBindings.insert(gpu::Shader::Binding(std::string("pyramidMap"), AmbientOcclusionEffect_LinearDepthMapSlot));
        gpu::Shader::makeProgram(*program, slotBindings);

        gpu::StatePointer state = gpu::StatePointer(new gpu::State());

        state->setColorWriteMask(true, true, true, false);
        state->setBlendFunction(true, gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA);
        // Good to go add the brand new pipeline
        _debugPipeline = gpu::Pipeline::create(program, state);
    }
    return _debugPipeline;
}

void DebugAmbientOcclusion::run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext, const Inputs& inputs) {
    assert(renderContext->args);
    assert(renderContext->args->hasViewFrustum());

    if (!_showCursorPixel) {
        return;
    }

    RenderArgs* args = renderContext->args;

    const auto& frameTransform = inputs.get0();
    const auto& linearDepthFramebuffer = inputs.get2();
    const auto& ambientOcclusionUniforms = inputs.get3();
    
    // Skip if AO is not started yet
    if (!ambientOcclusionUniforms._buffer) {
        return;
    }

    auto linearDepthTexture = linearDepthFramebuffer->getLinearDepthTexture();
    auto sourceViewport = args->_viewport;
    auto occlusionViewport = sourceViewport;

    auto resolutionLevel = ambientOcclusionUniforms->getResolutionLevel();
    
    if (resolutionLevel > 0) {
        linearDepthTexture = linearDepthFramebuffer->getHalfLinearDepthTexture();
        occlusionViewport = occlusionViewport >> ambientOcclusionUniforms->getResolutionLevel();
    }
        

    auto framebufferSize = glm::ivec2(linearDepthTexture->getDimensions());
    
    float sMin = occlusionViewport.x / (float)framebufferSize.x;
    float sWidth = occlusionViewport.z / (float)framebufferSize.x;
    float tMin = occlusionViewport.y / (float)framebufferSize.y;
    float tHeight = occlusionViewport.w / (float)framebufferSize.y;
    
    auto debugPipeline = getDebugPipeline();
    
    gpu::doInBatch(args->_context, [=](gpu::Batch& batch) {
        batch.enableStereo(false);

        batch.setViewportTransform(sourceViewport);
        batch.setProjectionTransform(glm::mat4());
        batch.setViewTransform(Transform());

        Transform model;
        model.setTranslation(glm::vec3(sMin, tMin, 0.0f));
        model.setScale(glm::vec3(sWidth, tHeight, 1.0f));
        batch.setModelTransform(model);

        batch.setUniformBuffer(AmbientOcclusionEffect_FrameTransformSlot, frameTransform->getFrameTransformBuffer());
        batch.setUniformBuffer(AmbientOcclusionEffect_ParamsSlot, ambientOcclusionUniforms);
        batch.setUniformBuffer(2, _parametersBuffer);
        
        batch.setPipeline(debugPipeline);
        batch.setResourceTexture(AmbientOcclusionEffect_LinearDepthMapSlot, linearDepthTexture);
        batch.draw(gpu::TRIANGLE_STRIP, 4);

        
        batch.setResourceTexture(AmbientOcclusionEffect_LinearDepthMapSlot, nullptr);    
    });

}
 