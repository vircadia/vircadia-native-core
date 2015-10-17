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

#include "ambient_occlusion_vert.h"
#include "ambient_occlusion_frag.h"
#include "gaussian_blur_vertical_vert.h"
#include "gaussian_blur_horizontal_vert.h"
#include "gaussian_blur_frag.h"
#include "occlusion_blend_frag.h"


AmbientOcclusion::AmbientOcclusion() {
}

const gpu::PipelinePointer& AmbientOcclusion::getOcclusionPipeline() {
    if (!_occlusionPipeline) {
        auto vs = gpu::ShaderPointer(gpu::Shader::createVertex(std::string(ambient_occlusion_vert)));
        auto ps = gpu::ShaderPointer(gpu::Shader::createPixel(std::string(ambient_occlusion_frag)));
        gpu::ShaderPointer program = gpu::ShaderPointer(gpu::Shader::createProgram(vs, ps));

        gpu::Shader::BindingSet slotBindings;
        slotBindings.insert(gpu::Shader::Binding(std::string("depthTexture"), 0));
        slotBindings.insert(gpu::Shader::Binding(std::string("normalTexture"), 1));

        gpu::Shader::makeProgram(*program, slotBindings);

        _gScaleLoc = program->getUniforms().findLocation("g_scale");
        _gBiasLoc = program->getUniforms().findLocation("g_bias");
        _gSampleRadiusLoc = program->getUniforms().findLocation("g_sample_rad");
        _gIntensityLoc = program->getUniforms().findLocation("g_intensity");

        _nearLoc = program->getUniforms().findLocation("near");
        _depthScaleLoc = program->getUniforms().findLocation("depthScale");
        _depthTexCoordOffsetLoc = program->getUniforms().findLocation("depthTexCoordOffset");
        _depthTexCoordScaleLoc = program->getUniforms().findLocation("depthTexCoordScale");
        _renderTargetResLoc = program->getUniforms().findLocation("renderTargetRes");
        _renderTargetResInvLoc = program->getUniforms().findLocation("renderTargetResInv");

        gpu::StatePointer state = gpu::StatePointer(new gpu::State());

        state->setDepthTest(false, false, gpu::LESS_EQUAL);

        // Blend on transparent
        state->setBlendFunction(false,
            gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA,
            gpu::State::DEST_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::ZERO);

        // Link the occlusion FBO to texture
        _occlusionBuffer = gpu::FramebufferPointer(gpu::Framebuffer::create(gpu::Element::COLOR_RGBA_32,
            DependencyManager::get<FramebufferCache>()->getFrameBufferSize().width(), DependencyManager::get<FramebufferCache>()->getFrameBufferSize().height()));
        auto format = gpu::Element(gpu::VEC4, gpu::NUINT8, gpu::RGBA);
        auto width = _occlusionBuffer->getWidth();
        auto height = _occlusionBuffer->getHeight();
        auto defaultSampler = gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_POINT);
        _occlusionTexture = gpu::TexturePointer(gpu::Texture::create2D(format, width, height, defaultSampler));

        // Good to go add the brand new pipeline
        _occlusionPipeline.reset(gpu::Pipeline::create(program, state));
    }
    return _occlusionPipeline;
}

const gpu::PipelinePointer& AmbientOcclusion::getVBlurPipeline() {
    if (!_vBlurPipeline) {
        auto vs = gpu::ShaderPointer(gpu::Shader::createVertex(std::string(gaussian_blur_vertical_vert)));
        auto ps = gpu::ShaderPointer(gpu::Shader::createPixel(std::string(gaussian_blur_frag)));
        gpu::ShaderPointer program = gpu::ShaderPointer(gpu::Shader::createProgram(vs, ps));

        gpu::Shader::BindingSet slotBindings;
        gpu::Shader::makeProgram(*program, slotBindings);

        gpu::StatePointer state = gpu::StatePointer(new gpu::State());

        state->setDepthTest(false, false, gpu::LESS_EQUAL);

        // Blend on transparent
        state->setBlendFunction(false,
            gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA,
            gpu::State::DEST_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::ZERO);

        // Link the horizontal blur FBO to texture
        _vBlurBuffer = gpu::FramebufferPointer(gpu::Framebuffer::create(gpu::Element::COLOR_RGBA_32,
            DependencyManager::get<FramebufferCache>()->getFrameBufferSize().width(), DependencyManager::get<FramebufferCache>()->getFrameBufferSize().height()));
        auto format = gpu::Element(gpu::VEC4, gpu::NUINT8, gpu::RGBA);
        auto width = _vBlurBuffer->getWidth();
        auto height = _vBlurBuffer->getHeight();
        auto defaultSampler = gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_POINT);
        _vBlurTexture = gpu::TexturePointer(gpu::Texture::create2D(format, width, height, defaultSampler));

        // Good to go add the brand new pipeline
        _vBlurPipeline.reset(gpu::Pipeline::create(program, state));
    }
    return _vBlurPipeline;
}

