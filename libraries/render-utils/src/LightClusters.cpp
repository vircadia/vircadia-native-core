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

#include <gpu/StandardShaderLib.h>

#include "lightClusters_drawGrid_vert.h"
#include "lightClusters_drawGrid_frag.h"

//#include "lightClusters_drawClusterFromDepth_vert.h"
#include "lightClusters_drawClusterFromDepth_frag.h"

enum LightClusterGridShader_MapSlot {
    DEFERRED_BUFFER_LINEAR_DEPTH_UNIT = 0,
};

enum LightClusterGridShader_BufferSlot {
    LIGHT_CLUSTER_GRID_FRUSTUM_GRID_SLOT = 0,
    DEFERRED_FRAME_TRANSFORM_BUFFER_SLOT,
    CAMERA_CORRECTION_BUFFER_SLOT,
    LIGHT_GPU_SLOT = render::ShapePipeline::Slot::LIGHT,
    LIGHT_INDEX_GPU_SLOT,
};

#include "DeferredLightingEffect.h"

LightClusters::LightClusters() :
_lightIndicesBuffer(std::make_shared<gpu::Buffer>()) {
}

void LightClusters::updateFrustum(const ViewFrustum& frustum) {
    _frustum = frustum;

    _frustumGridBuffer.edit().updateFrustum(frustum);
}

void LightClusters::updateLightStage(const LightStagePointer& lightStage) {
    _lightStage = lightStage;
}

void LightClusters::updateVisibleLights(const LightStage::LightIndices& visibleLights) {
    
    _visibleLightIndices.clear();
   // _lightClusters->_visibleLightIndices.push_back(0);
    _visibleLightIndices = visibleLights;
    _lightIndicesBuffer._buffer->setData(_visibleLightIndices.size() * sizeof(int), (const gpu::Byte*) _visibleLightIndices.data());
    _lightIndicesBuffer._size = _visibleLightIndices.size() * sizeof(int);
}


LightClusteringPass::LightClusteringPass() {
}


void LightClusteringPass::configure(const Config& config) {
    if (_lightClusters) {
        if (_lightClusters->_frustumGridBuffer->rangeNear != config.rangeNear) {
            _lightClusters->_frustumGridBuffer.edit().rangeNear = config.rangeNear;
        }
        if (_lightClusters->_frustumGridBuffer->rangeFar != config.rangeFar) {
            _lightClusters->_frustumGridBuffer.edit().rangeFar = config.rangeFar;
        }
    }
    
    _freeze = config.freeze;
}

void LightClusteringPass::run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext, const Inputs& inputs, Outputs& output) {
    auto args = renderContext->args;
    
    auto deferredTransform = inputs.get0();
    auto lightingModel = inputs.get1();
    auto surfaceGeometryFramebuffer = inputs.get2();
    
    bool points = lightingModel->isPointLightEnabled();
    bool spots = lightingModel->isSpotLightEnabled();
    auto deferredLightingEffect = DependencyManager::get<DeferredLightingEffect>();
    
    if (!_lightClusters) {
        _lightClusters = deferredLightingEffect->getLightClusters();
    }
    
    // first update the Grid with the new frustum
    if (!_freeze) {
        _lightClusters->updateFrustum(args->getViewFrustum());
    }
    
    // Now gather the lights
    // gather lights
    auto& srcPointLights = deferredLightingEffect->_pointLights;
    auto& srcSpotLights = deferredLightingEffect->_spotLights;
    int numPointLights = (int) srcPointLights.size();
    int offsetPointLights = 0;
    int numSpotLights = (int) srcSpotLights.size();
    int offsetSpotLights = numPointLights;
    
    std::vector<int> lightIndices(numPointLights + numSpotLights + 1);
    lightIndices[0] = 0;
    
    if (points && !srcPointLights.empty()) {
        memcpy(lightIndices.data() + (lightIndices[0] + 1), srcPointLights.data(), srcPointLights.size() * sizeof(int));
        lightIndices[0] += (int)srcPointLights.size();
    }
    if (spots && !srcSpotLights.empty()) {
        memcpy(lightIndices.data() + (lightIndices[0] + 1), srcSpotLights.data(), srcSpotLights.size() * sizeof(int));
        lightIndices[0] += (int)srcSpotLights.size();
    }
    
    _lightClusters->updateVisibleLights(lightIndices);
    
    output = _lightClusters;
}

