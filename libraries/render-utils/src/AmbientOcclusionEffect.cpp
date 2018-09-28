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

#include "AmbientOcclusionEffect.h"

#include <algorithm> //min max and more

#include <glm/gtc/random.hpp>

#include <PathUtils.h>
#include <SharedUtil.h>
#include <gpu/Context.h>
#include <shaders/Shaders.h>
#include <render/ShapePipeline.h>
#include <MathUtils.h>

#include "RenderUtilsLogging.h"

#include "render-utils/ShaderConstants.h"
#include "DeferredLightingEffect.h"
#include "TextureCache.h"
#include "FramebufferCache.h"
#include "DependencyManager.h"
#include "ViewFrustum.h"

gpu::PipelinePointer AmbientOcclusionEffect::_occlusionPipeline;
gpu::PipelinePointer AmbientOcclusionEffect::_bilateralBlurPipeline;
gpu::PipelinePointer AmbientOcclusionEffect::_mipCreationPipeline;
gpu::PipelinePointer AmbientOcclusionEffect::_gatherPipeline;
gpu::PipelinePointer AmbientOcclusionEffect::_buildNormalsPipeline;

AmbientOcclusionFramebuffer::AmbientOcclusionFramebuffer() {
}

bool AmbientOcclusionFramebuffer::update(const gpu::TexturePointer& linearDepthBuffer, int resolutionLevel, int depthResolutionLevel, bool isStereo) {
    // If the depth buffer or size changed, we need to delete our FBOs
    bool reset = false;
    if (_linearDepthTexture != linearDepthBuffer) {
        _linearDepthTexture = linearDepthBuffer;
        reset = true;
    }
    if (_resolutionLevel != resolutionLevel || isStereo != _isStereo || _depthResolutionLevel != depthResolutionLevel) {
        _resolutionLevel = resolutionLevel;
        _depthResolutionLevel = depthResolutionLevel;
        _isStereo = isStereo;
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

    return reset;
}

void AmbientOcclusionFramebuffer::clear() {
    _occlusionFramebuffer.reset();
    _occlusionTexture.reset();
    _occlusionBlurredFramebuffer.reset();
    _occlusionBlurredTexture.reset();
    _normalFramebuffer.reset();
    _normalTexture.reset();
}

gpu::TexturePointer AmbientOcclusionFramebuffer::getLinearDepthTexture() {
    return _linearDepthTexture;
}

void AmbientOcclusionFramebuffer::allocate() {
#if SSAO_BILATERAL_BLUR_USE_NORMAL    
    auto occlusionformat = gpu::Element{ gpu::VEC4, gpu::HALF, gpu::RGBA };
#else
    auto occlusionformat = gpu::Element{ gpu::VEC3, gpu::NUINT8, gpu::RGB };
#endif

    //  Full frame
    {
        auto width = _frameSize.x;
        auto height = _frameSize.y;
        auto sampler = gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_LINEAR, gpu::Sampler::WRAP_CLAMP);

        _occlusionTexture = gpu::Texture::createRenderBuffer(occlusionformat, width, height, gpu::Texture::SINGLE_MIP, sampler);
        _occlusionFramebuffer = gpu::FramebufferPointer(gpu::Framebuffer::create("occlusion"));
        _occlusionFramebuffer->setRenderBuffer(0, _occlusionTexture);

        _occlusionBlurredTexture = gpu::Texture::createRenderBuffer(occlusionformat, width, height, gpu::Texture::SINGLE_MIP, sampler);
        _occlusionBlurredFramebuffer = gpu::FramebufferPointer(gpu::Framebuffer::create("occlusionBlurred"));
        _occlusionBlurredFramebuffer->setRenderBuffer(0, _occlusionBlurredTexture);
    }

    // Lower res frame
    {
        auto sideSize = _frameSize;
        if (_isStereo) {
            sideSize.x >>= 1;
        }
        sideSize >>= _resolutionLevel;
        if (_isStereo) {
            sideSize.x <<= 1;
        }
        auto width = sideSize.x;
        auto height = sideSize.y;
        auto format = gpu::Element::COLOR_RGBA_32;
        _normalTexture = gpu::Texture::createRenderBuffer(format, width, height, gpu::Texture::SINGLE_MIP, 
                                                          gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_POINT, gpu::Sampler::WRAP_CLAMP));
        _normalFramebuffer = gpu::FramebufferPointer(gpu::Framebuffer::create("ssaoNormals"));
        _normalFramebuffer->setRenderBuffer(0, _normalTexture);
    }

