//
//  ToneMappingEffect.cpp
//  libraries/render-utils/src
//
//  Created by Sam Gateau on 12/7/2015.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ToneMappingEffect.h"

#include <gpu/Context.h>
#include <shaders/Shaders.h>

#include "render-utils/ShaderConstants.h"
#include "StencilMaskPass.h"
#include "FramebufferCache.h"


ToneMappingEffect::ToneMappingEffect() {
    Parameters parameters;
    _parametersBuffer = gpu::BufferView(std::make_shared<gpu::Buffer>(sizeof(Parameters), (const gpu::Byte*) &parameters));
}

void ToneMappingEffect::init(RenderArgs* args) {
    auto blitProgram = gpu::Shader::createProgram(shader::render_utils::program::toneMapping);

    auto blitState = std::make_shared<gpu::State>();
    blitState->setColorWriteMask(true, true, true, true);
    _blitLightBuffer = gpu::PipelinePointer(gpu::Pipeline::create(blitProgram, blitState));
}

void ToneMappingEffect::setExposure(float exposure) {
    auto& params = _parametersBuffer.get<Parameters>();
    if (params._exposure != exposure) {
        _parametersBuffer.edit<Parameters>()._exposure = exposure;
        _parametersBuffer.edit<Parameters>()._twoPowExposure = pow(2.0, exposure);
    }
}

void ToneMappingEffect::setToneCurve(ToneCurve curve) {
    auto& params = _parametersBuffer.get<Parameters>();
    if (params._toneCurve != curve) {
        _parametersBuffer.edit<Parameters>()._toneCurve = curve;
    }
}

void ToneMappingEffect::render(RenderArgs* args, const gpu::TexturePointer& lightingBuffer, const gpu::FramebufferPointer& requestedDestinationFramebuffer) {
    if (!_blitLightBuffer) {
        init(args);
    }

    auto destinationFramebuffer = requestedDestinationFramebuffer;
    if (!destinationFramebuffer) {
        destinationFramebuffer = args->_blitFramebuffer;
    }

    auto framebufferSize = glm::ivec2(lightingBuffer->getDimensions());
    gpu::doInBatch("ToneMappingEffect::render", args->_context, [&](gpu::Batch& batch) {
        batch.enableStereo(false);
        batch.setFramebuffer(destinationFramebuffer);

        // FIXME: Generate the Luminosity map
        //batch.generateTextureMips(lightingBuffer);

        batch.setViewportTransform(args->_viewport);
        batch.setProjectionTransform(glm::mat4());
        batch.resetViewTransform();
        batch.setModelTransform(gpu::Framebuffer::evalSubregionTexcoordTransform(framebufferSize, args->_viewport));
        batch.setPipeline(_blitLightBuffer);

        batch.setUniformBuffer(render_utils::slot::buffer::ToneMappingParams, _parametersBuffer);
        batch.setResourceTexture(render_utils::slot::texture::ToneMappingColor, lightingBuffer);
        batch.draw(gpu::TRIANGLE_STRIP, 4);
    });
}


void ToneMappingDeferred::configure(const Config& config) {
     _toneMappingEffect.setExposure(config.exposure);
     _toneMappingEffect.setToneCurve((ToneMappingEffect::ToneCurve)config.curve);
}

void ToneMappingDeferred::run(const render::RenderContextPointer& renderContext, const Inputs& inputs) {

    auto lightingBuffer = inputs.get0()->getRenderBuffer(0);
    auto destFbo = inputs.get1();
    _toneMappingEffect.render(renderContext->args, lightingBuffer, destFbo);
}
