
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

#include <PerfStat.h>
#include <PathUtils.h>
#include <ViewFrustum.h>
#include <gpu/Context.h>
#include <gpu/Texture.h>
#include <graphics/ShaderConstants.h>
#include <render/ShapePipeline.h>

#include <render/FilterTask.h>

#include "RenderHifi.h"
#include "render-utils/ShaderConstants.h"
#include "StencilMaskPass.h"
#include "ZoneRenderer.h"
#include "FadeEffect.h"
#include "ToneMappingEffect.h"
#include "BackgroundStage.h"
#include "FramebufferCache.h"
#include "TextureCache.h"
#include "RenderCommonTask.h"

namespace ru {
    using render_utils::slot::texture::Texture;
    using render_utils::slot::buffer::Buffer;
}

namespace gr {
    using graphics::slot::texture::Texture;
    using graphics::slot::buffer::Buffer;
}

using namespace render;

extern void initForwardPipelines(ShapePlumber& plumber);

void RenderForwardTask::build(JobModel& task, const render::Varying& input, render::Varying& output) {
    // Prepare the ShapePipelines
    auto fadeEffect = DependencyManager::get<FadeEffect>();
    ShapePlumberPointer shapePlumber = std::make_shared<ShapePlumber>();
    initForwardPipelines(*shapePlumber);

    // Unpack inputs
    const auto& inputs = input.get<Input>();
    
    // Separate the fetched items
    const auto& fetchedItems = inputs.get0();

        const auto& items = fetchedItems.get0();

            // Extract opaques / transparents / lights / metas / layered / background
            const auto& opaques = items[RenderFetchCullSortTask::OPAQUE_SHAPE];
            const auto& transparents = items[RenderFetchCullSortTask::TRANSPARENT_SHAPE];
            const auto& metas = items[RenderFetchCullSortTask::META];
            const auto& inFrontOpaque = items[RenderFetchCullSortTask::LAYER_FRONT_OPAQUE_SHAPE];
            const auto& inFrontTransparent = items[RenderFetchCullSortTask::LAYER_FRONT_TRANSPARENT_SHAPE];
            const auto& hudOpaque = items[RenderFetchCullSortTask::LAYER_HUD_OPAQUE_SHAPE];
            const auto& hudTransparent = items[RenderFetchCullSortTask::LAYER_HUD_TRANSPARENT_SHAPE];

    // Lighting model comes next, the big configuration of the view
    const auto& lightingModel = inputs[1];

    // Extract the Lighting Stages Current frame ( and zones)
    const auto& lightingStageInputs = inputs.get2();
        // Fetch the current frame stacks from all the stages
        const auto currentStageFrames = lightingStageInputs.get0();
            const auto lightFrame = currentStageFrames[0];
            const auto backgroundFrame = currentStageFrames[1];
 
        const auto& zones = lightingStageInputs[1];

    // First job, alter faded
    fadeEffect->build(task, opaques);

    // Prepare objects shared by several jobs
    const auto deferredFrameTransform = task.addJob<GenerateDeferredFrameTransform>("DeferredFrameTransform");

    // GPU jobs: Start preparing the main framebuffer
    const auto framebuffer = task.addJob<PrepareFramebuffer>("PrepareFramebuffer");

    task.addJob<PrepareForward>("PrepareForward", lightFrame);

    // draw a stencil mask in hidden regions of the framebuffer.
    task.addJob<PrepareStencil>("PrepareStencil", framebuffer);

    // Layered
    const auto nullJitter = Varying(glm::vec2(0.0f, 0.0f));
    const auto inFrontOpaquesInputs = DrawLayered3D::Inputs(inFrontOpaque, lightingModel, nullJitter).asVarying();
    const auto inFrontTransparentsInputs = DrawLayered3D::Inputs(inFrontTransparent, lightingModel, nullJitter).asVarying();
    task.addJob<DrawLayered3D>("DrawInFrontOpaque", inFrontOpaquesInputs, true);

    // Draw opaques forward
    const auto opaqueInputs = DrawForward::Inputs(opaques, lightingModel).asVarying();
    task.addJob<DrawForward>("DrawOpaques", opaqueInputs, shapePlumber);

    // Similar to light stage, background stage has been filled by several potential render items and resolved for the frame in this job
    const auto backgroundInputs = DrawBackgroundStage::Inputs(lightingModel, backgroundFrame).asVarying();
    task.addJob<DrawBackgroundStage>("DrawBackgroundForward", backgroundInputs);

    // Draw transparent objects forward
    const auto transparentInputs = DrawForward::Inputs(transparents, lightingModel).asVarying();
    task.addJob<DrawForward>("DrawTransparents", transparentInputs, shapePlumber);
    task.addJob<DrawLayered3D>("DrawInFrontTransparent", inFrontTransparentsInputs, false);

    {  // Debug the bounds of the rendered items, still look at the zbuffer

        task.addJob<DrawBounds>("DrawMetaBounds", metas);
        task.addJob<DrawBounds>("DrawBounds", opaques);
        task.addJob<DrawBounds>("DrawTransparentBounds", transparents);

        task.addJob<DrawBounds>("DrawZones", zones);
        const auto debugZoneInputs = DebugZoneLighting::Inputs(deferredFrameTransform, lightFrame, backgroundFrame).asVarying();
        task.addJob<DebugZoneLighting>("DrawZoneStack", debugZoneInputs);
    }

    // Just resolve the msaa
    const auto resolveInputs =
        ResolveFramebuffer::Inputs(framebuffer, static_cast<gpu::FramebufferPointer>(nullptr)).asVarying();
    const auto resolvedFramebuffer = task.addJob<ResolveFramebuffer>("Resolve", resolveInputs);
    //auto resolvedFramebuffer = task.addJob<ResolveNewFramebuffer>("Resolve", framebuffer);

#if defined(Q_OS_ANDROID)
#else
    // Lighting Buffer ready for tone mapping
    // Forward rendering on GLES doesn't support tonemapping to and from the same FBO, so we specify 
    // the output FBO as null, which causes the tonemapping to target the blit framebuffer
    const auto toneMappingInputs = ToneMappingDeferred::Inputs(resolvedFramebuffer, static_cast<gpu::FramebufferPointer>(nullptr)).asVarying();
    task.addJob<ToneMappingDeferred>("ToneMapping", toneMappingInputs);
#endif

    // Layered Overlays
    // Composite the HUD and HUD overlays
    task.addJob<CompositeHUD>("HUD", resolvedFramebuffer);

    const auto hudOpaquesInputs = DrawLayered3D::Inputs(hudOpaque, lightingModel, nullJitter).asVarying();
    const auto hudTransparentsInputs = DrawLayered3D::Inputs(hudTransparent, lightingModel, nullJitter).asVarying();
    task.addJob<DrawLayered3D>("DrawHUDOpaque", hudOpaquesInputs, true);
    task.addJob<DrawLayered3D>("DrawHUDTransparent", hudTransparentsInputs, false);

    // Disable blit because we do tonemapping and compositing directly to the blit FBO
    // Blit!
    // task.addJob<Blit>("Blit", framebuffer);
}