#if SSAO_USE_QUAD_SPLIT
    {
        auto splitSize = _frameSize;
        if (_isStereo) {
            splitSize.x >>= 1;
        }
        splitSize = divideRoundUp(_frameSize >> _resolutionLevel, SSAO_SPLIT_COUNT);
        if (_isStereo) {
            splitSize.x <<= 1;
        }
        auto width = splitSize.x;
        auto height = splitSize.y;

        _occlusionSplitTexture = gpu::Texture::createRenderBufferArray(occlusionformat, width, height, SSAO_SPLIT_COUNT*SSAO_SPLIT_COUNT, gpu::Texture::SINGLE_MIP,
                                                                       gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_LINEAR, gpu::Sampler::WRAP_CLAMP));
        for (int i = 0; i < SSAO_SPLIT_COUNT*SSAO_SPLIT_COUNT; i++) {
            _occlusionSplitFramebuffers[i] = gpu::FramebufferPointer(gpu::Framebuffer::create("occlusion"));
            _occlusionSplitFramebuffers[i]->setRenderBuffer(0, _occlusionSplitTexture, i);
        }
    }
#endif
}

#if SSAO_USE_QUAD_SPLIT
gpu::FramebufferPointer AmbientOcclusionFramebuffer::getOcclusionSplitFramebuffer(int index) {
    assert(index < SSAO_SPLIT_COUNT*SSAO_SPLIT_COUNT);
    if (!_occlusionSplitFramebuffers[index]) {
        allocate();
    }
    return _occlusionSplitFramebuffers[index];
}

gpu::TexturePointer AmbientOcclusionFramebuffer::getOcclusionSplitTexture() {
    if (!_occlusionSplitTexture) {
        allocate();
    }
    return _occlusionSplitTexture;
}
#endif

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

gpu::FramebufferPointer AmbientOcclusionFramebuffer::getNormalFramebuffer() {
    if (!_normalFramebuffer) {
        allocate();
    }
    return _normalFramebuffer;
}

gpu::TexturePointer AmbientOcclusionFramebuffer::getNormalTexture() {
    if (!_normalTexture) {
        allocate();
    }
    return _normalTexture;
}

AmbientOcclusionEffectConfig::AmbientOcclusionEffectConfig() :
    render::GPUJobConfig::Persistent(QStringList() << "Render" << "Engine" << "Ambient Occlusion", false),
    radius{ 0.3f },
    perspectiveScale{ 1.0f },
    obscuranceLevel{ 0.5f },
    falloffAngle{ 0.45f },
    edgeSharpness{ 1.0f },
    blurDeviation{ 2.5f },
    numSpiralTurns{ 7.0f },
    numSamples{ 1 },
    resolutionLevel{ 2 },
    blurRadius{ 4 },
    ditheringEnabled{ true },
    borderingEnabled{ true },
    fetchMipsEnabled{ true },
    horizonBased{ true } {
}

AmbientOcclusionEffect::AOParameters::AOParameters() {
    _resolutionInfo = { -1.0f, 1.0f, 1.0f, 0.0f };
    _radiusInfo = { 0.5f, 0.5f * 0.5f, 1.0f / (0.25f * 0.25f * 0.25f), 1.0f };
    _ditheringInfo = { 0.0f, 0.0f, 0.01f, 1.0f };
    _sampleInfo = { 11.0f, 1.0f / 11.0f, 7.0f, 1.0f };
}

AmbientOcclusionEffect::BlurParameters::BlurParameters() {
    _blurInfo = { 1.0f, 2.0f, 0.0f, 3.0f };
}

AmbientOcclusionEffect::AmbientOcclusionEffect() {
}

