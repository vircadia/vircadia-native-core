//
//  BloomEffect.h
//  render-utils/src/
//
//  Created by Olivier Prat on 09/25/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_render_utils_BloomEffect_h
#define hifi_render_utils_BloomEffect_h

#include <render/Engine.h>

#include "DeferredFrameTransform.h"

class BloomConfig : public render::Task::Config {
    Q_OBJECT
        Q_PROPERTY(float intensity MEMBER intensity WRITE setIntensity NOTIFY dirty)
        Q_PROPERTY(float size MEMBER size WRITE setSize NOTIFY dirty)

public:

    BloomConfig() : render::Task::Config(true) {}

    float intensity{ 0.2f };
    float size{ 0.4f };

    void setIntensity(float value);
    void setSize(float value);

signals:
    void dirty();
};

class ThresholdConfig : public render::Job::Config {
    Q_OBJECT
        Q_PROPERTY(float threshold MEMBER threshold NOTIFY dirty)

public:

    float threshold{ 0.25f };

signals:
    void dirty();
};

class ThresholdAndDownsampleJob {
public:
    using Inputs = render::VaryingSet2<DeferredFrameTransformPointer, gpu::FramebufferPointer>;
    using Outputs = gpu::FramebufferPointer;
    using Config = ThresholdConfig;
    using JobModel = render::Job::ModelIO<ThresholdAndDownsampleJob, Inputs, Outputs, Config>;

    ThresholdAndDownsampleJob();

    void configure(const Config& config);
    void run(const render::RenderContextPointer& renderContext, const Inputs& inputs, Outputs& outputs);

private:

    gpu::FramebufferPointer _downsampledBuffer;
    gpu::PipelinePointer _pipeline;
    float _threshold;
};

class DebugBloomConfig : public render::Job::Config {
    Q_OBJECT

public:

    DebugBloomConfig() : render::Job::Config(true) {}

};

class DebugBloom {
public:
    using Inputs = render::VaryingSet4<gpu::FramebufferPointer, gpu::FramebufferPointer, gpu::FramebufferPointer, gpu::FramebufferPointer>;
    using Config = DebugBloomConfig;
    using JobModel = render::Job::ModelI<DebugBloom, Inputs, Config>;

    DebugBloom();
    ~DebugBloom();

    void configure(const Config& config) {}
    void run(const render::RenderContextPointer& renderContext, const Inputs& inputs);

private:
    gpu::PipelinePointer _pipeline;
};

class Bloom {
public:
    using Inputs = render::VaryingSet2<DeferredFrameTransformPointer, gpu::FramebufferPointer>;
    using Config = BloomConfig;
	using JobModel = render::Task::ModelI<Bloom, Inputs, Config>;

	Bloom();

	void configure(const Config& config);
	void build(JobModel& task, const render::Varying& inputs, render::Varying& outputs);

};

#endif // hifi_render_utils_BloomEffect_h
