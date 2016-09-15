//
//  LightClusters.h
//
//  Created by Sam Gateau on 9/7/2016.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_render_utils_LightClusters_h
#define hifi_render_utils_LightClusters_h

#include <gpu/Buffer.h>
#include <render/Engine.h>
#include "LightStage.h"

#include <ViewFrustum.h>

class FrustumGrid {
public:
    float frustumNear { 0.1f };
    float rangeNear { 1.0f };
    float rangeFar { 100.0f };
    float frustumFar { 10000.0f };

    glm::ivec3 dims { 8, 8, 12 };
    float spare;

    glm::mat4 eyeToGridProj;
    glm::mat4 worldToEyeMat;
    glm::mat4 eyeToWorldMat;

    void updateFrustum(const ViewFrustum& frustum) {
        frustumNear = frustum.getNearClip();
        frustumFar = frustum.getFarClip();
        
        eyeToGridProj = frustum.evalProjectionMatrixRange(rangeNear, rangeFar);

        Transform view;
        frustum.evalViewTransform(view);
        eyeToWorldMat = view.getMatrix();
        worldToEyeMat = view.getInverseMatrix();
    }

    // Copy paste of the slh functions
    using vec3 = glm::vec3;
    using ivec3 = glm::ivec3;
    using mat4 = glm::mat4;
#define frustumGrid (*this)
#include "LightClusterGrid_shared.slh"


    using Planes = std::vector < glm::vec4 >;
    void generateGridPlanes(Planes& xPlanes, Planes& yPlanes, Planes& zPlanes);
};



class LightClusters {
public:
    using LightID = LightStage::Index;

    static const glm::uvec4 MAX_GRID_DIMENSIONS;

    LightClusters();

    void setDimensions(glm::uvec3 gridDims, uint32_t listBudget);

    void updateFrustum(const ViewFrustum& frustum);

    void updateLightStage(const LightStagePointer& lightStage);

    void updateLightFrame(const LightStage::Frame& lightFrame, bool points = true, bool spots = true);

    void updateClusters();

    ViewFrustum _frustum;

    LightStagePointer _lightStage;

    

    gpu::StructBuffer<FrustumGrid> _frustumGridBuffer;

    FrustumGrid::Planes _gridPlanes[3];

    LightStage::LightIndices _visibleLightIndices;
    gpu::BufferView _lightIndicesBuffer;

    uint32_t _numClusters { 0 };

    const uint32_t EMPTY_CLUSTER { 0x0000FFFF };
    const LightID INVALID_LIGHT { LightStage::INVALID_INDEX };

    std::vector<uint32_t> _clusterGrid;
    std::vector<LightID> _clusterContent;
    gpu::BufferView _clusterGridBuffer;
    gpu::BufferView _clusterContentBuffer;



};

using LightClustersPointer = std::shared_ptr<LightClusters>;



class LightClusteringPassConfig : public render::Job::Config {
    Q_OBJECT
    Q_PROPERTY(float rangeNear MEMBER rangeNear NOTIFY dirty)
    Q_PROPERTY(float rangeFar MEMBER rangeFar NOTIFY dirty)
 
    Q_PROPERTY(int dimX MEMBER dimX NOTIFY dirty)
    Q_PROPERTY(int dimY MEMBER dimY NOTIFY dirty)
    Q_PROPERTY(int dimZ MEMBER dimZ NOTIFY dirty)
    
    Q_PROPERTY(bool freeze MEMBER freeze NOTIFY dirty)
public:
    LightClusteringPassConfig() : render::Job::Config(true){}
    float rangeNear{ 1.0f };
    float rangeFar{ 512.0f };

    int dimX { 8 };
    int dimY { 8 };
    int dimZ { 12 };

    bool freeze{ false };
    
signals:
    void dirty();
    
protected:
};

#include "DeferredFrameTransform.h"
#include "LightingModel.h"
#include "SurfaceGeometryPass.h"

class LightClusteringPass {
public:
    using Inputs = render::VaryingSet3<DeferredFrameTransformPointer, LightingModelPointer, LinearDepthFramebufferPointer>;
    using Outputs = LightClustersPointer;
    using Config = LightClusteringPassConfig;
    using JobModel = render::Job::ModelIO<LightClusteringPass, Inputs, Outputs, Config>;
    
    LightClusteringPass();
    
    void configure(const Config& config);
    
    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext, const Inputs& inputs, Outputs& output);
    
protected:
    LightClustersPointer _lightClusters;
    bool _freeze;
};







class DebugLightClustersConfig : public render::Job::Config {
    Q_OBJECT
    Q_PROPERTY(bool doDrawGrid MEMBER doDrawGrid NOTIFY dirty)
    Q_PROPERTY(bool doDrawClusterFromDepth MEMBER doDrawClusterFromDepth NOTIFY dirty)
    Q_PROPERTY(bool doDrawContent MEMBER doDrawContent NOTIFY dirty)
public:
    DebugLightClustersConfig() : render::Job::Config(true){}


    bool doDrawGrid{ false };
    bool doDrawClusterFromDepth { false };
    bool doDrawContent { false };
    
signals:
    void dirty();

protected:
};


#include "DeferredFramebuffer.h"

class DebugLightClusters {
public:
    using Inputs = render::VaryingSet5 < DeferredFrameTransformPointer, DeferredFramebufferPointer, LightingModelPointer, LinearDepthFramebufferPointer, LightClustersPointer>;
    using Config = DebugLightClustersConfig;
    using JobModel = render::Job::ModelI<DebugLightClusters, Inputs, Config>;

    DebugLightClusters();

    void configure(const Config& config);

    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext, const Inputs& inputs);

protected:
    gpu::BufferPointer _gridBuffer;
    gpu::PipelinePointer _drawClusterGrid;
    gpu::PipelinePointer _drawClusterFromDepth;
    gpu::PipelinePointer _drawClusterContent;
    const gpu::PipelinePointer getDrawClusterGridPipeline();
    const gpu::PipelinePointer getDrawClusterFromDepthPipeline();
    const gpu::PipelinePointer getDrawClusterContentPipeline();
    bool doDrawGrid { false };
    bool doDrawClusterFromDepth { false };
    bool doDrawContent { false };
};

#endif
