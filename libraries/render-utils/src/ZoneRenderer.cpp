//
//  ZoneRenderer.cpp
//  render/src/render-utils
//
//  Created by Sam Gateau on 4/4/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "ZoneRenderer.h"


#include <gpu/Context.h>
#include <gpu/StandardShaderLib.h>

#include <render/FilterTask.h>
#include <render/DrawTask.h>

#include "DeferredLightingEffect.h"

#include "zone_drawKeyLight_frag.h"
#include "zone_drawAmbient_frag.h"
#include "zone_drawSkybox_frag.h"


using namespace render;

class SetupZones {
public:
    using Inputs = render::ItemBounds;
    using JobModel = render::Job::ModelI<SetupZones, Inputs>;

    SetupZones() {}

    void run(const RenderContextPointer& context, const Inputs& inputs);

protected:
};

const Selection::Name ZoneRendererTask::ZONES_SELECTION { "RankedZones" };

void ZoneRendererTask::build(JobModel& task, const Varying& input, Varying& ouput) {
    // Filter out the sorted list of zones
    const auto zoneItems = task.addJob<render::SelectSortItems>("FilterZones", input, ZONES_SELECTION.c_str());

    // just setup the current zone env
    task.addJob<SetupZones>("SetupZones", zoneItems);

    ouput = zoneItems;
}

void SetupZones::run(const RenderContextPointer& context, const Inputs& inputs) {
    
    auto backgroundStage = DependencyManager::get<DeferredLightingEffect>()->getBackgroundStage();
    backgroundStage->_currentFrame.clear();

    // call render in the correct order first...
    render::renderItems(context, inputs);

}

const gpu::PipelinePointer& DebugZoneLighting::getKeyLightPipeline() {
    if (!_keyLightPipeline) {
        auto vs = gpu::StandardShaderLib::getDrawTransformUnitQuadVS();
        auto ps = gpu::Shader::createPixel(std::string(zone_drawKeyLight_frag));
        gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);

        gpu::Shader::BindingSet slotBindings;
        slotBindings.insert(gpu::Shader::Binding(std::string("deferredFrameTransformBuffer"), ZONE_DEFERRED_TRANSFORM_BUFFER));
        slotBindings.insert(gpu::Shader::Binding(std::string("lightBuffer"), ZONE_KEYLIGHT_BUFFER));

        gpu::Shader::makeProgram(*program, slotBindings);

        gpu::StatePointer state = gpu::StatePointer(new gpu::State());

        state->setBlendFunction(true, gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA);
        _keyLightPipeline = gpu::Pipeline::create(program, state);
    }
    return _keyLightPipeline;
}

const gpu::PipelinePointer& DebugZoneLighting::getAmbientPipeline() {
    if (!_ambientPipeline) {
        auto vs = gpu::StandardShaderLib::getDrawTransformUnitQuadVS();
        auto ps = gpu::Shader::createPixel(std::string(zone_drawAmbient_frag));
        gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);

        gpu::Shader::BindingSet slotBindings;
        slotBindings.insert(gpu::Shader::Binding(std::string("deferredFrameTransformBuffer"), ZONE_DEFERRED_TRANSFORM_BUFFER));
        slotBindings.insert(gpu::Shader::Binding(std::string("lightAmbientBuffer"), ZONE_AMBIENT_BUFFER));
        slotBindings.insert(gpu::Shader::Binding(std::string("skyboxMap"), ZONE_AMBIENT_MAP));
        
        gpu::Shader::makeProgram(*program, slotBindings);

        gpu::StatePointer state = gpu::StatePointer(new gpu::State());

        state->setBlendFunction(true, gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA);
        _ambientPipeline = gpu::Pipeline::create(program, state);
    }
    return _ambientPipeline;
}
const gpu::PipelinePointer& DebugZoneLighting::getBackgroundPipeline() {
    if (!_backgroundPipeline) {
        auto vs = gpu::StandardShaderLib::getDrawTransformUnitQuadVS();
        auto ps = gpu::Shader::createPixel(std::string(zone_drawSkybox_frag));
        gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);

        gpu::Shader::BindingSet slotBindings;
        slotBindings.insert(gpu::Shader::Binding(std::string("deferredFrameTransformBuffer"), ZONE_DEFERRED_TRANSFORM_BUFFER));
        slotBindings.insert(gpu::Shader::Binding(std::string("skyboxMap"), ZONE_SKYBOX_MAP));
        slotBindings.insert(gpu::Shader::Binding(std::string("skyboxBuffer"), ZONE_SKYBOX_BUFFER));
        
        gpu::Shader::makeProgram(*program, slotBindings);

        gpu::StatePointer state = gpu::StatePointer(new gpu::State());

        state->setBlendFunction(true, gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA);
        _backgroundPipeline = gpu::Pipeline::create(program, state);
    }
    return _backgroundPipeline;
}

