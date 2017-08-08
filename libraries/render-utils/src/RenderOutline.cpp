//
//  RenderOutline.cpp
//  render-utils/src/
//
//  Created by Olivier Prat on 08/08/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "RenderOutline.h"

#include "gpu/Context.h"
#include "gpu/StandardShaderLib.h"

#include "surfaceGeometry_copyDepth_frag.h"

void PrepareOutline::run(const render::RenderContextPointer& renderContext, const PrepareOutline::Input& deferredFramebuffer, PrepareOutline::Output& output) {
    glm::uvec2 frameSize(renderContext->args->_viewport.z, renderContext->args->_viewport.w);
    auto args = renderContext->args;

    // Resizing framebuffers instead of re-building them seems to cause issues with threaded 
    // rendering
    if (_outlineFramebuffer && _outlineFramebuffer->getSize() != frameSize) {
        _outlineFramebuffer.reset();
    }

    if (!_outlineFramebuffer) {
        _outlineFramebuffer = gpu::FramebufferPointer(gpu::Framebuffer::create("deferredOutline"));
        auto colorFormat = gpu::Element::COLOR_RED_HALF;

        auto defaultSampler = gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_POINT);
        auto primaryColorTexture = gpu::Texture::createRenderBuffer(colorFormat, frameSize.x, frameSize.y, gpu::Texture::SINGLE_MIP, defaultSampler);

        _outlineFramebuffer->setRenderBuffer(0, primaryColorTexture);
    }

    if (!_copyDepthPipeline) {
        auto vs = gpu::StandardShaderLib::getDrawViewportQuadTransformTexcoordVS();
        auto ps = gpu::Shader::createPixel(std::string(surfaceGeometry_copyDepth_frag));
        gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);

        gpu::Shader::BindingSet slotBindings;
        slotBindings.insert(gpu::Shader::Binding(std::string("depthMap"), 0));
        gpu::Shader::makeProgram(*program, slotBindings);

        gpu::StatePointer state = gpu::StatePointer(new gpu::State());

        state->setColorWriteMask(true, false, false, false);

        // Good to go add the brand new pipeline
        _copyDepthPipeline = gpu::Pipeline::create(program, state);
    }

    gpu::doInBatch(args->_context, [&](gpu::Batch& batch) {
        auto depthBuffer = deferredFramebuffer->getPrimaryDepthTexture();

        // Copy depth to texture as we will overwrite
        batch.enableStereo(false);

        batch.setViewportTransform(args->_viewport);
        batch.setProjectionTransform(glm::mat4());
        batch.resetViewTransform();
        batch.setModelTransform(gpu::Framebuffer::evalSubregionTexcoordTransform(frameSize, args->_viewport));

        batch.setFramebuffer(_outlineFramebuffer);
        batch.setPipeline(_copyDepthPipeline);
        batch.setResourceTexture(0, depthBuffer);
        batch.draw(gpu::TRIANGLE_STRIP, 4);

        // Restore previous frame buffer
        batch.setFramebuffer(deferredFramebuffer->getDeferredFramebuffer());
    });

    output = _outlineFramebuffer->getRenderBuffer(0);
}