DebugLightClusters::DebugLightClusters() {

}


void DebugLightClusters::configure(const Config& config) {
    doDrawGrid = config.doDrawGrid;
    doDrawClusterFromDepth = config.doDrawClusterFromDepth;

}

const gpu::PipelinePointer DebugLightClusters::getDrawClusterGridPipeline() {
    if (!_drawClusterGrid) {
        auto vs = gpu::Shader::createVertex(std::string(lightClusters_drawGrid_vert));
        auto ps = gpu::Shader::createPixel(std::string(lightClusters_drawGrid_frag));
        gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);

        gpu::Shader::BindingSet slotBindings;
        slotBindings.insert(gpu::Shader::Binding(std::string("frustumGridBuffer"), LIGHT_CLUSTER_GRID_FRUSTUM_GRID_SLOT));

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

const gpu::PipelinePointer DebugLightClusters::getDrawClusterFromDepthPipeline() {
    if (!_drawClusterFromDepth) {
       // auto vs = gpu::Shader::createVertex(std::string(lightClusters_drawGrid_vert));
        auto vs = gpu::StandardShaderLib::getDrawUnitQuadTexcoordVS();
        auto ps = gpu::Shader::createPixel(std::string(lightClusters_drawClusterFromDepth_frag));
        gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);

        gpu::Shader::BindingSet slotBindings;
        slotBindings.insert(gpu::Shader::Binding(std::string("frustumGridBuffer"), LIGHT_CLUSTER_GRID_FRUSTUM_GRID_SLOT));
        slotBindings.insert(gpu::Shader::Binding(std::string("linearZeyeMap"), DEFERRED_BUFFER_LINEAR_DEPTH_UNIT));
        slotBindings.insert(gpu::Shader::Binding(std::string("cameraCorrectionBuffer"), CAMERA_CORRECTION_BUFFER_SLOT));
        slotBindings.insert(gpu::Shader::Binding(std::string("deferredFrameTransformBuffer"), DEFERRED_FRAME_TRANSFORM_BUFFER_SLOT));

        gpu::Shader::makeProgram(*program, slotBindings);


        auto state = std::make_shared<gpu::State>();

    //    state->setDepthTest(true, false, gpu::LESS_EQUAL);

        // Blend on transparent
        state->setBlendFunction(true, gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA);

        // Good to go add the brand new pipeline
        _drawClusterFromDepth = gpu::Pipeline::create(program, state);
    }
    return _drawClusterFromDepth;
}

void DebugLightClusters::run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext, const Inputs& inputs) {
    auto deferredTransform = inputs.get0();
    auto deferredFramebuffer = inputs.get1();
    auto lightingModel = inputs.get2();
    auto linearDepthTarget = inputs.get3();
    auto lightClusters = inputs.get4();
    
    auto args = renderContext->args;
    
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

    batch.setUniformBuffer(LIGHT_CLUSTER_GRID_FRUSTUM_GRID_SLOT, lightClusters->_frustumGridBuffer);


    if (doDrawClusterFromDepth) {
        batch.setPipeline(getDrawClusterFromDepthPipeline());
        batch.setUniformBuffer(DEFERRED_FRAME_TRANSFORM_BUFFER_SLOT, deferredTransform->getFrameTransformBuffer());

        if (linearDepthTarget) {
            batch.setResourceTexture(DEFERRED_BUFFER_LINEAR_DEPTH_UNIT, linearDepthTarget->getLinearDepthTexture());
        }

        batch.draw(gpu::TRIANGLE_STRIP, 4, 0);

        // Probably not necessary in the long run because the gpu layer would unbound this texture if used as render target
      
        batch.setResourceTexture(DEFERRED_BUFFER_LINEAR_DEPTH_UNIT, nullptr);
        batch.setUniformBuffer(DEFERRED_FRAME_TRANSFORM_BUFFER_SLOT, nullptr);
    }
    
    if (doDrawGrid) {
        // bind the one gpu::Pipeline we need
        batch.setPipeline(getDrawClusterGridPipeline());
        
        auto dims = lightClusters->_frustumGridBuffer->dims;
        glm::ivec3 summedDims(dims.x*dims.y * dims.z, dims.x*dims.y, dims.x);
        batch.drawInstanced(summedDims.x, gpu::LINES, 24, 0);
    }
    args->_context->appendFrameBatch(batch);

}