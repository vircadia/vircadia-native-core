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



const int AmbientOcclusionEffect_ParamsSlot = 0;
const int AmbientOcclusionEffect_DeferredTransformSlot = 1;
const int AmbientOcclusionEffect_DepthMapSlot = 0;
const int AmbientOcclusionEffect_PyramidMapSlot = 0;

AmbientOcclusionEffect::AmbientOcclusionEffect() {
    Parameters parameters;
    _parametersBuffer = gpu::BufferView(std::make_shared<gpu::Buffer>(sizeof(Parameters), (const gpu::Byte*) &parameters));
}

const gpu::PipelinePointer& AmbientOcclusionEffect::getPyramidPipeline() {
    if (!_pyramidPipeline) {
        const char AO_frag[] = R"SCRIBE(#version 410 core
        //
        //  Created by Sam Gateau on 12/23/2015
        //  Copyright 2015 High Fidelity, Inc.
        //
        //  Distributed under the Apache License, Version 2.0.
        //  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
        //

        struct AmbientOcclusionParams {
            vec4 _clipInfo;
            mat4 _projection;
        };

        uniform ambientOcclusionParamsBuffer {
            AmbientOcclusionParams params;
        };

        float evalZeyeFromZdb(float depth) {
            return params._clipInfo.x / (depth * params._clipInfo.y + params._clipInfo.z);
        }

        // the depth texture
        uniform sampler2D depthMap;

        in vec2 varTexCoord0;
        out vec4 outFragColor;
        
        void main(void) {
            float Zdb = texture(depthMap, varTexCoord0).x;
            float Zeye = -evalZeyeFromZdb(Zdb);
            outFragColor = vec4(Zeye, 0.0, 0.0, 1.0);
        }

        )SCRIBE";
        auto vs = gpu::StandardShaderLib::getDrawViewportQuadTransformTexcoordVS();
        auto ps = gpu::Shader::createPixel(std::string(AO_frag));
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
        const char AO_frag[] = R"SCRIBE(#version 410 core
        //
        //  Created by Sam Gateau on 12/23/2015
        //  Copyright 2015 High Fidelity, Inc.
        //
        //  Distributed under the Apache License, Version 2.0.
        //  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
        //

        struct AmbientOcclusionParams {
            vec4 _clipInfo;
            mat4 _projection;
        };

        uniform ambientOcclusionParamsBuffer {
            AmbientOcclusionParams params;
        };

        vec3 evalEyePositionFromZeye(float Zeye, vec2 texcoord, mat4 projection) {
            // compute the view space position using the depth
            // basically manually pick the proj matrix components to do the inverse
            float Xe = (-Zeye * (texcoord.x * 2.0 - 1.0) - Zeye * projection[2][0] - projection[3][0]) / projection[0][0];
            float Ye = (-Zeye * (texcoord.y * 2.0 - 1.0) - Zeye * projection[2][1] - projection[3][1]) / projection[1][1];
            return vec3(Xe, Ye, Zeye);
        }

        // the depth texture
        uniform sampler2D pyramidMap;

        in vec2 varTexCoord0;
        out vec4 outFragColor;

        void main(void) {
            float Zeye = textureLod(pyramidMap, varTexCoord0, 0).x;
            vec3 P = evalEyePositionFromZeye(-Zeye, varTexCoord0, params._projection);

            outFragColor = vec4(1.0, 0.0, 0.0, P.x / 100.0);
        }
        
        )SCRIBE";
        auto vs = gpu::StandardShaderLib::getDrawViewportQuadTransformTexcoordVS();
        auto ps = gpu::Shader::createPixel(std::string(AO_frag));
        gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);

        gpu::Shader::BindingSet slotBindings;
        slotBindings.insert(gpu::Shader::Binding(std::string("ambientOcclusionParamsBuffer"), AmbientOcclusionEffect_ParamsSlot));
        slotBindings.insert(gpu::Shader::Binding(std::string("deferredTransformBuffer"), AmbientOcclusionEffect_DeferredTransformSlot));
        slotBindings.insert(gpu::Shader::Binding(std::string("pyramidMap"), AmbientOcclusionEffect_PyramidMapSlot));

        gpu::Shader::makeProgram(*program, slotBindings);


        gpu::StatePointer state = gpu::StatePointer(new gpu::State());

        // Stencil test all the ao passes for objects pixels only, not the background
        state->setStencilTest(true, 0xFF, gpu::State::StencilTest(0, 0xFF, gpu::NOT_EQUAL, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP));

        state->setColorWriteMask(false, false, false, true);

        // Good to go add the brand new pipeline
        _occlusionPipeline = gpu::Pipeline::create(program, state);
    }
    return _occlusionPipeline;
}

