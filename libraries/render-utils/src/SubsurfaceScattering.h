//
//  SubsurfaceScattering.h
//  libraries/render-utils/src/
//
//  Created by Sam Gateau 6/3/2016.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_SubsurfaceScattering_h
#define hifi_SubsurfaceScattering_h

#include <DependencyManager.h>

#include "render/DrawTask.h"
#include "DeferredFrameTransform.h"

class SubsurfaceScatteringConfig : public render::Job::Config {
    Q_OBJECT
    Q_PROPERTY(float depthThreshold MEMBER depthThreshold NOTIFY dirty)
public:
    SubsurfaceScatteringConfig() : render::Job::Config(true) {}

    float depthThreshold{ 0.1f };

signals:
    void dirty();
};

class SubsurfaceScattering {
public:
    using Config = SubsurfaceScatteringConfig;
    using JobModel = render::Job::ModelIO<SubsurfaceScattering, DeferredFrameTransformPointer, gpu::FramebufferPointer, Config>;

    SubsurfaceScattering();

    void configure(const Config& config);
    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext, const DeferredFrameTransformPointer& frameTransform, gpu::FramebufferPointer& curvatureFramebuffer);
    
    float getCurvatureDepthThreshold() const { return _parametersBuffer.get<Parameters>().curvatureInfo.x; }


    static gpu::TexturePointer generatePreIntegratedScattering(RenderArgs* args);

private:
    typedef gpu::BufferView UniformBufferView;

    // Class describing the uniform buffer with all the parameters common to the AO shaders
    class Parameters {
    public:
        // Resolution info
        glm::vec4 resolutionInfo { -1.0f, 0.0f, 0.0f, 0.0f };
        // Curvature algorithm
        glm::vec4 curvatureInfo{ 0.0f };

        Parameters() {}
    };
    gpu::BufferView _parametersBuffer;


    gpu::TexturePointer _scatteringTable;


    gpu::PipelinePointer _scatteringPipeline;

    gpu::PipelinePointer getScatteringPipeline();
};

#endif // hifi_SubsurfaceScattering_h