void PrepareFramebuffer::configure(const Config& config) {
    _numSamples = config.getNumSamples();
}

void PrepareFramebuffer::run(const RenderContextPointer& renderContext, gpu::FramebufferPointer& framebuffer) {
    glm::uvec2 frameSize(renderContext->args->_viewport.z, renderContext->args->_viewport.w);

    // Resizing framebuffers instead of re-building them seems to cause issues with threaded rendering
    if (_framebuffer && (_framebuffer->getSize() != frameSize || _framebuffer->getNumSamples() != _numSamples)) {
        _framebuffer.reset();
    }

    if (!_framebuffer) {
        _framebuffer = gpu::FramebufferPointer(gpu::Framebuffer::create("forward"));

        int numSamples = _numSamples;

        auto colorFormat = gpu::Element::COLOR_SRGBA_32;
        auto defaultSampler = gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_LINEAR);
        auto colorTexture =
            gpu::Texture::createRenderBufferMultisample(colorFormat, frameSize.x, frameSize.y, numSamples, defaultSampler);
        _framebuffer->setRenderBuffer(0, colorTexture);

        auto depthFormat = gpu::Element(gpu::SCALAR, gpu::UINT32, gpu::DEPTH_STENCIL);  // Depth24_Stencil8 texel format
        auto depthTexture =
           gpu::Texture::createRenderBufferMultisample(depthFormat, frameSize.x, frameSize.y, numSamples, defaultSampler);
        _framebuffer->setDepthStencilBuffer(depthTexture, depthFormat);
    }

    auto args = renderContext->args;
    gpu::doInBatch("PrepareFramebuffer::run", args->_context, [&](gpu::Batch& batch) {
        batch.enableStereo(false);
        batch.setViewportTransform(args->_viewport);
        batch.setStateScissorRect(args->_viewport);

        batch.setFramebuffer(_framebuffer);
        batch.clearFramebuffer(gpu::Framebuffer::BUFFER_COLOR0 | gpu::Framebuffer::BUFFER_DEPTH |
            gpu::Framebuffer::BUFFER_STENCIL,
            vec4(vec3(0), 0), 1.0, 0, true);
    });

    framebuffer = _framebuffer;
}

