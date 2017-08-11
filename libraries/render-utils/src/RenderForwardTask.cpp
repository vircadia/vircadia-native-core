
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
#include <ViewFrustum.h>
#include <gpu/Context.h>

#include "FramebufferCache.h"
#include "TextureCache.h"

#include <gpu/StandardShaderLib.h>

#include "nop_frag.h"

using namespace render;
extern void initForwardPipelines(ShapePlumber& plumber);

void RenderForwardTask::build(JobModel& task, const render::Varying& input, render::Varying& output) {
    auto items = input.get<Input>();

    // Prepare the ShapePipelines
    ShapePlumberPointer shapePlumber = std::make_shared<ShapePlumber>();
    initForwardPipelines(*shapePlumber);

    // Extract opaques / transparents / lights / metas / overlays / background
    const auto& opaques = items.get0()[RenderFetchCullSortTask::OPAQUE_SHAPE];
//    const auto& transparents = items.get0()[RenderFetchCullSortTask::TRANSPARENT_SHAPE];
//    const auto& lights = items.get0()[RenderFetchCullSortTask::LIGHT];
//    const auto& metas = items.get0()[RenderFetchCullSortTask::META];
//    const auto& overlayOpaques = items.get0()[RenderFetchCullSortTask::OVERLAY_OPAQUE_SHAPE];
//    const auto& overlayTransparents = items.get0()[RenderFetchCullSortTask::OVERLAY_TRANSPARENT_SHAPE];
    const auto& background = items.get0()[RenderFetchCullSortTask::BACKGROUND];
//    const auto& spatialSelection = items[1];

    const auto framebuffer = task.addJob<PrepareFramebuffer>("PrepareFramebuffer");

    task.addJob<Draw>("DrawOpaques", opaques, shapePlumber);
    task.addJob<Stencil>("Stencil");
    task.addJob<DrawBackground>("DrawBackground", background);

    // Bounds do not draw on stencil buffer, so they must come last
    task.addJob<DrawBounds>("DrawBounds", opaques);

    // Blit!
    task.addJob<Blit>("Blit", framebuffer);
}

void PrepareFramebuffer::run(const RenderContextPointer& renderContext,
        gpu::FramebufferPointer& framebuffer) {
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
        auto colorTexture = gpu::Texture::create2D(colorFormat, frameSize.x, frameSize.y, gpu::Texture::SINGLE_MIP, defaultSampler);
        _framebuffer->setRenderBuffer(0, colorTexture);

        auto depthFormat = gpu::Element(gpu::SCALAR, gpu::UINT32, gpu::DEPTH_STENCIL); // Depth24_Stencil8 texel format
        auto depthTexture = gpu::Texture::create2D(depthFormat, frameSize.x, frameSize.y, gpu::Texture::SINGLE_MIP, defaultSampler);
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

void Draw::run(const RenderContextPointer& renderContext,
        const Inputs& items) {
    RenderArgs* args = renderContext->args;

    gpu::doInBatch(args->_context, [&](gpu::Batch& batch) {
        args->_batch = &batch;

        // Setup projection
        glm::mat4 projMat;
        Transform viewMat;
        args->getViewFrustum().evalProjectionMatrix(projMat);
        args->getViewFrustum().evalViewTransform(viewMat);
        batch.setProjectionTransform(projMat);
        batch.setViewTransform(viewMat);
        batch.setModelTransform(Transform());

        // Render items
        renderStateSortShapes(renderContext, _shapePlumber, items, -1);
    });
    args->_batch = nullptr;
}

const gpu::PipelinePointer Stencil::getPipeline() {
    if (!_stencilPipeline) {
        auto vs = gpu::StandardShaderLib::getDrawUnitQuadTexcoordVS();
        auto ps = gpu::Shader::createPixel(std::string(nop_frag));
        gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);
        gpu::Shader::makeProgram(*program);

        auto state = std::make_shared<gpu::State>();
        state->setDepthTest(true, false, gpu::LESS_EQUAL);
        const gpu::int8 STENCIL_OPAQUE = 1;
        state->setStencilTest(true, 0xFF, gpu::State::StencilTest(STENCIL_OPAQUE, 0xFF, gpu::ALWAYS,
                    gpu::State::STENCIL_OP_REPLACE,
                    gpu::State::STENCIL_OP_REPLACE,
                    gpu::State::STENCIL_OP_KEEP));

        _stencilPipeline = gpu::Pipeline::create(program, state);
    }
    return _stencilPipeline;
}

void Stencil::run(const RenderContextPointer& renderContext) {
    RenderArgs* args = renderContext->args;

    gpu::doInBatch(args->_context, [&](gpu::Batch& batch) {
        args->_batch = &batch;

        batch.enableStereo(false);
        batch.setViewportTransform(args->_viewport);
        batch.setStateScissorRect(args->_viewport);

        batch.setPipeline(getPipeline());
        batch.draw(gpu::TRIANGLE_STRIP, 4);
    });
    args->_batch = nullptr;
}

void DrawBackground::run(const RenderContextPointer& renderContext,
        const Inputs& background) {
    RenderArgs* args = renderContext->args;

    gpu::doInBatch(args->_context, [&](gpu::Batch& batch) {
        args->_batch = &batch;

        batch.enableSkybox(true);
        batch.setViewportTransform(args->_viewport);
        batch.setStateScissorRect(args->_viewport);

        // Setup projection
        glm::mat4 projMat;
        Transform viewMat;
        args->getViewFrustum().evalProjectionMatrix(projMat);
        args->getViewFrustum().evalViewTransform(viewMat);
        batch.setProjectionTransform(projMat);
        batch.setViewTransform(viewMat);

        renderItems(renderContext, background);
    });
    args->_batch = nullptr;
}


