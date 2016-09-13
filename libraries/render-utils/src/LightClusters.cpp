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


#include "lightClusters_drawClusterContent_vert.h"
#include "lightClusters_drawClusterContent_frag.h"

enum LightClusterGridShader_MapSlot {
    DEFERRED_BUFFER_LINEAR_DEPTH_UNIT = 0,
};

enum LightClusterGridShader_BufferSlot {
    LIGHT_CLUSTER_GRID_FRUSTUM_GRID_SLOT = 0,
    DEFERRED_FRAME_TRANSFORM_BUFFER_SLOT,
    CAMERA_CORRECTION_BUFFER_SLOT,
    LIGHT_GPU_SLOT = render::ShapePipeline::Slot::LIGHT,
    LIGHT_INDEX_GPU_SLOT,

    LIGHT_CLUSTER_GRID_CLUSTER_GRID_SLOT,
    LIGHT_CLUSTER_GRID_CLUSTER_CONTENT_SLOT,
};

#include "DeferredLightingEffect.h"

LightClusters::LightClusters() :
    _lightIndicesBuffer(std::make_shared<gpu::Buffer>()),
    _clusterGridBuffer(std::make_shared<gpu::Buffer>(), gpu::Element::INDEX_INT32),
    _clusterContentBuffer(std::make_shared<gpu::Buffer>(), gpu::Element::INDEX_INT32) {
    setDimensions(_frustumGridBuffer->dims, 10000);
}

void LightClusters::setDimensions(glm::uvec3 gridDims, uint32_t listBudget) {
    _frustumGridBuffer.edit().dims = gridDims;

    _numClusters = _frustumGridBuffer.edit().frustumGrid_numClusters();

    _clusterGridBuffer._size = _clusterGridBuffer._buffer->resize(_numClusters * sizeof(uint32_t));
    _clusterContentBuffer._size = _clusterContentBuffer._buffer->resize(listBudget * sizeof(uint32_t));
    _clusterGrid.resize(_numClusters, EMPTY_CLUSTER);
    _clusterContent.resize(listBudget, INVALID_LIGHT);
}


void LightClusters::updateFrustum(const ViewFrustum& frustum) {
    _frustum = frustum;

    _frustumGridBuffer.edit().updateFrustum(frustum);
}

void LightClusters::updateLightStage(const LightStagePointer& lightStage) {
    _lightStage = lightStage;
    
  }

void LightClusters::updateLightFrame(const LightStage::Frame& lightFrame, bool points, bool spots) {

    // start fresh
    _visibleLightIndices.clear();

    // Now gather the lights
    // gather lights
    auto& srcPointLights = lightFrame._pointLights;
    auto& srcSpotLights = lightFrame._spotLights;
    int numPointLights = (int)srcPointLights.size();
    // int offsetPointLights = 0;
    int numSpotLights = (int)srcSpotLights.size();
    // int offsetSpotLights = numPointLights;

    _visibleLightIndices.resize(numPointLights + numSpotLights + 1);

    _visibleLightIndices[0] = 0;

    if (points && !srcPointLights.empty()) {
        memcpy(_visibleLightIndices.data() + (_visibleLightIndices[0] + 1), srcPointLights.data(), srcPointLights.size() * sizeof(int));
        _visibleLightIndices[0] += (int)srcPointLights.size();
    }
    if (spots && !srcSpotLights.empty()) {
        memcpy(_visibleLightIndices.data() + (_visibleLightIndices[0] + 1), srcSpotLights.data(), srcSpotLights.size() * sizeof(int));
        _visibleLightIndices[0] += (int)srcSpotLights.size();
    }

    _lightIndicesBuffer._buffer->setData(_visibleLightIndices.size() * sizeof(int), (const gpu::Byte*) _visibleLightIndices.data());
    _lightIndicesBuffer._size = _visibleLightIndices.size() * sizeof(int);
}