void AmbientOcclusionEffect::configure(const Config& config) {
    DependencyManager::get<DeferredLightingEffect>()->setAmbientOcclusionEnabled(config.enabled);

    bool shouldUpdateBlurs = false;

    const double RADIUS_POWER = 6.0;
    const auto& radius = config.radius;
    if (radius != _aoParametersBuffer->getRadius() || config.horizonBased != _aoParametersBuffer->isHorizonBased()) {
        auto& current = _aoParametersBuffer.edit()._radiusInfo;
        current.x = radius;
        current.y = radius * radius;
        current.z = 10.0f;
        if (!config.horizonBased) {
            current.z *= (float)(1.0 / pow((double)radius, RADIUS_POWER));
        }
    }

    if (config.horizonBased != _aoParametersBuffer->isHorizonBased()) {
        auto& current = _aoParametersBuffer.edit()._resolutionInfo;
        current.y = config.horizonBased & 1;
    }

    if (config.obscuranceLevel != _aoParametersBuffer->getObscuranceLevel()) {
        auto& current = _aoParametersBuffer.edit()._radiusInfo;
        current.w = config.obscuranceLevel;
    }

    if (config.falloffAngle != _aoParametersBuffer->getFalloffAngle()) {
        auto& current = _aoParametersBuffer.edit()._ditheringInfo;
        current.z = config.falloffAngle;
        current.y = 1.0f / (1.0f - config.falloffAngle);
    }

    // Update bilateral blur
    {
        const float BLUR_EDGE_DISTANCE_SCALE = float(10000 * SSAO_DEPTH_KEY_SCALE);
        const float BLUR_EDGE_NORMAL_SCALE = 2.0f;

        auto& hblur = _hblurParametersBuffer.edit()._blurInfo;
        auto& vblur = _vblurParametersBuffer.edit()._blurInfo;
        float blurRadialSigma = float(config.blurRadius) * 0.5f;
        float blurRadialScale = 1.0f / (2.0f*blurRadialSigma*blurRadialSigma);
        glm::vec3 blurScales = -glm::vec3(blurRadialScale, glm::vec2(BLUR_EDGE_DISTANCE_SCALE, BLUR_EDGE_NORMAL_SCALE) * config.edgeSharpness);

        hblur.x = blurScales.x;
        hblur.y = blurScales.y;
        hblur.z = blurScales.z;
        hblur.w = (float)config.blurRadius;

        vblur.x = blurScales.x;
        vblur.y = blurScales.y;
        vblur.z = blurScales.z;
        vblur.w = (float)config.blurRadius;
    }

    if (config.numSpiralTurns != _aoParametersBuffer->getNumSpiralTurns()) {
        auto& current = _aoParametersBuffer.edit()._sampleInfo;
        current.z = config.numSpiralTurns;
    }

    if (config.numSamples != _aoParametersBuffer->getNumSamples()) {
        auto& current = _aoParametersBuffer.edit()._sampleInfo;
        current.x = config.numSamples;
        current.y = 1.0f / config.numSamples;

        // Regenerate offsets
        const int B = 3;
        const float invB = 1.0f / (float)B;

        for (int i = 0; i < _randomSamples.size(); i++) {
            int index = i+1; // Indices start at 1, not 0
            float f = 1.0f;
            float r = 0.0f;

            while (index > 0) {
                f = f * invB;
                r = r + f * (float)(index % B);
                index = index / B;
            }
            _randomSamples[i] = r * 2.0f * M_PI / config.numSamples;
        }
    }

    if (config.fetchMipsEnabled != _aoParametersBuffer->isFetchMipsEnabled()) {
        auto& current = _aoParametersBuffer.edit()._sampleInfo;
        current.w = (float)config.fetchMipsEnabled;
    }

    if (!_framebuffer) {
        _framebuffer = std::make_shared<AmbientOcclusionFramebuffer>();
        shouldUpdateBlurs = true;
    }
    
    if (config.perspectiveScale != _aoParametersBuffer->getPerspectiveScale()) {
        _aoParametersBuffer.edit()._resolutionInfo.z = config.perspectiveScale;
    }

    if (config.resolutionLevel != _aoParametersBuffer->getResolutionLevel()) {
        auto& current = _aoParametersBuffer.edit()._resolutionInfo;
        current.x = (float)config.resolutionLevel;
        shouldUpdateBlurs = true;
    }

    if (config.ditheringEnabled != _aoParametersBuffer->isDitheringEnabled()) {
        auto& current = _aoParametersBuffer.edit()._ditheringInfo;
        current.x = (float)config.ditheringEnabled;
    }

    if (config.borderingEnabled != _aoParametersBuffer->isBorderingEnabled()) {
        auto& current = _aoParametersBuffer.edit()._ditheringInfo;
        current.w = (float)config.borderingEnabled;
    }

    if (shouldUpdateBlurs) {
        updateBlurParameters();
    }
}