const gpu::PipelinePointer& AmbientOcclusionEffect::getVBlurPipeline() {
    if (!_vBlurPipeline) {
        auto vs = gpu::Shader::createVertex(std::string(gaussian_blur_vertical_vert));
        auto ps = gpu::Shader::createPixel(std::string(gaussian_blur_frag));
        gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);

        gpu::Shader::BindingSet slotBindings;
        gpu::Shader::makeProgram(*program, slotBindings);

        gpu::StatePointer state = gpu::StatePointer(new gpu::State());

        state->setDepthTest(false, false, gpu::LESS_EQUAL);

        // Blend on transparent
        state->setBlendFunction(false,
            gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA,
            gpu::State::DEST_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::ZERO);

        // Link the horizontal blur FBO to texture
    /*    _vBlurBuffer = gpu::FramebufferPointer(gpu::Framebuffer::create(gpu::Element::COLOR_RGBA_32,
            DependencyManager::get<FramebufferCache>()->getFrameBufferSize().width(), DependencyManager::get<FramebufferCache>()->getFrameBufferSize().height()));
        auto format = gpu::Element(gpu::VEC4, gpu::NUINT8, gpu::RGBA);
        auto width = _vBlurBuffer->getWidth();
        auto height = _vBlurBuffer->getHeight();
        auto defaultSampler = gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_POINT);
        _vBlurTexture = gpu::TexturePointer(gpu::Texture::create2D(format, width, height, defaultSampler));
        */
        // Good to go add the brand new pipeline
        _vBlurPipeline = gpu::Pipeline::create(program, state);
    }
    return _vBlurPipeline;
}

const gpu::PipelinePointer& AmbientOcclusionEffect::getHBlurPipeline() {
    if (!_hBlurPipeline) {
        auto vs = gpu::Shader::createVertex(std::string(gaussian_blur_horizontal_vert));
        auto ps = gpu::Shader::createPixel(std::string(gaussian_blur_frag));
        gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);

        gpu::Shader::BindingSet slotBindings;
        gpu::Shader::makeProgram(*program, slotBindings);

        gpu::StatePointer state = gpu::StatePointer(new gpu::State());

        state->setDepthTest(false, false, gpu::LESS_EQUAL);

        // Blend on transparent
        state->setBlendFunction(false,
            gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA,
            gpu::State::DEST_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::ZERO);

    /*    // Link the horizontal blur FBO to texture
        _hBlurBuffer = gpu::FramebufferPointer(gpu::Framebuffer::create(gpu::Element::COLOR_RGBA_32,
            DependencyManager::get<FramebufferCache>()->getFrameBufferSize().width(), DependencyManager::get<FramebufferCache>()->getFrameBufferSize().height()));
        auto format = gpu::Element(gpu::VEC4, gpu::NUINT8, gpu::RGBA);
        auto width = _hBlurBuffer->getWidth();
        auto height = _hBlurBuffer->getHeight();
        auto defaultSampler = gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_POINT);
        _hBlurTexture = gpu::TexturePointer(gpu::Texture::create2D(format, width, height, defaultSampler));
        */
        // Good to go add the brand new pipeline
        _hBlurPipeline = gpu::Pipeline::create(program, state);
    }
    return _hBlurPipeline;
}

void AmbientOcclusionEffect::setClipInfo(float nearZ, float farZ) {
    _parametersBuffer.edit<Parameters>()._clipInfo = glm::vec4(nearZ*farZ, farZ -nearZ, -farZ, 0.0f);

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
    auto pyramidFBO = framebufferCache->getDepthPyramidFramebuffer();
    auto occlusionFBO = framebufferCache->getDeferredFramebufferDepthColor();

    QSize framebufferSize = framebufferCache->getFrameBufferSize();
    float sMin = args->_viewport.x / (float)framebufferSize.width();
    float sWidth = args->_viewport.z / (float)framebufferSize.width();
    float tMin = args->_viewport.y / (float)framebufferSize.height();
    float tHeight = args->_viewport.w / (float)framebufferSize.height();


    updateDeferredTransformBuffer(renderContext);

    // Eval the mono projection
    mat4 monoProjMat;
    args->_viewFrustum->evalProjectionMatrix(monoProjMat);

    setClipInfo(args->_viewFrustum->getNearClip(), args->_viewFrustum->getFarClip());
    _parametersBuffer.edit<Parameters>()._projection = monoProjMat;


    auto pyramidPipeline = getPyramidPipeline();
    auto occlusionPipeline = getOcclusionPipeline();

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
        batch.setPipeline(pyramidPipeline);
        batch.setResourceTexture(AmbientOcclusionEffect_DepthMapSlot, depthBuffer);
        batch.draw(gpu::TRIANGLE_STRIP, 4);

        // 
        batch.setFramebuffer(occlusionFBO);
        batch.generateTextureMips(pyramidFBO->getRenderBuffer(0));

        // Occlusion pass
        batch.setPipeline(occlusionPipeline);
        batch.setResourceTexture(AmbientOcclusionEffect_PyramidMapSlot, pyramidFBO->getRenderBuffer(0));
        batch.draw(gpu::TRIANGLE_STRIP, 4);

    });
}
