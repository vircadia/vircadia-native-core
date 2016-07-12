//
//  SurfaceGeometryPass.h
//  libraries/render-utils/src/
//
//  Created by Sam Gateau 6/3/2016.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_SurfaceGeometryPass_h
#define hifi_SurfaceGeometryPass_h

#include <DependencyManager.h>

#include "render/DrawTask.h"
#include "DeferredFrameTransform.h"
#include "DeferredFramebuffer.h"

class SurfaceGeometryPassConfig : public render::Job::Config {
    Q_OBJECT
    Q_PROPERTY(float depthThreshold MEMBER depthThreshold NOTIFY dirty)
    Q_PROPERTY(float basisScale MEMBER basisScale NOTIFY dirty)
    Q_PROPERTY(float curvatureScale MEMBER curvatureScale NOTIFY dirty)
    Q_PROPERTY(double gpuTime READ getGpuTime)
public:
    SurfaceGeometryPassConfig() : render::Job::Config(true) {}

    float depthThreshold{ 0.02f }; // meters
    float basisScale{ 1.0f };
    float curvatureScale{ 10.0f };

    double getGpuTime() { return gpuTime; }

    double gpuTime{ 0.0 };

signals:
    void dirty();
};

class SurfaceGeometryPass {
public:
    using Inputs = render::VaryingSet2<DeferredFrameTransformPointer, DeferredFramebufferPointer>;
    using Outputs = render::VaryingSet2<gpu::FramebufferPointer, gpu::TexturePointer>;
    using Config = SurfaceGeometryPassConfig;
    using JobModel = render::Job::ModelIO<SurfaceGeometryPass, Inputs, Outputs, Config>;

    SurfaceGeometryPass();

    void configure(const Config& config);
    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext, const Inputs& inputs, Outputs& curvatureAndDepth);
    
    float getCurvatureDepthThreshold() const { return _parametersBuffer.get<Parameters>().curvatureInfo.x; }
    float getCurvatureBasisScale() const { return _parametersBuffer.get<Parameters>().curvatureInfo.y; }
    float getCurvatureScale() const { return _parametersBuffer.get<Parameters>().curvatureInfo.w; }

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

    const gpu::PipelinePointer& getLinearDepthPipeline();
    const gpu::PipelinePointer& getCurvaturePipeline();

    gpu::PipelinePointer _linearDepthPipeline;
    gpu::PipelinePointer _curvaturePipeline;

    gpu::RangeTimer _gpuTimer;
};

#endif // hifi_SurfaceGeometryPass_h
