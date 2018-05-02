//
//  VelocityBufferPass.h
//  libraries/render-utils/src/
//
//  Created by Sam Gateau 8/15/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_VelocityBufferPass_h
#define hifi_VelocityBufferPass_h

#include "SurfaceGeometryPass.h"


// VelocityFramebuffer is  a helper class gathering in one place theframebuffers and targets describing the surface geometry linear depth
// from a z buffer
class VelocityFramebuffer {
public:
    VelocityFramebuffer();

    gpu::FramebufferPointer getVelocityFramebuffer();
    gpu::TexturePointer getVelocityTexture();

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

    gpu::FramebufferPointer _velocityFramebuffer;
    gpu::TexturePointer _velocityTexture;

    glm::ivec2 _frameSize;
    glm::ivec2 _halfFrameSize;
    int _resolutionLevel{ 0 };
};

using VelocityFramebufferPointer = std::shared_ptr<VelocityFramebuffer>;

class VelocityBufferPassConfig : public render::GPUJobConfig {
    Q_OBJECT
    Q_PROPERTY(float depthThreshold MEMBER depthThreshold NOTIFY dirty)

public:
    VelocityBufferPassConfig() : render::GPUJobConfig(true) {}

    float depthThreshold{ 5.0f };

signals:
    void dirty();
};

class VelocityBufferPass {
public:
    using Inputs = render::VaryingSet2<DeferredFrameTransformPointer, DeferredFramebufferPointer>;
    using Outputs = render::VaryingSet3<VelocityFramebufferPointer, gpu::FramebufferPointer, gpu::TexturePointer>;
    using Config = VelocityBufferPassConfig;
    using JobModel = render::Job::ModelIO<VelocityBufferPass, Inputs, Outputs, Config>;

    VelocityBufferPass();

    void configure(const Config& config);
    void run(const render::RenderContextPointer& renderContext, const Inputs& inputs, Outputs& outputs);
    
private:
    typedef gpu::BufferView UniformBufferView;

    VelocityFramebufferPointer _velocityFramebuffer;

    const gpu::PipelinePointer& getCameraMotionPipeline(const render::RenderContextPointer& renderContext);
    gpu::PipelinePointer _cameraMotionPipeline;

    gpu::RangeTimerPointer _gpuTimer;
};


#endif // hifi_VelocityBufferPass_h