void DebugZoneLighting::run(const render::RenderContextPointer& context, const Inputs& inputs) {
    RenderArgs* args = context->args;

    auto deferredTransform = inputs;

    auto lightStage = DependencyManager::get<DeferredLightingEffect>()->getLightStage();
    model::LightPointer keyLight;
    if (lightStage && lightStage->_currentFrame._sunLights.size()) {
        keyLight = lightStage->getLight(lightStage->_currentFrame._sunLights.front());
    }
    else {
        keyLight = DependencyManager::get<DeferredLightingEffect>()->getGlobalLight();
    }

    model::LightPointer keyAmbiLight;
    if (lightStage && lightStage->_currentFrame._ambientLights.size()) {
        keyAmbiLight = lightStage->getLight(lightStage->_currentFrame._ambientLights.front());
    } else {
        keyAmbiLight = DependencyManager::get<DeferredLightingEffect>()->getGlobalLight();
    }

    auto backgroundStage = DependencyManager::get<DeferredLightingEffect>()->getBackgroundStage();
    model::SkyboxPointer skybox;
    if (backgroundStage && backgroundStage->_currentFrame._backgrounds.size()) {
        auto background = backgroundStage->getBackground(backgroundStage->_currentFrame._backgrounds.front());
        if (background) {
            skybox = background->getSkybox();
        }
    } else {
        skybox = DependencyManager::get<DeferredLightingEffect>()->getDefaultSkybox();
    }

    gpu::doInBatch(args->_context, [=](gpu::Batch& batch) {

        batch.setViewportTransform(args->_viewport);
        auto viewFrustum = args->getViewFrustum();
        batch.setProjectionTransform(viewFrustum.getProjection());
        batch.resetViewTransform();

        Transform model;

        batch.setUniformBuffer(ZONE_DEFERRED_TRANSFORM_BUFFER, deferredTransform->getFrameTransformBuffer());

        batch.setPipeline(getKeyLightPipeline());
        model.setTranslation(glm::vec3(-4.0, -3.0, -10.0));
        batch.setModelTransform(model);
        if (keyLight) {
            batch.setUniformBuffer(ZONE_KEYLIGHT_BUFFER, keyLight->getLightSchemaBuffer());
        }
        batch.draw(gpu::TRIANGLE_STRIP, 4);

        batch.setPipeline(getAmbientPipeline());
        model.setTranslation(glm::vec3(-4.0, 0.0, -10.0));
        batch.setModelTransform(model);
        if (keyAmbiLight) {
            batch.setUniformBuffer(ZONE_AMBIENT_BUFFER, keyAmbiLight->getAmbientSchemaBuffer());

            if (keyAmbiLight->getAmbientMap()) {
                batch.setResourceTexture(ZONE_AMBIENT_MAP, keyAmbiLight->getAmbientMap());
            }
        }
        batch.draw(gpu::TRIANGLE_STRIP, 4);

        batch.setPipeline(getBackgroundPipeline());
        model.setTranslation(glm::vec3(-4.0, 3.0, -10.0));
        batch.setModelTransform(model);
        if (skybox) {
            batch.setResourceTexture(ZONE_SKYBOX_MAP, skybox->getCubemap());
            batch.setUniformBuffer(ZONE_SKYBOX_BUFFER, skybox->getSchemaBuffer());
        }
        batch.draw(gpu::TRIANGLE_STRIP, 4);


    }); 
}
