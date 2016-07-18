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


// SurfaceGeometryFramebuffer is  a helper class gathering in one place theframebuffers and targets describing the surface geometry linear depth
// from a z buffer
class LinearDepthFramebuffer {
public:
    LinearDepthFramebuffer();

    gpu::FramebufferPointer getLinearDepthFramebuffer();
    gpu::TexturePointer getLinearDepthTexture();

    gpu::FramebufferPointer getDownsampleFramebuffer();
    gpu::TexturePointer getHalfLinearDepthTexture();
    gpu::TexturePointer getHalfNormalTexture();

    // Update the depth buffer which will drive the allocation of all the other resources according to its size.
    void updatePrimaryDepth(const gpu::TexturePointer& depthBuffer);
    gpu::TexturePointer getPrimaryDepthTexture();
    const glm::ivec2& getDepthFrameSize() const { return _frameSize; }

    void setResolutionLevel(int level);
    int getResolutionLevel() const { return _resolutionLevel; }

protected:
    void clear();
    void allocate();

    gpu::TexturePointer _primaryDepthTexture;

    gpu::FramebufferPointer _linearDepthFramebuffer;
    gpu::TexturePointer _linearDepthTexture;

    gpu::FramebufferPointer _downsampleFramebuffer;
    gpu::TexturePointer _halfLinearDepthTexture;
    gpu::TexturePointer _halfNormalTexture;

    
    glm::ivec2 _frameSize;
    glm::ivec2 _halfFrameSize;
    int _resolutionLevel{ 0 };
};

using LinearDepthFramebufferPointer = std::shared_ptr<LinearDepthFramebuffer>;


class LinearDepthPassConfig : public render::Job::Config {
    Q_OBJECT
    Q_PROPERTY(double gpuTime READ getGpuTime)
public:
    LinearDepthPassConfig() : render::Job::Config(true) {}

    double getGpuTime() { return gpuTime; }

    double gpuTime{ 0.0 };

signals:
    void dirty();
};

class LinearDepthPass {
public:
    using Inputs = render::VaryingSet2<DeferredFrameTransformPointer, DeferredFramebufferPointer>;
    using Outputs = render::VaryingSet5<LinearDepthFramebufferPointer, gpu::FramebufferPointer, gpu::TexturePointer, gpu::TexturePointer, gpu::TexturePointer>;
    using Config = LinearDepthPassConfig;
    using JobModel = render::Job::ModelIO<LinearDepthPass, Inputs, Outputs, Config>;

    LinearDepthPass();

    void configure(const Config& config);
    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext, const Inputs& inputs, Outputs& outputs);
    
private:
    typedef gpu::BufferView UniformBufferView;

    LinearDepthFramebufferPointer _linearDepthFramebuffer;

    const gpu::PipelinePointer& getLinearDepthPipeline();
    gpu::PipelinePointer _linearDepthPipeline;

    const gpu::PipelinePointer& getDownsamplePipeline();
    gpu::PipelinePointer _downsamplePipeline;

    gpu::RangeTimer _gpuTimer;
};


// SurfaceGeometryFramebuffer is  a helper class gathering in one place theframebuffers and targets describing the surface geometry linear depth and curvature generated
// from a z buffer and a normal buffer
class SurfaceGeometryFramebuffer {
public:
    SurfaceGeometryFramebuffer();

    gpu::FramebufferPointer getCurvatureFramebuffer();
    gpu::TexturePointer getCurvatureTexture();

    // Update the source framebuffer size which will drive the allocation of all the other resources.
    void updateLinearDepth(const gpu::TexturePointer& linearDepthBuffer);
    gpu::TexturePointer getLinearDepthTexture();
    const glm::ivec2& getSourceFrameSize() const { return _frameSize; }
    glm::ivec2 getCurvatureFrameSize() const { return _frameSize >> _resolutionLevel; }

    void setResolutionLevel(int level);
    int getResolutionLevel() const { return _resolutionLevel; }

protected:
    void clear();
    void allocate();

    gpu::TexturePointer _linearDepthTexture;

    gpu::FramebufferPointer _curvatureFramebuffer;
    gpu::TexturePointer _curvatureTexture;

    glm::ivec2 _frameSize;
    int _resolutionLevel{ 0 };
};

using SurfaceGeometryFramebufferPointer = std::shared_ptr<SurfaceGeometryFramebuffer>;

class SurfaceGeometryPassConfig : public render::Job::Config {
    Q_OBJECT
    Q_PROPERTY(float depthThreshold MEMBER depthThreshold NOTIFY dirty)
    Q_PROPERTY(float basisScale MEMBER basisScale NOTIFY dirty)
    Q_PROPERTY(float curvatureScale MEMBER curvatureScale NOTIFY dirty)
    Q_PROPERTY(int resolutionLevel MEMBER resolutionLevel NOTIFY dirty)
    Q_PROPERTY(double gpuTime READ getGpuTime)
public:
    SurfaceGeometryPassConfig() : render::Job::Config(true) {}

    float depthThreshold{ 0.005f }; // meters
    float basisScale{ 1.0f };
    float curvatureScale{ 10.0f };
    int resolutionLevel{ 0 };

    double getGpuTime() { return gpuTime; }

    double gpuTime{ 0.0 };

signals:
    void dirty();
};

class SurfaceGeometryPass {
public:
    using Inputs = render::VaryingSet3<DeferredFrameTransformPointer, DeferredFramebufferPointer, LinearDepthFramebufferPointer>;
    using Outputs = render::VaryingSet2<SurfaceGeometryFramebufferPointer, gpu::FramebufferPointer>;
    using Config = SurfaceGeometryPassConfig;
    using JobModel = render::Job::ModelIO<SurfaceGeometryPass, Inputs, Outputs, Config>;

    SurfaceGeometryPass();

    void configure(const Config& config);
    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext, const Inputs& inputs, Outputs& outputs);
    
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

    SurfaceGeometryFramebufferPointer _surfaceGeometryFramebuffer;

    const gpu::PipelinePointer& getCurvaturePipeline();

    gpu::PipelinePointer _curvaturePipeline;


    gpu::RangeTimer _gpuTimer;
};

#endif // hifi_SurfaceGeometryPass_h
