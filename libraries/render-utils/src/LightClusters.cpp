//
//  LightClusters.cpp
//
//  Created by Sam Gateau on 9/7/2016.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "LightClusters.h"


#include <gpu/Context.h>
#include "lightClusters_drawGrid_vert.h"
#include "lightClusters_drawGrid_frag.h"

enum LightClusterGridShader_BufferSlot {
    LIGHT_CLUSTER_GRID_FRUSTUM_GRID_SLOT = 0,
};

#include "DeferredLightingEffect.h"

LightClusters::LightClusters() {
}

void LightClusters::updateFrustum(const ViewFrustum& frustum) {
    _frustum = frustum;

    _frustrumGridBuffer.edit().updateFrustrum(frustum);
}

void LightClusters::updateLightStage(const LightStagePointer& lightStage) {
    _lightStage = lightStage;
}

void LightClusters::updateVisibleLights(const LightStage::LightIndices& visibleLights) {

}





DebugLightClusters::DebugLightClusters() {

}


void DebugLightClusters::configure(const Config& config) {
}

const gpu::PipelinePointer DebugLightClusters::getDrawClusterGridPipeline() {
    if (!_drawClusterGrid) {
        auto vs = gpu::Shader::createVertex(std::string(lightClusters_drawGrid_vert));
        auto ps = gpu::Shader::createPixel(std::string(lightClusters_drawGrid_frag));
        gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);

        gpu::Shader::BindingSet slotBindings;
        slotBindings.insert(gpu::Shader::Binding(std::string("frustrumGridBuffer"), LIGHT_CLUSTER_GRID_FRUSTUM_GRID_SLOT));

        gpu::Shader::makeProgram(*program, slotBindings);


        auto state = std::make_shared<gpu::State>();

        state->setDepthTest(true, false, gpu::LESS_EQUAL);

        // Blend on transparent
        state->setBlendFunction(true, gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA);

        // Good to go add the brand new pipeline
        _drawClusterGrid = gpu::Pipeline::create(program, state);
    }
    return _drawClusterGrid;
}

void DebugLightClusters::run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext) {
    auto deferredLightingEffect = DependencyManager::get<DeferredLightingEffect>();
    auto lightClusters = deferredLightingEffect->getLightClusters();


   /* auto deferredTransform = inputs.get0();
    auto deferredFramebuffer = inputs.get1();
    auto lightingModel = inputs.get2();
    auto surfaceGeometryFramebuffer = inputs.get3();
    auto ssaoFramebuffer = inputs.get4();
    auto subsurfaceScatteringResource = inputs.get5();
    */
    auto args = renderContext->args;


    auto drawPipeline = getDrawClusterGridPipeline();

    gpu::Batch batch;


    // Assign the camera transform
    batch.setViewportTransform(args->_viewport);
    glm::mat4 projMat;
    Transform viewMat;
    args->getViewFrustum().evalProjectionMatrix(projMat);
    args->getViewFrustum().evalViewTransform(viewMat);
    batch.setProjectionTransform(projMat);
    batch.setViewTransform(viewMat, true);


    // Then the actual ClusterGrid attributes
    batch.setModelTransform(Transform());

    batch.setUniformBuffer(LIGHT_CLUSTER_GRID_FRUSTUM_GRID_SLOT, lightClusters->_frustrumGridBuffer);

    // bind the one gpu::Pipeline we need
    batch.setPipeline(drawPipeline);

    if (true) {
        batch.draw(gpu::LINES, 24, 0);
    }


    args->_context->appendFrameBatch(batch);

}