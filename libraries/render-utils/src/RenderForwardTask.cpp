
//
//  RenderForwardTask.cpp
//  render-utils/src/
//
//  Created by Zach Pomerantz on 12/13/2016.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "RenderForwardTask.h"
#include "RenderDeferredTask.h"

#include <PerfStat.h>
#include <PathUtils.h>
#include <RenderArgs.h>
#include <ViewFrustum.h>
#include <gpu/Context.h>

#include "FramebufferCache.h"
#include "TextureCache.h"

#include <gpu/StandardShaderLib.h>

#include <render/drawItemBounds_vert.h>
#include <render/drawItemBounds_frag.h>

using namespace render;
extern void initOverlay3DPipelines(render::ShapePlumber& plumber);
extern void initDeferredPipelines(render::ShapePlumber& plumber);

RenderForwardTask::RenderForwardTask(RenderFetchSortCullTask::Output items) {
    // Prepare the ShapePipelines
    ShapePlumberPointer shapePlumber = std::make_shared<ShapePlumber>();
    initDeferredPipelines(*shapePlumber);

    // Extract opaques / transparents / lights / overlays
    const auto opaques = items[0];
    const auto transparents = items[1];
    const auto lights = items[2];
    const auto overlayOpaques = items[3];
    const auto overlayTransparents = items[4];
    const auto background = items[5];

    const auto framebuffer = addJob<PrepareFramebuffer>("PrepareFramebuffer");

    addJob<DrawBounds>("DrawBounds", opaques);

    // Blit!
    addJob<Blit>("Blit", framebuffer);
}

void PrepareFramebuffer::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, gpu::FramebufferPointer& framebuffer) {
    auto framebufferCache = DependencyManager::get<FramebufferCache>();
    auto framebufferSize = framebufferCache->getFrameBufferSize();
    glm::uvec2 frameSize(framebufferSize.width(), framebufferSize.height());

    // Resizing framebuffers instead of re-building them seems to cause issues with threaded rendering
    if (_framebuffer && _framebuffer->getSize() != frameSize) {
        _framebuffer.reset();
    }

    if (!_framebuffer) {
        _framebuffer = gpu::FramebufferPointer(gpu::Framebuffer::create("forward"));

        auto colorFormat = gpu::Element::COLOR_SRGBA_32;
        auto defaultSampler = gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_POINT);
        auto colorTexture = gpu::TexturePointer(gpu::Texture::create2D(colorFormat, frameSize.x, frameSize.y, defaultSampler));
        _framebuffer->setRenderBuffer(0, colorTexture);

        auto depthFormat = gpu::Element(gpu::SCALAR, gpu::UINT32, gpu::DEPTH_STENCIL); // Depth24_Stencil8 texel format
        auto depthTexture = gpu::TexturePointer(gpu::Texture::create2D(depthFormat, frameSize.x, frameSize.y, defaultSampler));
        _framebuffer->setDepthStencilBuffer(depthTexture, depthFormat);
    }

    auto args = renderContext->args;
    gpu::doInBatch(args->_context, [&](gpu::Batch& batch) {
        batch.enableStereo(false);
        batch.setViewportTransform(args->_viewport);
        batch.setStateScissorRect(args->_viewport);

        batch.setFramebuffer(_framebuffer);
        batch.clearFramebuffer(
            gpu::Framebuffer::BUFFER_COLOR0 |
            gpu::Framebuffer::BUFFER_DEPTH |
            gpu::Framebuffer::BUFFER_STENCIL,
            vec4(vec3(0), 1), 1.0, 0.0, true);
    });

    framebuffer = _framebuffer;
}

const gpu::PipelinePointer DrawBounds::getPipeline() {
    if (!_boundsPipeline) {
        auto vs = gpu::Shader::createVertex(std::string(drawItemBounds_vert));
        auto ps = gpu::Shader::createPixel(std::string(drawItemBounds_frag));
        gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);

        gpu::Shader::BindingSet slotBindings;
        gpu::Shader::makeProgram(*program, slotBindings);

        _cornerLocation = program->getUniforms().findLocation("inBoundPos");
        _scaleLocation = program->getUniforms().findLocation("inBoundDim");

        auto state = std::make_shared<gpu::State>();
        state->setDepthTest(true, false, gpu::LESS_EQUAL);
        state->setBlendFunction(true,
            gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA,
            gpu::State::DEST_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::ZERO);

        _boundsPipeline = gpu::Pipeline::create(program, state);
    }
    return _boundsPipeline;
}

void DrawBounds::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const Inputs& items) {
    RenderArgs* args = renderContext->args;

    gpu::doInBatch(args->_context, [&](gpu::Batch& batch) {
        // Setup projection
        glm::mat4 projMat;
        Transform viewMat;
        args->getViewFrustum().evalProjectionMatrix(projMat);
        args->getViewFrustum().evalViewTransform(viewMat);
        batch.setProjectionTransform(projMat);
        batch.setViewTransform(viewMat);
        batch.setModelTransform(Transform());

        // Bind program
        batch.setPipeline(getPipeline());
        assert(_cornerLocation >= 0);
        assert(_scaleLocation >= 0);

        // Render bounds
        for (const auto& item : items) {
            batch._glUniform3fv(_cornerLocation, 1, (const float*)(&item.bound.getCorner()));
            batch._glUniform3fv(_scaleLocation, 1, (const float*)(&item.bound.getScale()));

            static const int NUM_VERTICES_PER_CUBE = 24;
            batch.draw(gpu::LINES, NUM_VERTICES_PER_CUBE, 0);
        }
    });
}