const gpu::PipelinePointer& AmbientOcclusion::getHBlurPipeline() {
    if (!_hBlurPipeline) {
        auto vs = gpu::ShaderPointer(gpu::Shader::createVertex(std::string(gaussian_blur_horizontal_vert)));
        auto ps = gpu::ShaderPointer(gpu::Shader::createPixel(std::string(gaussian_blur_frag)));
        gpu::ShaderPointer program = gpu::ShaderPointer(gpu::Shader::createProgram(vs, ps));

        gpu::Shader::BindingSet slotBindings;
        gpu::Shader::makeProgram(*program, slotBindings);

        gpu::StatePointer state = gpu::StatePointer(new gpu::State());

        state->setDepthTest(false, false, gpu::LESS_EQUAL);

        // Blend on transparent
        state->setBlendFunction(false,
            gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA,
            gpu::State::DEST_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::ZERO);

        // Link the horizontal blur FBO to texture
        _hBlurBuffer = gpu::FramebufferPointer(gpu::Framebuffer::create(gpu::Element::COLOR_RGBA_32,
            DependencyManager::get<FramebufferCache>()->getFrameBufferSize().width(), DependencyManager::get<FramebufferCache>()->getFrameBufferSize().height()));
        auto format = gpu::Element(gpu::VEC4, gpu::NUINT8, gpu::RGBA);
        auto width = _hBlurBuffer->getWidth();
        auto height = _hBlurBuffer->getHeight();
        auto defaultSampler = gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_POINT);
        _hBlurTexture = gpu::TexturePointer(gpu::Texture::create2D(format, width, height, defaultSampler));

        // Good to go add the brand new pipeline
        _hBlurPipeline.reset(gpu::Pipeline::create(program, state));
    }
    return _hBlurPipeline;
}

const gpu::PipelinePointer& AmbientOcclusion::getBlendPipeline() {
    if (!_blendPipeline) {
        auto vs = gpu::ShaderPointer(gpu::Shader::createVertex(std::string(ambient_occlusion_vert)));
        auto ps = gpu::ShaderPointer(gpu::Shader::createPixel(std::string(occlusion_blend_frag)));
        gpu::ShaderPointer program = gpu::ShaderPointer(gpu::Shader::createProgram(vs, ps));

        gpu::Shader::BindingSet slotBindings;
        slotBindings.insert(gpu::Shader::Binding(std::string("blurredOcclusionTexture"), 0));

        gpu::Shader::makeProgram(*program, slotBindings);

        gpu::StatePointer state = gpu::StatePointer(new gpu::State());

        state->setDepthTest(false, false, gpu::LESS_EQUAL);

        // Blend on transparent
        state->setBlendFunction(true,
            gpu::State::INV_SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::SRC_ALPHA);

        // Good to go add the brand new pipeline
        _blendPipeline.reset(gpu::Pipeline::create(program, state));
    }
    return _blendPipeline;
}

