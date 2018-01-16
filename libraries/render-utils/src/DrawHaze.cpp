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
#include "HazeStage.h"
#include "LightStage.h"

#include "Haze_frag.h"

void HazeConfig::setHazeColor(const glm::vec3 value) { 
    hazeColor = value; 
}

void HazeConfig::setHazeGlareAngle(const float value) {
    hazeGlareAngle = value; 
}

void HazeConfig::setHazeGlareColor(const glm::vec3 value) {
    hazeGlareColor = value; 
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

void HazeConfig::setHazeAttenuateKeyLight(const bool active) { 
    isHazeAttenuateKeyLight = active;
}

void HazeConfig::setModulateColorActive(const bool active) { 
    isModulateColorActive = active; 
}

void HazeConfig::setHazeEnableGlare(const bool active) {
    isHazeEnableGlare = active;
}

void HazeConfig::setHazeRange(const float value) { 
    hazeRange = value;
}

void HazeConfig::setHazeAltitude(const float value) {
    hazeHeight = value; 
}

void HazeConfig::setHazeKeyLightRange(const float value) { 
    hazeKeyLightRange = value;
}

void HazeConfig::setHazeKeyLightAltitude(const float value) {
    hazeKeyLightAltitude = value;
}

void HazeConfig::setHazeBackgroundBlend(const float value) {
    hazeBackgroundBlend = value;
}

MakeHaze::MakeHaze() {
    _haze = std::make_shared<model::Haze>();
}

void MakeHaze::configure(const Config& config) {
    _haze->setHazeColor(config.hazeColor);
    _haze->setHazeGlareBlend(model::Haze::convertGlareAngleToPower(config.hazeGlareAngle));

    _haze->setHazeGlareColor(config.hazeGlareColor);
    _haze->setHazeBaseReference(config.hazeBaseReference);

    _haze->setHazeActive(config.isHazeActive);
    _haze->setAltitudeBased(config.isAltitudeBased);
    _haze->setHazeAttenuateKeyLight(config.isHazeAttenuateKeyLight);
    _haze->setModulateColorActive(config.isModulateColorActive);
    _haze->setHazeEnableGlare(config.isHazeEnableGlare);

    _haze->setHazeRangeFactor(model::Haze::convertHazeRangeToHazeRangeFactor(config.hazeRange));
    _haze->setHazeAltitudeFactor(model::Haze::convertHazeAltitudeToHazeAltitudeFactor(config.hazeHeight));

    _haze->setHazeKeyLightRangeFactor(model::Haze::convertHazeRangeToHazeRangeFactor(config.hazeKeyLightRange));
    _haze->setHazeKeyLightAltitudeFactor(model::Haze::convertHazeAltitudeToHazeAltitudeFactor(config.hazeKeyLightAltitude));

    _haze->setHazeBackgroundBlend(config.hazeBackgroundBlend);
}

void MakeHaze::run(const render::RenderContextPointer& renderContext, model::HazePointer& haze) {
    haze = _haze;
}

const int HazeEffect_ParamsSlot = 0;
const int HazeEffect_TransformBufferSlot = 1;
const int HazeEffect_ColorMapSlot = 2;
const int HazeEffect_LinearDepthMapSlot = 3;
const int HazeEffect_LightingMapSlot = 4;

void DrawHaze::configure(const Config& config) {
}

void DrawHaze::run(const render::RenderContextPointer& renderContext, const Inputs& inputs) {
    const auto haze = inputs.get0();
    if (haze == nullptr) {
        return;
    }

    const auto inputBuffer = inputs.get1()->getRenderBuffer(0);
    const auto framebuffer = inputs.get2();
    const auto transformBuffer = inputs.get3();

    auto outputBuffer = inputs.get4();

    auto depthBuffer = framebuffer->getLinearDepthTexture();

    RenderArgs* args = renderContext->args;

    if (!_hazePipeline) {
        gpu::ShaderPointer ps = gpu::Shader::createPixel(std::string(Haze_frag));
        gpu::ShaderPointer vs = gpu::StandardShaderLib::getDrawViewportQuadTransformTexcoordVS();

        gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);
        gpu::StatePointer state = gpu::StatePointer(new gpu::State());

        // Mask out haze on the tablet
        PrepareStencil::testMask(*state);

        gpu::Shader::BindingSet slotBindings;
        slotBindings.insert(gpu::Shader::Binding(std::string("hazeBuffer"), HazeEffect_ParamsSlot));
        slotBindings.insert(gpu::Shader::Binding(std::string("deferredFrameTransformBuffer"), HazeEffect_TransformBufferSlot));
        slotBindings.insert(gpu::Shader::Binding(std::string("colorMap"), HazeEffect_ColorMapSlot));
        slotBindings.insert(gpu::Shader::Binding(std::string("linearDepthMap"), HazeEffect_LinearDepthMapSlot));
        slotBindings.insert(gpu::Shader::Binding(std::string("lightBuffer"), HazeEffect_LightingMapSlot));
        gpu::Shader::makeProgram(*program, slotBindings);

        _hazePipeline = gpu::PipelinePointer(gpu::Pipeline::create(program, state));
    }

    auto sourceFramebufferSize = glm::ivec2(inputBuffer->getDimensions());

    gpu::doInBatch(args->_context, [&](gpu::Batch& batch) {
        batch.enableStereo(false);
        batch.setFramebuffer(outputBuffer);

        batch.setViewportTransform(args->_viewport);
        batch.setProjectionTransform(glm::mat4());
        batch.resetViewTransform();
        batch.setModelTransform(gpu::Framebuffer::evalSubregionTexcoordTransform(sourceFramebufferSize, args->_viewport));

        batch.setPipeline(_hazePipeline);

        auto hazeStage = args->_scene->getStage<HazeStage>();
        if (hazeStage && hazeStage->_currentFrame._hazes.size() > 0) {
            model::HazePointer hazePointer = hazeStage->getHaze(hazeStage->_currentFrame._hazes.front());
            if (hazePointer) {
                batch.setUniformBuffer(HazeEffect_ParamsSlot, hazePointer->getHazeParametersBuffer());
            } else {
                // Something is wrong, so just quit Haze
                return;
            }
        }

        batch.setUniformBuffer(HazeEffect_TransformBufferSlot, transformBuffer->getFrameTransformBuffer());

	    auto lightStage = args->_scene->getStage<LightStage>();
	    if (lightStage) {
	        model::LightPointer keyLight;
	        keyLight = lightStage->getCurrentKeyLight();
	        if (keyLight) {
	            batch.setUniformBuffer(HazeEffect_LightingMapSlot, keyLight->getLightSchemaBuffer());
	        }
	    }

        batch.setResourceTexture(HazeEffect_ColorMapSlot, inputBuffer);
        batch.setResourceTexture(HazeEffect_LinearDepthMapSlot, depthBuffer);

        batch.draw(gpu::TRIANGLE_STRIP, 4);
    });
}