void AmbientOcclusionEffect::updateBlurParameters() {
    const auto resolutionLevel = _aoParametersBuffer->getResolutionLevel();
    auto& vblur = _vblurParametersBuffer.edit();
    auto& hblur = _hblurParametersBuffer.edit();
    auto frameSize = _framebuffer->getSourceFrameSize();
    if (_framebuffer->isStereo()) {
        frameSize.x >>= 1;
    }
    const auto occlusionSize = frameSize >> resolutionLevel;

    // Occlusion UV limit
    hblur._blurAxis.z = occlusionSize.x / float(frameSize.x);
    hblur._blurAxis.w = occlusionSize.y / float(frameSize.y);

    vblur._blurAxis.z = 1.0f;
    vblur._blurAxis.w = occlusionSize.y / float(frameSize.y);

    // Occlusion axis
    hblur._blurAxis.x = hblur._blurAxis.z / occlusionSize.x;
    hblur._blurAxis.y = 0.0f;

    vblur._blurAxis.x = 0.0f;
    vblur._blurAxis.y = vblur._blurAxis.w / occlusionSize.y;
}

void AmbientOcclusionEffect::updateFramebufferSizes() {
    auto& params = _aoParametersBuffer.edit();
    const int stereoDivide = _framebuffer->isStereo() & 1;
    auto sourceFrameSideSize = _framebuffer->getSourceFrameSize();
    sourceFrameSideSize.x >>= stereoDivide;

    const int resolutionLevel = _aoParametersBuffer.get().getResolutionLevel();
    const int depthResolutionLevel = getDepthResolutionLevel();
    const auto occlusionDepthFrameSize = sourceFrameSideSize >> depthResolutionLevel;
    const auto occlusionFrameSize = sourceFrameSideSize >> resolutionLevel;
    auto normalTextureSize = _framebuffer->getNormalTexture()->getDimensions();

    normalTextureSize.x >>= stereoDivide;

    params._sideSizes[0].x = normalTextureSize.x;
    params._sideSizes[0].y = normalTextureSize.y;
    params._sideSizes[0].z = resolutionLevel;
    params._sideSizes[0].w = depthResolutionLevel;

    params._sideSizes[1].x = params._sideSizes[0].x;
    params._sideSizes[1].y = params._sideSizes[0].y;
    auto occlusionSplitSize = divideRoundUp(occlusionFrameSize, SSAO_SPLIT_COUNT);
    params._sideSizes[1].z = occlusionSplitSize.x;
    params._sideSizes[1].w = occlusionSplitSize.y;
}

const gpu::PipelinePointer& AmbientOcclusionEffect::getOcclusionPipeline() {
    if (!_occlusionPipeline) {
        gpu::ShaderPointer program = gpu::Shader::createProgram(shader::render_utils::program::ssao_makeOcclusion);
        gpu::StatePointer state = gpu::StatePointer(new gpu::State());

        state->setColorWriteMask(true, true, true, true);

        // Good to go add the brand new pipeline
        _occlusionPipeline = gpu::Pipeline::create(program, state);
    }
    return _occlusionPipeline;
}

const gpu::PipelinePointer& AmbientOcclusionEffect::getBilateralBlurPipeline() {
    if (!_bilateralBlurPipeline) {
        gpu::ShaderPointer program = gpu::Shader::createProgram(shader::render_utils::program::ssao_bilateralBlur);
        gpu::StatePointer state = gpu::StatePointer(new gpu::State());

        state->setColorWriteMask(true, true, true, false);
        
        // Good to go add the brand new pipeline
        _bilateralBlurPipeline = gpu::Pipeline::create(program, state);
    }
    return _bilateralBlurPipeline;
}