void PrepareForward::run(const RenderContextPointer& renderContext, const Inputs& inputs) {
    RenderArgs* args = renderContext->args;

    gpu::doInBatch("RenderForward::Draw::run", args->_context, [&](gpu::Batch& batch) {
        args->_batch = &batch;

        graphics::LightPointer keySunLight;
        auto lightStage = args->_scene->getStage<LightStage>();
        if (lightStage) {
            keySunLight = lightStage->getCurrentKeyLight(*inputs);
        }

        graphics::LightPointer keyAmbiLight;
        if (lightStage) {
            keyAmbiLight = lightStage->getCurrentAmbientLight(*inputs);
        }

        if (keySunLight) {
            batch.setUniformBuffer(gr::Buffer::KeyLight, keySunLight->getLightSchemaBuffer());
        }

        if (keyAmbiLight) {
            batch.setUniformBuffer(gr::Buffer::AmbientLight, keyAmbiLight->getAmbientSchemaBuffer());

            if (keyAmbiLight->getAmbientMap()) {
                batch.setResourceTexture(ru::Texture::Skybox, keyAmbiLight->getAmbientMap());
            }
        }
    });
}

void DrawForward::run(const RenderContextPointer& renderContext, const Inputs& inputs) {
    RenderArgs* args = renderContext->args;

    const auto& inItems = inputs.get0();
    const auto& lightingModel = inputs.get1();

    gpu::doInBatch("DrawForward::run", args->_context, [&](gpu::Batch& batch) {
        args->_batch = &batch;


        // Setup projection
        glm::mat4 projMat;
        Transform viewMat;
        args->getViewFrustum().evalProjectionMatrix(projMat);
        args->getViewFrustum().evalViewTransform(viewMat);
        batch.setProjectionTransform(projMat);
        batch.setViewTransform(viewMat);
        batch.setModelTransform(Transform());

        // Setup lighting model for all items;
        batch.setUniformBuffer(ru::Buffer::LightModel, lightingModel->getParametersBuffer());

        // From the lighting model define a global shapeKey ORED with individiual keys
        ShapeKey::Builder keyBuilder;
        if (lightingModel->isWireframeEnabled()) {
            keyBuilder.withWireframe();
        }
        ShapeKey globalKey = keyBuilder.build();
        args->_globalShapeKey = globalKey._flags.to_ulong();

        // Render items
        renderStateSortShapes(renderContext, _shapePlumber, inItems, -1, globalKey);

        args->_batch = nullptr;
        args->_globalShapeKey = 0;
    });
}


