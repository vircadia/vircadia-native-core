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

    void setFilterRadiusScale(float scale);

    // Class describing the uniform buffer with all the parameters common to the blur shaders
    class Params {
    public:
        // Resolution info (width, height, inverse of width, inverse of height)
        glm::vec4 resolutionInfo{ 0.0f, 0.0f, 0.0f, 0.0f };

        // Filter info (radius scale
        glm::vec4 filterInfo{ 1.0f, 0.0f, 0.0f, 0.0f };

        // stereo info if blurring a stereo render
        glm::vec4 stereoInfo{ 0.0f };

        Params() {}
    };
    gpu::BufferView _parametersBuffer;

    BlurParams();
};
using BlurParamsPointer = std::shared_ptr<BlurParams>;

class BlurGaussianConfig : public Job::Config {
    Q_OBJECT
    Q_PROPERTY(bool enabled MEMBER enabled NOTIFY dirty) // expose enabled flag
    Q_PROPERTY(float filterScale MEMBER filterScale NOTIFY dirty) // expose enabled flag
public:

    float filterScale{ 2.0f };
signals :
    void dirty();

protected:
};


class BlurGaussian {
public:
    using Config = BlurGaussianConfig;
    using JobModel = Job::ModelI<BlurGaussian, gpu::FramebufferPointer, Config>;

    BlurGaussian();

    void configure(const Config& config);
    void run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const gpu::FramebufferPointer& sourceFramebuffer);

protected:

    BlurParamsPointer _parameters;

    gpu::PipelinePointer _blurVPipeline;
    gpu::PipelinePointer _blurHPipeline;

    gpu::PipelinePointer getBlurVPipeline();
    gpu::PipelinePointer getBlurHPipeline();

    gpu::FramebufferPointer _blurredFramebuffer;

    struct BlurringResources {
        gpu::TexturePointer sourceTexture;
        gpu::FramebufferPointer blurringFramebuffer;
        gpu::TexturePointer blurringTexture;
        gpu::FramebufferPointer finalFramebuffer;
    };
    bool updateBlurringResources(const gpu::FramebufferPointer& sourceFramebuffer, BlurringResources& blurringResources);
};

class BlurGaussianDepthAware {
public:
    using InputPair = VaryingPair<gpu::FramebufferPointer, gpu::TexturePointer>;
    using Config = BlurGaussianConfig;
    using JobModel = Job::ModelI<BlurGaussianDepthAware, InputPair, Config>;

    BlurGaussianDepthAware();

    void configure(const Config& config);
    void run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const InputPair& SourceAndDepth);

protected:

    BlurParamsPointer _parameters;

    gpu::PipelinePointer _blurVPipeline;
    gpu::PipelinePointer _blurHPipeline;

    gpu::PipelinePointer getBlurVPipeline();
    gpu::PipelinePointer getBlurHPipeline();

    gpu::FramebufferPointer _blurredFramebuffer;

    struct BlurringResources {
        gpu::TexturePointer sourceTexture;
        gpu::FramebufferPointer blurringFramebuffer;
        gpu::TexturePointer blurringTexture;
        gpu::FramebufferPointer finalFramebuffer;
    };
    bool updateBlurringResources(const gpu::FramebufferPointer& sourceFramebuffer, BlurringResources& blurringResources);
};


}

#endif // hifi_render_DrawTask_h