const gpu::PipelinePointer& AmbientOcclusionEffect::getMipCreationPipeline() {
	if (!_mipCreationPipeline) {
		_mipCreationPipeline = gpu::Context::createMipGenerationPipeline(gpu::Shader::createPixel(shader::render_utils::fragment::ssao_mip_depth));
	}
	return _mipCreationPipeline;
}

const gpu::PipelinePointer& AmbientOcclusionEffect::getGatherPipeline() {
    if (!_gatherPipeline) {
        gpu::ShaderPointer program = gpu::Shader::createProgram(shader::render_utils::program::ssao_gather);
        gpu::StatePointer state = gpu::StatePointer(new gpu::State());

        state->setColorWriteMask(true, true, true, true);

        // Good to go add the brand new pipeline
        _gatherPipeline = gpu::Pipeline::create(program, state);
    }
    return _gatherPipeline;
}

const gpu::PipelinePointer& AmbientOcclusionEffect::getBuildNormalsPipeline() {
    if (!_buildNormalsPipeline) {
        gpu::ShaderPointer program = gpu::Shader::createProgram(shader::render_utils::program::ssao_buildNormals);
        gpu::StatePointer state = gpu::StatePointer(new gpu::State());

        state->setColorWriteMask(true, true, true, true);

        // Good to go add the brand new pipeline
        _buildNormalsPipeline = gpu::Pipeline::create(program, state);
    }
    return _buildNormalsPipeline;
}

int AmbientOcclusionEffect::getDepthResolutionLevel() const {
    return std::min(1, _aoParametersBuffer->getResolutionLevel());
}

