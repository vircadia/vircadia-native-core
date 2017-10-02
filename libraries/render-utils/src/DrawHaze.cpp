//
//  DrawHaze.cpp
//  libraries/render-utils/src
//
//  Created by Nissim Hadar on 9/1/2017.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "DrawHaze.h"

#include <gpu/Context.h>
#include <gpu/StandardShaderLib.h>

#include "StencilMaskPass.h"
#include "FramebufferCache.h"

#include "Haze_frag.h"

void HazeConfig::setHazeColorR(const float value) { 
    hazeColorR = value; 
}

void HazeConfig::setHazeColorG(const float value) { 
    hazeColorG = value; 
}

void HazeConfig::setHazeColorB(const float value) { 
    hazeColorB = value; 
}

void HazeConfig::setDirectionalLightAngle_degs(const float value) {
    hazeDirectionalLightAngle_degs = value; 
}

void HazeConfig::setDirectionalLightColorR(const float value) { 
    hazeDirectionalLightColorR = value; 
}

void HazeConfig::setDirectionalLightColorG(const float value) { 
    hazeDirectionalLightColorG = value; 
}

void HazeConfig::setDirectionalLightColorB(const float value) {
    hazeDirectionalLightColorB = value; 
}

void HazeConfig::setHazeBaseReference(const float value) { 
    hazeBaseReference = value; 
}

void HazeConfig::setHazeActive(const bool active) { 
    isHazeActive = active; 
}

void HazeConfig::setAltitudeBased(const bool active) { 
    isAltitudeBased = active; 
}

void HazeConfig::setIsdirectionalLightAttenuationActive(const bool active) { 
    isDirectionalLightAttenuationActive = active;
}

void HazeConfig::setModulateColorActive(const bool active) { 
    isModulateColorActive = active; 
}

void HazeConfig::setHazeRange_m(const float value) { 
    hazeRange_m = value;
}

void HazeConfig::setHazeAltitude_m(const float value) {
    hazeAltitude_m = value; 
}

void HazeConfig::setHazeRangeKeyLight_m(const float value) { 
    hazeRangeKeyLight_m = value;
}

void HazeConfig::setHazeAltitudeKeyLight_m(const float value) {
    hazeAltitudeKeyLight_m = value;
}

void HazeConfig::setHazeBackgroundBlendValue(const float value) {
    hazeBackgroundBlendValue = value;
}

MakeHaze::MakeHaze() {
    _haze = std::make_shared<model::Haze>();
}

void MakeHaze::configure(const Config& config) {
    _haze->setHazeColor(glm::vec3(config.hazeColorR, config.hazeColorG, config.hazeColorB));
    _haze->setDirectionalLightBlend(model::convertDirectionalLightAngleToPower(config.hazeDirectionalLightAngle_degs));

    _haze->setDirectionalLightColor(glm::vec3(config.hazeDirectionalLightColorR, config.hazeDirectionalLightColorG, config.hazeDirectionalLightColorB));
    _haze->setHazeBaseReference(config.hazeBaseReference);

    _haze->setHazeActive(config.isHazeActive);
    _haze->setAltitudeBased(config.isAltitudeBased);
    _haze->setDirectionaLightAttenuationActive(config.isDirectionaLightAttenuationActive);
    _haze->setModulateColorActive(config.isModulateColorActive);

    _haze->setHazeRangeFactor(model::convertHazeRangeToHazeRangeFactor(config.hazeRange_m));
    _haze->setHazeAltitudeFactor(model::convertHazeAltitudeToHazeAltitudeFactor(config.hazeAltitude_m));

    _haze->setHazeKeyLightRangeFactor(model::convertHazeRangeToHazeRangeFactor(config.hazeKeyLightRange_m));
    _haze->setHazeKeyLightAltitudeFactor(model::convertHazeAltitudeToHazeAltitudeFactor(config.hazeKeyLightAltitude_m));

    _haze->setHazeBackgroundBlendValue(config.hazeBackgroundBlendValue);
}

void MakeHaze::run(const render::RenderContextPointer& renderContext, model::HazePointer& haze) {
    haze = _haze;
}

const int HazeEffect_ParamsSlot = 0;
const int HazeEffect_TransformBufferSlot = 1;

const int HazeEffect_LightingMapSlot = 2;
const int HazeEffect_LinearDepthMapSlot = 3;

void DrawHaze::configure(const Config& config) {
}

void DrawHaze::run(const render::RenderContextPointer& renderContext, const Inputs& inputs) {
    const auto haze = inputs.get0();
    const auto inputBuffer = inputs.get1()->getRenderBuffer(0);
    const auto framebuffer = inputs.get2();
    const auto transformBuffer = inputs.get3();

    auto outputBuffer = inputs.get4();

    auto depthBuffer = framebuffer->getLinearDepthTexture();

    if (_hazePipeline == nullptr) {
        gpu::ShaderPointer ps = gpu::Shader::createPixel(std::string(Haze_frag));
        gpu::ShaderPointer vs = gpu::StandardShaderLib::getDrawViewportQuadTransformTexcoordVS();

        gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);
        gpu::StatePointer state = gpu::StatePointer(new gpu::State());

        // Mask out haze on the tablet
        PrepareStencil::testNoAA(*state);

        gpu::Shader::BindingSet slotBindings;
        slotBindings.insert(gpu::Shader::Binding(std::string("hazeBuffer"), HazeEffect_ParamsSlot));
        slotBindings.insert(gpu::Shader::Binding(std::string("deferredFrameTransformBuffer"), HazeEffect_TransformBufferSlot));
        slotBindings.insert(gpu::Shader::Binding(std::string("colorMap"), HazeEffect_LightingMapSlot));
        slotBindings.insert(gpu::Shader::Binding(std::string("linearDepthMap"), HazeEffect_LinearDepthMapSlot));
        gpu::Shader::makeProgram(*program, slotBindings);

        _hazePipeline = gpu::PipelinePointer(gpu::Pipeline::create(program, state));
    }

    auto sourceFramebufferSize = glm::ivec2(inputBuffer->getDimensions());

    gpu::doInBatch(renderContext->args->_context, [&](gpu::Batch& batch) {
        batch.enableStereo(false);
        batch.setFramebuffer(outputBuffer);

        batch.setViewportTransform(renderContext->args->_viewport);
        batch.setProjectionTransform(glm::mat4());
        batch.resetViewTransform();
        batch.setModelTransform(
            gpu::Framebuffer::evalSubregionTexcoordTransform(sourceFramebufferSize, renderContext->args->_viewport));

        batch.setPipeline(_hazePipeline);

        batch.setUniformBuffer(HazeEffect_ParamsSlot, haze->getParametersBuffer());
        batch.setUniformBuffer(HazeEffect_TransformBufferSlot, transformBuffer->getFrameTransformBuffer());

        batch.setResourceTexture(HazeEffect_LightingMapSlot, inputBuffer);
        batch.setResourceTexture(HazeEffect_LinearDepthMapSlot, depthBuffer);

        batch.draw(gpu::TRIANGLE_STRIP, 4);
    });
}