void AmbientOcclusion::run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext) {
    assert(renderContext->args);
    assert(renderContext->args->_viewFrustum);

    RenderArgs* args = renderContext->args;
    gpu::doInBatch(args->_context, [=](gpu::Batch& batch) {
        auto framebufferCache = DependencyManager::get<FramebufferCache>();
        QSize framebufferSize = framebufferCache->getFrameBufferSize();
        float fbWidth = framebufferSize.width();
        float fbHeight = framebufferSize.height();
        float sMin = args->_viewport.x / fbWidth;
        float sWidth = args->_viewport.z / fbWidth;
        float tMin = args->_viewport.y / fbHeight;
        float tHeight = args->_viewport.w / fbHeight;


        glm::mat4 projMat;
        Transform viewMat;
        args->_viewFrustum->evalProjectionMatrix(projMat);
        args->_viewFrustum->evalViewTransform(viewMat);
        batch.setProjectionTransform(projMat);
        batch.setViewTransform(viewMat);
        batch.setModelTransform(Transform());

        // Occlusion step
        getOcclusionPipeline();
        batch.setResourceTexture(0, framebufferCache->getPrimaryDepthTexture());
        batch.setResourceTexture(1, framebufferCache->getPrimaryNormalTexture());
        _occlusionBuffer->setRenderBuffer(0, _occlusionTexture);
        batch.setFramebuffer(_occlusionBuffer);

        // Occlusion uniforms
        g_scale = 1.0f;
        g_bias = 1.0f;
        g_sample_rad = 1.0f;
        g_intensity = 1.0f;

        // Bind the first gpu::Pipeline we need - for calculating occlusion buffer
        batch.setPipeline(getOcclusionPipeline());
        batch._glUniform1f(_gScaleLoc, g_scale);
        batch._glUniform1f(_gBiasLoc, g_bias);
        batch._glUniform1f(_gSampleRadiusLoc, g_sample_rad);
        batch._glUniform1f(_gIntensityLoc, g_intensity);

        // setup uniforms for unpacking a view-space position from the depth buffer
        // This is code taken from DeferredLightEffect.render() method in DeferredLightingEffect.cpp.
        // DeferredBuffer.slh shows how the unpacking is done and what variables are needed.

        // initialize the view-space unpacking uniforms using frustum data
        float left, right, bottom, top, nearVal, farVal;
        glm::vec4 nearClipPlane, farClipPlane;

        args->_viewFrustum->computeOffAxisFrustum(left, right, bottom, top, nearVal, farVal, nearClipPlane, farClipPlane);

        float depthScale = (farVal - nearVal) / farVal;
        float nearScale = -1.0f / nearVal;
        float depthTexCoordScaleS = (right - left) * nearScale / sWidth;
        float depthTexCoordScaleT = (top - bottom) * nearScale / tHeight;
        float depthTexCoordOffsetS = left * nearScale - sMin * depthTexCoordScaleS;
        float depthTexCoordOffsetT = bottom * nearScale - tMin * depthTexCoordScaleT;

        // now set the position-unpacking unforms
        batch._glUniform1f(_nearLoc, nearVal);
        batch._glUniform1f(_depthScaleLoc, depthScale);
        batch._glUniform2f(_depthTexCoordOffsetLoc, depthTexCoordOffsetS, depthTexCoordOffsetT);
        batch._glUniform2f(_depthTexCoordScaleLoc, depthTexCoordScaleS, depthTexCoordScaleT);

        batch._glUniform2f(_renderTargetResLoc, fbWidth, fbHeight);
        batch._glUniform2f(_renderTargetResInvLoc, 1.0f / fbWidth, 1.0f / fbHeight);

        glm::vec4 color(0.0f, 0.0f, 0.0f, 1.0f);
        glm::vec2 bottomLeft(-1.0f, -1.0f);
        glm::vec2 topRight(1.0f, 1.0f);
        glm::vec2 texCoordTopLeft(0.0f, 0.0f);
        glm::vec2 texCoordBottomRight(1.0f, 1.0f);
        DependencyManager::get<GeometryCache>()->renderQuad(batch, bottomLeft, topRight, texCoordTopLeft, texCoordBottomRight, color);

        // Vertical blur step
        getVBlurPipeline();
        batch.setResourceTexture(0, _occlusionTexture);
        _vBlurBuffer->setRenderBuffer(0, _vBlurTexture);
        batch.setFramebuffer(_vBlurBuffer);

        // Bind the second gpu::Pipeline we need - for calculating blur buffer
        batch.setPipeline(getVBlurPipeline());

        DependencyManager::get<GeometryCache>()->renderQuad(batch, bottomLeft, topRight, texCoordTopLeft, texCoordBottomRight, color);

        // Horizontal blur step
        getHBlurPipeline();
        batch.setResourceTexture(0, _vBlurTexture);
        _hBlurBuffer->setRenderBuffer(0, _hBlurTexture);
        batch.setFramebuffer(_hBlurBuffer);

        // Bind the third gpu::Pipeline we need - for calculating blur buffer
        batch.setPipeline(getHBlurPipeline());

        DependencyManager::get<GeometryCache>()->renderQuad(batch, bottomLeft, topRight, texCoordTopLeft, texCoordBottomRight, color);

        // Blend step
        getBlendPipeline();
        batch.setResourceTexture(0, _hBlurTexture);
        batch.setFramebuffer(framebufferCache->getPrimaryFramebuffer());

        // Bind the fourth gpu::Pipeline we need - for blending the primary color buffer with blurred occlusion texture
        batch.setPipeline(getBlendPipeline());

        DependencyManager::get<GeometryCache>()->renderQuad(batch, bottomLeft, topRight, texCoordTopLeft, texCoordBottomRight, color);
    });
}
