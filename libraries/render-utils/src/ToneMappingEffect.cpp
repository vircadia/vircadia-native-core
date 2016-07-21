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
#include <gpu/StandardShaderLib.h>

#include <RenderArgs.h>

#include "FramebufferCache.h"

#include "toneMapping_frag.h"

const int ToneMappingEffect_ParamsSlot = 0;
const int ToneMappingEffect_LightingMapSlot = 0;

ToneMappingEffect::ToneMappingEffect() {
    Parameters parameters;
    _parametersBuffer = gpu::BufferView(std::make_shared<gpu::Buffer>(sizeof(Parameters), (const gpu::Byte*) &parameters));
}

void ToneMappingEffect::init() {
    auto blitPS = gpu::ShaderPointer(gpu::Shader::createPixel(std::string(toneMapping_frag)));

    auto blitVS = gpu::StandardShaderLib::getDrawViewportQuadTransformTexcoordVS();
    auto blitProgram = gpu::ShaderPointer(gpu::Shader::createProgram(blitVS, blitPS));

    gpu::Shader::BindingSet slotBindings;
    slotBindings.insert(gpu::Shader::Binding(std::string("toneMappingParamsBuffer"), ToneMappingEffect_ParamsSlot));
    slotBindings.insert(gpu::Shader::Binding(std::string("colorMap"), ToneMappingEffect_LightingMapSlot));
    gpu::Shader::makeProgram(*blitProgram, slotBindings);
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

void ToneMappingEffect::render(RenderArgs* args, const gpu::TexturePointer& lightingBuffer, const gpu::FramebufferPointer& destinationFramebuffer) {
    if (!_blitLightBuffer) {
        init();
    }
    auto framebufferSize = glm::ivec2(lightingBuffer->getDimensions());
    gpu::doInBatch(args->_context, [&](gpu::Batch& batch) {
        batch.enableStereo(false);
        batch.setFramebuffer(destinationFramebuffer);

        // FIXME: Generate the Luminosity map
        //batch.generateTextureMips(lightingBuffer);

        batch.setViewportTransform(args->_viewport);
        batch.setProjectionTransform(glm::mat4());
        batch.setViewTransform(Transform());
        batch.setModelTransform(gpu::Framebuffer::evalSubregionTexcoordTransform(framebufferSize, args->_viewport));
        batch.setPipeline(_blitLightBuffer);

        batch.setUniformBuffer(ToneMappingEffect_ParamsSlot, _parametersBuffer);
        batch.setResourceTexture(ToneMappingEffect_LightingMapSlot, lightingBuffer);
        batch.draw(gpu::TRIANGLE_STRIP, 4);
    });
}


void ToneMappingDeferred::configure(const Config& config) {
     _toneMappingEffect.setExposure(config.exposure);
     _toneMappingEffect.setToneCurve((ToneMappingEffect::ToneCurve)config.curve);
}

void ToneMappingDeferred::run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext, const Inputs& inputs) {

    auto lightingBuffer = inputs.get0()->getRenderBuffer(0);
    auto destFbo = inputs.get1();
    _toneMappingEffect.render(renderContext->args, lightingBuffer, destFbo);
}
