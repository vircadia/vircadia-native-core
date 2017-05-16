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

#include "zone_drawAmbient_frag.h"


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
    
    // call render in the correct order first...
    render::renderItems(context, inputs);

}

const gpu::PipelinePointer& DebugZoneLighting::getKeyLightPipeline() {
    if (!_keyLightPipeline) {
    }
    return _keyLightPipeline;
}

const gpu::PipelinePointer& DebugZoneLighting::getAmbientPipeline() {
    if (!_ambientPipeline) {
        auto vs = gpu::StandardShaderLib::getDrawTransformUnitQuadVS();
        auto ps = gpu::Shader::createPixel(std::string(zone_drawAmbient_frag));
        gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);

        gpu::Shader::BindingSet slotBindings;
        //slotBindings.insert(gpu::Shader::Binding(std::string("blurParamsBuffer"), BlurTask_ParamsSlot));
        //slotBindings.insert(gpu::Shader::Binding(std::string("sourceMap"), BlurTask_SourceSlot));
        gpu::Shader::makeProgram(*program, slotBindings);

        gpu::StatePointer state = gpu::StatePointer(new gpu::State());
        _ambientPipeline = gpu::Pipeline::create(program, state);
    }
    return _ambientPipeline;
}
const gpu::PipelinePointer& DebugZoneLighting::getBackgroundPipeline() {
    if (!_backgroundPipeline) {
    }
    return _backgroundPipeline;
}

void DebugZoneLighting::run(const render::RenderContextPointer& context) {
    RenderArgs* args = context->args;

    auto lightStage = DependencyManager::get<DeferredLightingEffect>()->getLightStage();
   
    const auto light = lightStage->getLight(0);
    model::LightPointer keyAmbiLight;
    if (lightStage && lightStage->_currentFrame._ambientLights.size()) {
        keyAmbiLight = lightStage->getLight(lightStage->_currentFrame._ambientLights.front());
    } else {
   //     keyAmbiLight = _allocatedLights[_globalLights.front()];
    }

/*    if (lightBufferUnit >= 0) {
        batch.setUniformBuffer(lightBufferUnit, keySunLight->getLightSchemaBuffer());
    }*/

    gpu::doInBatch(args->_context, [=](gpu::Batch& batch) {

        batch.setViewportTransform(args->_viewport);
        auto viewFrustum = args->getViewFrustum();
        batch.setProjectionTransform(viewFrustum.getProjection());
        batch.resetViewTransform();

        Transform model;
        model.setTranslation(glm::vec3(0.0, 0.0, -10.0));
        batch.setModelTransform(model);

        batch.setPipeline(getAmbientPipeline());
        if (keyAmbiLight) {
            if (keyAmbiLight->hasAmbient()) {
                batch.setUniformBuffer(0, keyAmbiLight->getAmbientSchemaBuffer());
            }

            if (keyAmbiLight->getAmbientMap()) {
                batch.setResourceTexture(0, keyAmbiLight->getAmbientMap());
            }
        }

        batch.draw(gpu::TRIANGLE_STRIP, 4);

    }); 
}