void AmbientOcclusionEffect::run(const render::RenderContextPointer& renderContext, const Inputs& inputs, Outputs& outputs) {
    assert(renderContext->args);
    assert(renderContext->args->hasViewFrustum());

    RenderArgs* args = renderContext->args;

    const auto& frameTransform = inputs.get0();
    const auto& linearDepthFramebuffer = inputs.get2();
    
    const int resolutionLevel = _aoParametersBuffer->getResolutionLevel();
    const auto resolutionScale = powf(0.5f, resolutionLevel);

    const auto depthResolutionLevel = getDepthResolutionLevel();
    const auto depthResolutionScale = powf(0.5f, depthResolutionLevel);
    const auto isHorizonBased = _aoParametersBuffer->isHorizonBased();

    auto fullResDepthTexture = linearDepthFramebuffer->getLinearDepthTexture();
    auto occlusionDepthTexture = fullResDepthTexture;
    auto sourceViewport = args->_viewport;
    auto occlusionViewport = sourceViewport >> resolutionLevel;
    auto firstBlurViewport = sourceViewport;
    firstBlurViewport.w = occlusionViewport.w;

    if (!_gpuTimer) {
        _gpuTimer = std::make_shared < gpu::RangeTimer>(__FUNCTION__);
    }

    if (!_framebuffer) {
        _framebuffer = std::make_shared<AmbientOcclusionFramebuffer>();
    }

    if (depthResolutionLevel > 0) {
        occlusionDepthTexture = linearDepthFramebuffer->getHalfLinearDepthTexture();
    }

    if (_framebuffer->update(fullResDepthTexture, resolutionLevel, depthResolutionLevel, args->isStereo())) {
        updateBlurParameters();
        updateFramebufferSizes();
    }
    
    auto occlusionFBO = _framebuffer->getOcclusionFramebuffer();
    auto occlusionBlurredFBO = _framebuffer->getOcclusionBlurredFramebuffer();
    
    outputs.edit0() = _framebuffer;
    outputs.edit1() = _aoParametersBuffer;

    auto framebufferSize = _framebuffer->getSourceFrameSize();
    auto occlusionPipeline = getOcclusionPipeline();
    auto bilateralBlurPipeline = getBilateralBlurPipeline();
    auto mipCreationPipeline = getMipCreationPipeline();
#if SSAO_USE_QUAD_SPLIT
    auto gatherPipeline = getGatherPipeline();
    auto buildNormalsPipeline = getBuildNormalsPipeline();
    auto occlusionNormalFramebuffer = _framebuffer->getNormalFramebuffer();
    auto occlusionNormalTexture = _framebuffer->getNormalTexture();
    auto normalViewport = glm::ivec4{ 0, 0, occlusionNormalFramebuffer->getWidth(), occlusionNormalFramebuffer->getHeight() };
    auto splitSize = glm::ivec2(_framebuffer->getOcclusionSplitTexture()->getDimensions());
    auto splitViewport = glm::ivec4{ 0, 0, splitSize.x, splitSize.y };
#endif
    auto occlusionDepthSize = glm::ivec2(occlusionDepthTexture->getDimensions());

    // Update sample rotation
    const int SSAO_RANDOM_SAMPLE_COUNT = int(_randomSamples.size() / (SSAO_SPLIT_COUNT*SSAO_SPLIT_COUNT));
    for (int splitId=0 ; splitId < SSAO_SPLIT_COUNT*SSAO_SPLIT_COUNT ; splitId++) {
        auto& sample = _aoFrameParametersBuffer[splitId].edit();
        sample._angleInfo.x = _randomSamples[splitId + SSAO_RANDOM_SAMPLE_COUNT * _frameId];
    }
    _frameId = (_frameId + 1) % SSAO_RANDOM_SAMPLE_COUNT;

    gpu::doInBatch("AmbientOcclusionEffect::run", args->_context, [=](gpu::Batch& batch) {
		PROFILE_RANGE_BATCH(batch, "SSAO");
		batch.enableStereo(false);

        _gpuTimer->begin(batch);

        batch.resetViewTransform();

        batch.setProjectionTransform(glm::mat4());
        batch.setModelTransform(Transform());

        // We need this with the mips levels
        batch.pushProfileRange("Depth Mips");
        if (isHorizonBased) {
            batch.setPipeline(mipCreationPipeline);
            batch.generateTextureMipsWithPipeline(occlusionDepthTexture);
        } else {
            batch.generateTextureMips(occlusionDepthTexture);
        }
        batch.popProfileRange();

#if SSAO_USE_QUAD_SPLIT
        batch.pushProfileRange("Normal Gen.");
        // Build face normals pass
        batch.setModelTransform(Transform());
        batch.setViewportTransform(normalViewport);
        batch.setPipeline(buildNormalsPipeline);
        batch.setResourceTexture(render_utils::slot::texture::SsaoDepth, fullResDepthTexture);
        batch.setResourceTexture(render_utils::slot::texture::SsaoNormal, nullptr);
        batch.setUniformBuffer(render_utils::slot::buffer::DeferredFrameTransform, frameTransform->getFrameTransformBuffer());
        batch.setUniformBuffer(render_utils::slot::buffer::SsaoParams, _aoParametersBuffer);
        batch.setFramebuffer(occlusionNormalFramebuffer);
        batch.draw(gpu::TRIANGLE_STRIP, 4);
        batch.popProfileRange();
#endif

        // Occlusion pass
        batch.pushProfileRange("Occlusion");

        batch.setUniformBuffer(render_utils::slot::buffer::DeferredFrameTransform, frameTransform->getFrameTransformBuffer());
        batch.setUniformBuffer(render_utils::slot::buffer::SsaoParams, _aoParametersBuffer);
        batch.setPipeline(occlusionPipeline);
        batch.setResourceTexture(render_utils::slot::texture::SsaoDepth, occlusionDepthTexture);

#if SSAO_USE_QUAD_SPLIT
        batch.setResourceTexture(render_utils::slot::texture::SsaoNormal, occlusionNormalTexture);
        {
            const auto uvScale = glm::vec3(
                (splitSize.x * SSAO_SPLIT_COUNT) / float(occlusionViewport.z),
                (splitSize.y * SSAO_SPLIT_COUNT) / float(occlusionViewport.w),
                1.0f);
            const auto postPixelOffset = glm::vec2(0.5f) / glm::vec2(occlusionViewport.z, occlusionViewport.w);
            const auto prePixelOffset = glm::vec2(0.5f * uvScale.x, 0.5f * uvScale.y) / glm::vec2(splitSize);
            Transform model;

            batch.setViewportTransform(splitViewport);

            model.setScale(uvScale);
            for (int y = 0; y < SSAO_SPLIT_COUNT; y++) {
                for (int x = 0; x < SSAO_SPLIT_COUNT; x++) {
                    const int splitIndex = x + y * SSAO_SPLIT_COUNT;
                    const auto uvTranslate = glm::vec3(
                        postPixelOffset.x * (2 * x + 1) - prePixelOffset.x,
                        postPixelOffset.y * (2 * y + 1) - prePixelOffset.y,
                        0.0f
                        );
                    model.setTranslation(uvTranslate);
                    batch.setModelTransform(model);
                    batch.setFramebuffer(_framebuffer->getOcclusionSplitFramebuffer(splitIndex));
                    batch.setUniformBuffer(render_utils::slot::buffer::SsaoFrameParams, _aoFrameParametersBuffer[splitIndex]);
                    batch.draw(gpu::TRIANGLE_STRIP, 4);
                }
            }
        }
#else
        batch.setViewportTransform(occlusionViewport);
        model.setIdentity();
        batch.setModelTransform(model);
        batch.setFramebuffer(occlusionFBO);
        batch.setUniformBuffer(render_utils::slot::buffer::SsaoFrameParams, _aoFrameParametersBuffer[0]);
        batch.draw(gpu::TRIANGLE_STRIP, 4);
#endif

        batch.popProfileRange();

#if SSAO_USE_QUAD_SPLIT
        // Gather back the four separate renders into one interleaved one
        batch.pushProfileRange("Gather");
        batch.setViewportTransform(occlusionViewport);
        batch.setModelTransform(Transform());
        batch.setFramebuffer(occlusionFBO);
        batch.setPipeline(gatherPipeline);
        batch.setResourceTexture(render_utils::slot::texture::SsaoOcclusion, _framebuffer->getOcclusionSplitTexture());
        batch.draw(gpu::TRIANGLE_STRIP, 4);
        batch.popProfileRange();
#endif

        {
            PROFILE_RANGE_BATCH(batch, "Bilateral Blur");
            // Blur 1st pass
            batch.pushProfileRange("Horiz.");
            {
                const auto uvScale = glm::vec3(
                    occlusionViewport.z / float(sourceViewport.z),
                    occlusionViewport.w / float(sourceViewport.w),
                    1.0f);
                Transform model;
                model.setScale(uvScale);
                batch.setModelTransform(model);
            }
            batch.setPipeline(bilateralBlurPipeline);
            // Use full resolution depth for bilateral upscaling and blur
            batch.setResourceTexture(render_utils::slot::texture::SsaoDepth, fullResDepthTexture);
#if SSAO_USE_QUAD_SPLIT
            batch.setResourceTexture(render_utils::slot::texture::SsaoNormal, occlusionNormalTexture);
#else
            batch.setResourceTexture(render_utils::slot::texture::SsaoNormal, nullptr);
#endif
            batch.setViewportTransform(firstBlurViewport);
            batch.setFramebuffer(occlusionBlurredFBO);
            batch.setUniformBuffer(render_utils::slot::buffer::SsaoBlurParams, _hblurParametersBuffer);
            batch.setResourceTexture(render_utils::slot::texture::SsaoOcclusion, occlusionFBO->getRenderBuffer(0));
            batch.draw(gpu::TRIANGLE_STRIP, 4);
            batch.popProfileRange();

            // Blur 2nd pass
            batch.pushProfileRange("Vert.");
            {
                const auto uvScale = glm::vec3(
                    1.0f,
                    occlusionViewport.w / float(sourceViewport.w),
                    1.0f);

                Transform model;
                model.setScale(uvScale);
                batch.setModelTransform(model);
            }
            batch.setViewportTransform(sourceViewport);
            batch.setFramebuffer(occlusionFBO);
            batch.setUniformBuffer(render_utils::slot::buffer::SsaoBlurParams, _vblurParametersBuffer);
            batch.setResourceTexture(render_utils::slot::texture::SsaoOcclusion, occlusionBlurredFBO->getRenderBuffer(0));
            batch.draw(gpu::TRIANGLE_STRIP, 4);
            batch.popProfileRange();
        }

        batch.setResourceTexture(render_utils::slot::texture::SsaoDepth, nullptr);
        batch.setResourceTexture(render_utils::slot::texture::SsaoNormal, nullptr);
        batch.setResourceTexture(render_utils::slot::texture::SsaoOcclusion, nullptr);

        _gpuTimer->end(batch);
    });

    // Update the timer
    auto config = std::static_pointer_cast<Config>(renderContext->jobConfig);
    config->setGPUBatchRunTime(_gpuTimer->getGPUAverage(), _gpuTimer->getBatchAverage());
}

