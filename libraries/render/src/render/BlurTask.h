//
//  BlurTask.h
//  render/src/render
//
//  Created by Sam Gateau on 6/7/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_render_BlurTask_h
#define hifi_render_BlurTask_h

#include "Engine.h"

namespace render {


class BlurParams {
public:

    void setWidthHeight(int width, int height, bool isStereo);

    void setTexcoordTransform(const glm::vec4 texcoordTransformViewport);

    void setFilterRadiusScale(float scale);

    void setDepthPerspective(float oneOverTan2FOV);
    void setDepthThreshold(float threshold);

    void setLinearDepthPosFar(float farPosDepth);

    // Class describing the uniform buffer with all the parameters common to the blur shaders
    class Params {
    public:
        // Resolution info (width, height, inverse of width, inverse of height)
        glm::vec4 resolutionInfo{ 0.0f, 0.0f, 0.0f, 0.0f };

        // Viewport to Texcoord info, if the region of the blur (viewport) is smaller than the full frame
        glm::vec4 texcoordTransform{ 0.0f, 0.0f, 1.0f, 1.0f };

        // Filter info (radius scale
        glm::vec4 filterInfo{ 1.0f, 0.0f, 0.0f, 0.0f };

        // Depth info (radius scale
        glm::vec4 depthInfo{ 1.0f, 0.0f, 0.0f, 0.0f };

        // stereo info if blurring a stereo render
        glm::vec4 stereoInfo{ 0.0f };

        // LinearDepth info is { f }
        glm::vec4 linearDepthInfo{ 0.0f };

        Params() {}
    };
    gpu::BufferView _parametersBuffer;

    BlurParams();
};
using BlurParamsPointer = std::shared_ptr<BlurParams>;

class BlurInOutResource {
public:
    BlurInOutResource(bool generateOutputFramebuffer = false);

    struct Resources {
        gpu::TexturePointer sourceTexture;
        gpu::FramebufferPointer blurringFramebuffer;
        gpu::TexturePointer blurringTexture;
        gpu::FramebufferPointer finalFramebuffer;
    };

    bool updateResources(const gpu::FramebufferPointer& sourceFramebuffer, Resources& resources);

    gpu::FramebufferPointer _blurredFramebuffer;

    // the output framebuffer defined if the job needs to output the result in a new framebuffer and not in place in th einput buffer
    gpu::FramebufferPointer _outputFramebuffer;
    bool _generateOutputFramebuffer{ false };
};


class BlurGaussianConfig : public Job::Config {
    Q_OBJECT
    Q_PROPERTY(bool enabled WRITE setEnabled READ isEnabled NOTIFY dirty) // expose enabled flag
    Q_PROPERTY(float filterScale MEMBER filterScale NOTIFY dirty) // expose enabled flag
public:

    BlurGaussianConfig() : Job::Config(true) {}

    float filterScale{ 0.2f };
signals :
    void dirty();

protected:
};


class BlurGaussian {
public:
    using Config = BlurGaussianConfig;
    using JobModel = Job::ModelIO<BlurGaussian, gpu::FramebufferPointer, gpu::FramebufferPointer, Config>;

    BlurGaussian(bool generateOutputFramebuffer = false);

    void configure(const Config& config);
    void run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const gpu::FramebufferPointer& sourceFramebuffer, gpu::FramebufferPointer& blurredFramebuffer);

protected:

    BlurParamsPointer _parameters;

    gpu::PipelinePointer _blurVPipeline;
    gpu::PipelinePointer _blurHPipeline;

    gpu::PipelinePointer getBlurVPipeline();
    gpu::PipelinePointer getBlurHPipeline();

    BlurInOutResource _inOutResources;
};

class BlurGaussianDepthAwareConfig : public BlurGaussianConfig {
    Q_OBJECT
        Q_PROPERTY(float depthThreshold MEMBER depthThreshold NOTIFY dirty) // expose enabled flag
public:
    BlurGaussianDepthAwareConfig() : BlurGaussianConfig() {}

    float depthThreshold{ 1.0f };
signals:
    void dirty(); 
protected:
};

class BlurGaussianDepthAware {
public:
    using Inputs = VaryingSet2<gpu::FramebufferPointer, gpu::TexturePointer>;
    using Config = BlurGaussianDepthAwareConfig;
    using JobModel = Job::ModelIO<BlurGaussianDepthAware, Inputs, gpu::FramebufferPointer, Config>;

    BlurGaussianDepthAware(bool generateNewOutput = false, const BlurParamsPointer& params = BlurParamsPointer());

    void configure(const Config& config);
    void run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const Inputs& SourceAndDepth, gpu::FramebufferPointer& blurredFramebuffer);

    const BlurParamsPointer& getParameters() const { return _parameters; }
 
    gpu::PipelinePointer getBlurVPipeline();
    gpu::PipelinePointer getBlurHPipeline();

protected:
    gpu::PipelinePointer _blurVPipeline;
    gpu::PipelinePointer _blurHPipeline;

    BlurInOutResource _inOutResources;
    BlurParamsPointer _parameters;
};

}

#endif // hifi_render_BlurTask_h
