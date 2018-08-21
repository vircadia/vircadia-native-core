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
#include <shaders/Shaders.h>

#include <graphics/ShaderConstants.h>
#include "render-utils/ShaderConstants.h"

#include "StencilMaskPass.h"
#include "FramebufferCache.h"
#include "HazeStage.h"
#include "LightStage.h"

namespace ru {
    using render_utils::slot::texture::Texture;
    using render_utils::slot::buffer::Buffer;
}

namespace gr {
    using graphics::slot::texture::Texture;
    using graphics::slot::buffer::Buffer;
}

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
    _haze = std::make_shared<graphics::Haze>();
}

void MakeHaze::configure(const Config& config) {
    _haze->setHazeColor(config.hazeColor);
    _haze->setHazeGlareBlend(graphics::Haze::convertGlareAngleToPower(config.hazeGlareAngle));

    _haze->setHazeGlareColor(config.hazeGlareColor);
    _haze->setHazeBaseReference(config.hazeBaseReference);

    _haze->setHazeActive(config.isHazeActive);
    _haze->setAltitudeBased(config.isAltitudeBased);
    _haze->setHazeAttenuateKeyLight(config.isHazeAttenuateKeyLight);
    _haze->setModulateColorActive(config.isModulateColorActive);
    _haze->setHazeEnableGlare(config.isHazeEnableGlare);

    _haze->setHazeRangeFactor(graphics::Haze::convertHazeRangeToHazeRangeFactor(config.hazeRange));
    _haze->setHazeAltitudeFactor(graphics::Haze::convertHazeAltitudeToHazeAltitudeFactor(config.hazeHeight));

    _haze->setHazeKeyLightRangeFactor(graphics::Haze::convertHazeRangeToHazeRangeFactor(config.hazeKeyLightRange));
    _haze->setHazeKeyLightAltitudeFactor(graphics::Haze::convertHazeAltitudeToHazeAltitudeFactor(config.hazeKeyLightAltitude));

    _haze->setHazeBackgroundBlend(config.hazeBackgroundBlend);
}

void MakeHaze::run(const render::RenderContextPointer& renderContext, graphics::HazePointer& haze) {
    haze = _haze;
}

void DrawHaze::configure(const Config& config) {
}

void DrawHaze::run(const render::RenderContextPointer& renderContext, const Inputs& inputs) {
    const auto haze = inputs.get0();
    if (haze == nullptr) {
        return;
    }

    const auto outputBuffer = inputs.get1();
    const auto framebuffer = inputs.get2();
    const auto transformBuffer = inputs.get3();
    const auto lightingModel = inputs.get4();

    auto depthBuffer = framebuffer->getLinearDepthTexture();

    RenderArgs* args = renderContext->args;

    if (!_hazePipeline) {
        gpu::ShaderPointer program = gpu::Shader::createProgram(shader::render_utils::program::haze);
        gpu::StatePointer state = gpu::StatePointer(new gpu::State());

        state->setBlendFunction(true,
                                gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA,
                                gpu::State::FACTOR_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::ONE);

        // Mask out haze on the tablet
        PrepareStencil::testMask(*state);
        _hazePipeline = gpu::PipelinePointer(gpu::Pipeline::create(program, state));
    }

    auto outputFramebufferSize = glm::ivec2(outputBuffer->getSize());

    gpu::doInBatch("DrawHaze::run", args->_context, [&](gpu::Batch& batch) {
        batch.enableStereo(false);
        batch.setFramebuffer(outputBuffer);

        batch.setViewportTransform(args->_viewport);
        batch.setProjectionTransform(glm::mat4());
        batch.resetViewTransform();
        batch.setModelTransform(gpu::Framebuffer::evalSubregionTexcoordTransform(outputFramebufferSize, args->_viewport));

        batch.setPipeline(_hazePipeline);

        auto hazeStage = args->_scene->getStage<HazeStage>();
        if (hazeStage && hazeStage->_currentFrame._hazes.size() > 0) {
            graphics::HazePointer hazePointer = hazeStage->getHaze(hazeStage->_currentFrame._hazes.front());
            if (hazePointer) {
                batch.setUniformBuffer(ru::Buffer::HazeParams, hazePointer->getHazeParametersBuffer());
            } else {
                // Something is wrong, so just quit Haze
                return;
            }
        }

        batch.setUniformBuffer(ru::Buffer::DeferredFrameTransform, transformBuffer->getFrameTransformBuffer());
        batch.setUniformBuffer(ru::Buffer::LightModel, lightingModel->getParametersBuffer());
        batch.setResourceTexture(ru::Texture::HazeLinearDepth, depthBuffer);
        auto lightStage = args->_scene->getStage<LightStage>();
        if (lightStage) {
            graphics::LightPointer keyLight;
            keyLight = lightStage->getCurrentKeyLight();
            if (keyLight) {
                batch.setUniformBuffer(gr::Buffer::KeyLight, keyLight->getLightSchemaBuffer());
            }
        }


        batch.draw(gpu::TRIANGLE_STRIP, 4);
    });
}