DebugAmbientOcclusion::DebugAmbientOcclusion() {
}

void DebugAmbientOcclusion::configure(const Config& config) {

    _showCursorPixel = config.showCursorPixel;

    auto cursorPos = glm::vec2(_parametersBuffer->pixelInfo);
    if (cursorPos != config.debugCursorTexcoord) {
        _parametersBuffer.edit().pixelInfo = glm::vec4(config.debugCursorTexcoord, 0.0f, 0.0f);
    }
}

const gpu::PipelinePointer& DebugAmbientOcclusion::getDebugPipeline() {
    if (!_debugPipeline) {
        gpu::ShaderPointer program = gpu::Shader::createProgram(shader::render_utils::program::ssao_debugOcclusion);
        gpu::StatePointer state = gpu::StatePointer(new gpu::State());

        state->setColorWriteMask(true, true, true, false);
        state->setBlendFunction(true, gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA);
        // Good to go add the brand new pipeline
        _debugPipeline = gpu::Pipeline::create(program, state);
    }
    return _debugPipeline;
}

void DebugAmbientOcclusion::run(const render::RenderContextPointer& renderContext, const Inputs& inputs) {
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

    auto fullResDepthTexture = linearDepthFramebuffer->getLinearDepthTexture();
    auto sourceViewport = args->_viewport;
    auto occlusionViewport = sourceViewport;

    auto resolutionLevel = ambientOcclusionUniforms->getResolutionLevel();
    
    if (resolutionLevel > 0) {
        fullResDepthTexture = linearDepthFramebuffer->getHalfLinearDepthTexture();
        occlusionViewport = occlusionViewport >> ambientOcclusionUniforms->getResolutionLevel();
    }

    auto framebufferSize = glm::ivec2(fullResDepthTexture->getDimensions());
    
    float sMin = occlusionViewport.x / (float)framebufferSize.x;
    float sWidth = occlusionViewport.z / (float)framebufferSize.x;
    float tMin = occlusionViewport.y / (float)framebufferSize.y;
    float tHeight = occlusionViewport.w / (float)framebufferSize.y;
    
    auto debugPipeline = getDebugPipeline();
    
    gpu::doInBatch("DebugAmbientOcclusion::run", args->_context, [=](gpu::Batch& batch) {
        batch.enableStereo(false);

        batch.setViewportTransform(sourceViewport);
        batch.setProjectionTransform(glm::mat4());
        batch.setViewTransform(Transform());

        Transform model;
        model.setTranslation(glm::vec3(sMin, tMin, 0.0f));
        model.setScale(glm::vec3(sWidth, tHeight, 1.0f));
        batch.setModelTransform(model);

        batch.setUniformBuffer(render_utils::slot::buffer::DeferredFrameTransform, frameTransform->getFrameTransformBuffer());
        batch.setUniformBuffer(render_utils::slot::buffer::SsaoParams, ambientOcclusionUniforms);
        batch.setUniformBuffer(render_utils::slot::buffer::SsaoDebugParams, _parametersBuffer);
        
        batch.setPipeline(debugPipeline);
        batch.setResourceTexture(render_utils::slot::texture::SsaoDepth, fullResDepthTexture);
        batch.draw(gpu::TRIANGLE_STRIP, 4);

        
        batch.setResourceTexture(render_utils::slot::texture::SsaoDepth, nullptr);
    });

}
 
