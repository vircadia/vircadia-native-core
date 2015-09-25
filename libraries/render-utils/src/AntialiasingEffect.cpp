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

#include "gpu/StandardShaderLib.h"
#include "AntialiasingEffect.h"
#include "TextureCache.h"
#include "FramebufferCache.h"
#include "DependencyManager.h"
#include "ViewFrustum.h"
#include "GeometryCache.h"

#include "fxaa_vert.h"
#include "fxaa_frag.h"
#include "fxaa_blend_frag.h"


Antialiasing::Antialiasing() {
}

const gpu::PipelinePointer& Antialiasing::getAntialiasingPipeline() {
    if (!_antialiasingPipeline) {
        auto vs = gpu::ShaderPointer(gpu::Shader::createVertex(std::string(fxaa_vert)));
        auto ps = gpu::ShaderPointer(gpu::Shader::createPixel(std::string(fxaa_frag)));
        gpu::ShaderPointer program = gpu::ShaderPointer(gpu::Shader::createProgram(vs, ps));

        gpu::Shader::BindingSet slotBindings;
        slotBindings.insert(gpu::Shader::Binding(std::string("colorTexture"), 0));

        gpu::Shader::makeProgram(*program, slotBindings);

        _texcoordOffsetLoc = program->getUniforms().findLocation("texcoordOffset");

        gpu::StatePointer state = gpu::StatePointer(new gpu::State());

        state->setDepthTest(false, false, gpu::LESS_EQUAL);

        // Link the antialiasing FBO to texture
        _antialiasingBuffer = gpu::FramebufferPointer(gpu::Framebuffer::create(gpu::Element::COLOR_RGBA_32,
          DependencyManager::get<FramebufferCache>()->getFrameBufferSize().width(), DependencyManager::get<FramebufferCache>()->getFrameBufferSize().height()));
        auto format = gpu::Element(gpu::VEC4, gpu::NUINT8, gpu::RGBA);
        auto width = _antialiasingBuffer->getWidth();
        auto height = _antialiasingBuffer->getHeight();
        auto defaultSampler = gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_POINT);
        _antialiasingTexture = gpu::TexturePointer(gpu::Texture::create2D(format, width, height, defaultSampler));

        // Good to go add the brand new pipeline
        _antialiasingPipeline.reset(gpu::Pipeline::create(program, state));
    }

    int w = DependencyManager::get<FramebufferCache>()->getFrameBufferSize().width();
    int h = DependencyManager::get<FramebufferCache>()->getFrameBufferSize().height();
    if (w != _antialiasingBuffer->getWidth() || h != _antialiasingBuffer->getHeight()) {
        _antialiasingBuffer->resize(w, h);
    }

    return _antialiasingPipeline;
}

const gpu::PipelinePointer& Antialiasing::getBlendPipeline() {
    if (!_blendPipeline) {
        auto vs = gpu::ShaderPointer(gpu::Shader::createVertex(std::string(fxaa_vert)));
        auto ps = gpu::ShaderPointer(gpu::Shader::createPixel(std::string(fxaa_blend_frag)));
        gpu::ShaderPointer program = gpu::ShaderPointer(gpu::Shader::createProgram(vs, ps));

        gpu::Shader::BindingSet slotBindings;
        slotBindings.insert(gpu::Shader::Binding(std::string("colorTexture"), 0));

        gpu::Shader::makeProgram(*program, slotBindings);

        gpu::StatePointer state = gpu::StatePointer(new gpu::State());

        state->setDepthTest(false, false, gpu::LESS_EQUAL);

        // Good to go add the brand new pipeline
        _blendPipeline.reset(gpu::Pipeline::create(program, state));
    }
    return _blendPipeline;
}

void Antialiasing::run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext) {
    assert(renderContext->args);
    assert(renderContext->args->_viewFrustum);

    if (renderContext->args->_renderMode == RenderArgs::MIRROR_RENDER_MODE) {
        return;
    }

    RenderArgs* args = renderContext->args;
    gpu::doInBatch(args->_context, [=](gpu::Batch& batch) {
        batch.enableStereo(false);

        auto framebufferCache = DependencyManager::get<FramebufferCache>();
        QSize framebufferSize = framebufferCache->getFrameBufferSize();
        float fbWidth = framebufferSize.width();
        float fbHeight = framebufferSize.height();
        // float sMin = args->_viewport.x / fbWidth;
        // float sWidth = args->_viewport.z / fbWidth;
        // float tMin = args->_viewport.y / fbHeight;
        // float tHeight = args->_viewport.w / fbHeight;

        glm::mat4 projMat;
        Transform viewMat;
        args->_viewFrustum->evalProjectionMatrix(projMat);
        args->_viewFrustum->evalViewTransform(viewMat);
        batch.setProjectionTransform(projMat);
        batch.setViewTransform(viewMat);
        batch.setModelTransform(Transform());

        // FXAA step
        getAntialiasingPipeline();
        batch.setResourceTexture(0, framebufferCache->getPrimaryColorTexture());
        _antialiasingBuffer->setRenderBuffer(0, _antialiasingTexture);
        batch.setFramebuffer(_antialiasingBuffer);
        batch.setPipeline(getAntialiasingPipeline());

        // initialize the view-space unpacking uniforms using frustum data
        float left, right, bottom, top, nearVal, farVal;
        glm::vec4 nearClipPlane, farClipPlane;

        args->_viewFrustum->computeOffAxisFrustum(left, right, bottom, top, nearVal, farVal, nearClipPlane, farClipPlane);

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
        DependencyManager::get<GeometryCache>()->renderQuad(batch, bottomLeft, topRight, texCoordTopLeft, texCoordBottomRight, color);

        // Blend step
        getBlendPipeline();
        batch.setResourceTexture(0, _antialiasingTexture);
        batch.setFramebuffer(framebufferCache->getPrimaryFramebuffer());
        batch.setPipeline(getBlendPipeline());

        DependencyManager::get<GeometryCache>()->renderQuad(batch, bottomLeft, topRight, texCoordTopLeft, texCoordBottomRight, color);
    });
}
