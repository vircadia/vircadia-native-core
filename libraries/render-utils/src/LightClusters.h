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

    glm::ivec3 dims { 8, 8, 8 };
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

};

class LightClusters {
public:
    
    LightClusters();

    void updateFrustum(const ViewFrustum& frustum);

    void updateLightStage(const LightStagePointer& lightStage);

    void updateVisibleLights(const LightStage::LightIndices& visibleLights);


    ViewFrustum _frustum;

    LightStagePointer _lightStage;



    gpu::StructBuffer<FrustumGrid> _frustumGridBuffer;


    gpu::BufferPointer _lightIndicesBuffer;
};

using LightClustersPointer = std::shared_ptr<LightClusters>;



class DebugLightClustersConfig : public render::Job::Config {
    Q_OBJECT
        Q_PROPERTY(int numDrawn READ getNumDrawn NOTIFY numDrawnChanged)
        Q_PROPERTY(int maxDrawn MEMBER maxDrawn NOTIFY dirty)
public:
    DebugLightClustersConfig() : render::Job::Config(true){}

    int getNumDrawn() { return numDrawn; }
    void setNumDrawn(int num) { numDrawn = num; emit numDrawnChanged(); }

    int maxDrawn { -1 };

signals:
    void numDrawnChanged();
    void dirty();

protected:
    int numDrawn { 0 };
};


#include "DeferredFrameTransform.h"
#include "DeferredFramebuffer.h"
#include "LightingModel.h"
#include "SurfaceGeometryPass.h"

class DebugLightClusters {
public:
    using Inputs = render::VaryingSet4 < DeferredFrameTransformPointer, DeferredFramebufferPointer, LightingModelPointer, LinearDepthFramebufferPointer>;
    using Config = DebugLightClustersConfig;
    using JobModel = render::Job::ModelI<DebugLightClusters, Inputs, Config>;

    DebugLightClusters();

    void configure(const Config& config);

    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext, const Inputs& inputs);

protected:
    gpu::BufferPointer _gridBuffer;
    gpu::PipelinePointer _drawClusterGrid;
    gpu::PipelinePointer _drawClusterFromDepth;
    const gpu::PipelinePointer getDrawClusterGridPipeline();
    const gpu::PipelinePointer getDrawClusterFromDepthPipeline();
};

#endif
