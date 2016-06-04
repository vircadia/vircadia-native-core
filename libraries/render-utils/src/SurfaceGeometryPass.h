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

class SurfaceGeometryPassConfig : public render::Job::Config {
    Q_OBJECT
    Q_PROPERTY(double gpuTime READ getGpuTime)
public:
    SurfaceGeometryPassConfig() : render::Job::Config(true) {}

    double getGpuTime() { return gpuTime; }

    double gpuTime{ 0.0 };

signals:
    void dirty();
};

class SurfaceGeometryPass {
public:
    using Config = SurfaceGeometryPassConfig;
    using JobModel = render::Job::ModelI<SurfaceGeometryPass, DeferredFrameTransformPointer, Config>;

    SurfaceGeometryPass();

    void configure(const Config& config);
    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext, const DeferredFrameTransformPointer& frameTransform);
    
private:
    typedef gpu::BufferView UniformBufferView;

    // Class describing the uniform buffer with all the parameters common to the AO shaders
    class Parameters {
    public:
        // Resolution info
        glm::vec4 resolutionInfo { -1.0f, 0.0f, 0.0f, 0.0f };
        // radius info is { R, R^2, 1 / R^6, ObscuranceScale}
        glm::vec4 radiusInfo{ 0.5f, 0.5f * 0.5f, 1.0f / (0.25f * 0.25f * 0.25f), 1.0f };
        // Dithering info 
        glm::vec4 ditheringInfo { 0.0f, 0.0f, 0.01f, 1.0f };
        // Sampling info
        glm::vec4 sampleInfo { 11.0f, 1.0f/11.0f, 7.0f, 1.0f };
        // Blurring info
        glm::vec4 blurInfo { 1.0f, 3.0f, 2.0f, 0.0f };
         // gaussian distribution coefficients first is the sampling radius (max is 6)
        const static int GAUSSIAN_COEFS_LENGTH = 8;
        float _gaussianCoefs[GAUSSIAN_COEFS_LENGTH];
        
        Parameters() {}
    };
    gpu::BufferView _parametersBuffer;

    const gpu::PipelinePointer& getCurvaturePipeline();

    gpu::PipelinePointer _curvaturePipeline;

    gpu::RangeTimer _gpuTimer;
};

#endif // hifi_SurfaceGeometryPass_h