void LightClusters::updateClusters() {
    // Clean up last info
    std::vector< std::vector< uint32_t > > clusterGrid(_numClusters);

    _clusterGrid.resize(_numClusters, EMPTY_CLUSTER);
    uint32_t maxNumIndices = _clusterContent.size();
    _clusterContent.resize(maxNumIndices, INVALID_LIGHT);

    auto theFrustumGrid(_frustumGridBuffer.get());

    glm::ivec3 gridPosToOffset(1, theFrustumGrid.dims.x, theFrustumGrid.dims.x * theFrustumGrid.dims.y);

    uint32_t numClusterTouched = 0;
    for (size_t lightNum = 1; lightNum < _visibleLightIndices.size(); ++lightNum) {
        auto lightId = _visibleLightIndices[lightNum];
        auto light = _lightStage->getLight(lightId);
        if (!light)
            continue;

        auto worldOri = light->getPosition();
        auto radius = light->getMaximumRadius();

        // Bring into frustum eye space
        auto eyeOri = theFrustumGrid.frustumGrid_worldToEye(glm::vec4(worldOri, 1.0f));

        // Remove light that slipped through and is not in the z range
        float eyeZMax = eyeOri.z - radius;
        if (eyeZMax > -theFrustumGrid.rangeNear) {
            continue;
        }
        float eyeZMin = eyeOri.z + radius;
        if (eyeZMin < -theFrustumGrid.rangeFar) {
            continue;
        }

        // Get z slices
        int zMin = theFrustumGrid.frustumGrid_eyeDepthToClusterLayer(eyeZMin);
        int zMax = theFrustumGrid.frustumGrid_eyeDepthToClusterLayer(eyeZMax);
        // That should never happen
        if (zMin == -1 && zMax == -1) {
            continue;
        }


        // 
        float eyeOriLen2 = glm::length2(eyeOri);

        // CLamp the z range 
        zMin = std::max(0, zMin);



        // find 2D corners of the sphere in grid
        int xMin { 0 };
        int xMax { theFrustumGrid.dims.x - 1 };
        int yMin { 0 };
        int yMax { theFrustumGrid.dims.y - 1 };

        float radius2 = radius * radius;
        auto eyeOriH = eyeOri;
        auto eyeOriV = eyeOri;

        eyeOriH.y = 0.0f;
        eyeOriV.x = 0.0f;

        float eyeOriLen2H = glm::length2(eyeOriH);
        float eyeOriLen2V = glm::length2(eyeOriV);

        if ((eyeOriLen2H > radius2) && (eyeOriLen2V > radius2)) {
            float eyeOriLenH = sqrt(eyeOriLen2H);
            float eyeOriLenV = sqrt(eyeOriLen2V);

            auto eyeOriDirH = glm::vec3(eyeOriH) / eyeOriLenH;
            auto eyeOriDirV = glm::vec3(eyeOriV) / eyeOriLenV;



            float eyeToTangentCircleLenH = sqrt(eyeOriLen2H - radius2);
            float eyeToTangentCircleLenV = sqrt(eyeOriLen2V - radius2);

            float eyeToTangentCircleTanH = radius / eyeToTangentCircleLenH;
            float eyeToTangentCircleTanV = radius / eyeToTangentCircleLenV;

            float eyeToTangentCircleCosH = eyeToTangentCircleLenH / eyeOriLenH;
            float eyeToTangentCircleCosV = eyeToTangentCircleLenV / eyeOriLenV;

            float eyeToTangentCircleSinH = radius / eyeOriLenH;
            float eyeToTangentCircleSinV = radius / eyeOriLenV;


            // rotate the eyeToOriDir (H & V) in both directions
            glm::vec3 leftDir(eyeOriDirH.x * eyeToTangentCircleCosH - eyeOriDirH.z * eyeToTangentCircleSinH, 0.0f, eyeOriDirH.x * eyeToTangentCircleSinH + eyeOriDirH.z * eyeToTangentCircleCosH);
            glm::vec3 rightDir(eyeOriDirH.x * eyeToTangentCircleCosH + eyeOriDirH.z * eyeToTangentCircleSinH, 0.0f, eyeOriDirH.x * -eyeToTangentCircleSinH + eyeOriDirH.z * eyeToTangentCircleCosH);
            glm::vec3 bottomDir(0.0f, eyeOriDirV.y * eyeToTangentCircleCosV - eyeOriDirV.z * eyeToTangentCircleSinV, eyeOriDirV.y * eyeToTangentCircleSinV + eyeOriDirV.z * eyeToTangentCircleCosV);
            glm::vec3 topDir(0.0f, eyeOriDirV.y * eyeToTangentCircleCosV + eyeOriDirV.z * eyeToTangentCircleSinV, eyeOriDirV.y * -eyeToTangentCircleSinV + eyeOriDirV.z * eyeToTangentCircleCosV);

        }

        // now voxelize
        for (auto z = zMin; z <= zMax; z++) {
            for (auto y = yMin; y <= yMax; y++) {
                for (auto x = xMin; x <= xMax; x++) {
                    auto index = x + gridPosToOffset.y * y + gridPosToOffset.z * z;
                    clusterGrid[index].emplace_back(lightId);
                    numClusterTouched++;
                }
            }
        }
    }

    // Lights have been gathered now reexpress in terms of 2 sequential buffers

    uint16_t indexOffset = 0;
    for (int i = 0; i < clusterGrid.size(); i++) {
        auto& cluster = clusterGrid[i];
        uint16_t numLights = ((uint16_t)cluster.size());
        uint16_t offset = indexOffset;

        _clusterGrid[i] = (uint32_t)((numLights << 16) | offset);

        if (numLights) {
            memcpy(_clusterContent.data() + indexOffset, cluster.data(), numLights * sizeof(uint32_t));
        }

        indexOffset += numLights;
    }

    // update the buffers
    _clusterGridBuffer._buffer->setData(_clusterGridBuffer._size, (gpu::Byte*) _clusterGrid.data());
    _clusterContentBuffer._buffer->setSubData(0, indexOffset * sizeof(uint32_t), (gpu::Byte*) _clusterContent.data());
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
    
    
    if (!_lightClusters) {
        _lightClusters = std::make_shared<LightClusters>();
    }
    
    // first update the Grid with the new frustum
    if (!_freeze) {
        _lightClusters->updateFrustum(args->getViewFrustum());
    }
    
    // From the LightStage and the current frame, update the light cluster Grid
    auto deferredLightingEffect = DependencyManager::get<DeferredLightingEffect>();
    auto lightStage = deferredLightingEffect->getLightStage();
    _lightClusters->updateLightStage(lightStage);
    _lightClusters->updateLightFrame(lightStage->_currentFrame, lightingModel->isPointLightEnabled(), lightingModel->isSpotLightEnabled());
    
    _lightClusters->updateClusters();

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

const gpu::PipelinePointer DebugLightClusters::getDrawClusterContentPipeline() {
    if (!_drawClusterContent) {
        auto vs = gpu::Shader::createVertex(std::string(lightClusters_drawClusterContent_vert));
        auto ps = gpu::Shader::createPixel(std::string(lightClusters_drawClusterContent_frag));
        gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);

        gpu::Shader::BindingSet slotBindings;
        slotBindings.insert(gpu::Shader::Binding(std::string("frustumGridBuffer"), LIGHT_CLUSTER_GRID_FRUSTUM_GRID_SLOT));
        slotBindings.insert(gpu::Shader::Binding(std::string("clusterGridBuffer"), LIGHT_CLUSTER_GRID_CLUSTER_GRID_SLOT));
        slotBindings.insert(gpu::Shader::Binding(std::string("clusterContentBuffer"), LIGHT_CLUSTER_GRID_CLUSTER_CONTENT_SLOT));


        gpu::Shader::makeProgram(*program, slotBindings);


        auto state = std::make_shared<gpu::State>();

        state->setDepthTest(true, false, gpu::LESS_EQUAL);

        // Blend on transparent
        state->setBlendFunction(true, gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA);

        // Good to go add the brand new pipeline
        _drawClusterContent = gpu::Pipeline::create(program, state);
    }
    return _drawClusterContent;
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

    if (doDrawContent) {
        // bind the one gpu::Pipeline we need
        batch.setPipeline(getDrawClusterContentPipeline());
        batch.setUniformBuffer(LIGHT_CLUSTER_GRID_CLUSTER_GRID_SLOT, lightClusters->_clusterGridBuffer);
        batch.setUniformBuffer(LIGHT_CLUSTER_GRID_CLUSTER_CONTENT_SLOT, lightClusters->_clusterContentBuffer);

        auto dims = lightClusters->_frustumGridBuffer->dims;
        glm::ivec3 summedDims(dims.x*dims.y * dims.z, dims.x*dims.y, dims.x);
        batch.drawInstanced(summedDims.x, gpu::LINES, 24, 0);
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