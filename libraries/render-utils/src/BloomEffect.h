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
        Q_PROPERTY(float intensity READ getIntensity WRITE setIntensity NOTIFY dirty)
        Q_PROPERTY(float size MEMBER size WRITE setSize NOTIFY dirty)

public:

    BloomConfig() : render::Task::Config(true) {}

    float size{ 0.45f };

    void setIntensity(float value);
    float getIntensity() const;
    void setSize(float value);

signals:
    void dirty();
};

class ThresholdConfig : public render::Job::Config {
    Q_OBJECT
        Q_PROPERTY(float threshold MEMBER threshold NOTIFY dirty)

public:

    float threshold{ 1.25f };

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


class BloomApplyConfig : public render::Job::Config {
    Q_OBJECT
        Q_PROPERTY(float intensity MEMBER intensity NOTIFY dirty)

public:

    float intensity{ 0.5f };

signals:
    void dirty();
};

class BloomApply {
public:
    using Inputs = render::VaryingSet4<gpu::FramebufferPointer, gpu::FramebufferPointer, gpu::FramebufferPointer, gpu::FramebufferPointer>;
    using Config = BloomApplyConfig;
    using JobModel = render::Job::ModelI<BloomApply, Inputs, Config>;

    BloomApply();

    void configure(const Config& config);
    void run(const render::RenderContextPointer& renderContext, const Inputs& inputs);

private:

    gpu::PipelinePointer _pipeline;
    float _intensity{ 1.0f };
};

class DebugBloomConfig : public render::Job::Config {
    Q_OBJECT

public:

    DebugBloomConfig() : render::Job::Config(false) {}

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
