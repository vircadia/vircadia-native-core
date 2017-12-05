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

    BloomConfig() : render::Task::Config(false) {}

    float size{ 0.8f };

    void setIntensity(float value);
    float getIntensity() const;
    void setSize(float value);

signals:
    void dirty();
};

class BloomThresholdConfig : public render::Job::Config {
    Q_OBJECT
        Q_PROPERTY(float threshold MEMBER threshold NOTIFY dirty)

public:

    float threshold{ 1.25f };

signals:
    void dirty();
};

class BloomThreshold {
public:
    using Inputs = render::VaryingSet2<DeferredFrameTransformPointer, gpu::FramebufferPointer>;
    using Outputs = gpu::FramebufferPointer;
    using Config = BloomThresholdConfig;
    using JobModel = render::Job::ModelIO<BloomThreshold, Inputs, Outputs, Config>;

    BloomThreshold(unsigned int downsamplingFactor);

    void configure(const Config& config);
    void run(const render::RenderContextPointer& renderContext, const Inputs& inputs, Outputs& outputs);

private:

    gpu::FramebufferPointer _outputBuffer;
    gpu::PipelinePointer _pipeline;
    float _threshold;
    unsigned int _downsamplingFactor;
};


class BloomApplyConfig : public render::Job::Config {
    Q_OBJECT
        Q_PROPERTY(float intensity MEMBER intensity NOTIFY dirty)

public:

    float intensity{ 0.8f };

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

class BloomDraw {
public:
    using Inputs = render::VaryingSet2<gpu::FramebufferPointer, gpu::FramebufferPointer>;
    using JobModel = render::Job::ModelI<BloomDraw, Inputs>;

    BloomDraw() {}

    void run(const render::RenderContextPointer& renderContext, const Inputs& inputs);

private:

    gpu::PipelinePointer _pipeline;
};

class DebugBloomConfig : public render::Job::Config {
    Q_OBJECT
        Q_PROPERTY(int mode MEMBER mode NOTIFY dirty)

public:

    enum Mode {
        MODE_LEVEL0 = 0,
        MODE_LEVEL1,
        MODE_LEVEL2,
        MODE_ALL_LEVELS,

        MODE_COUNT
    };

    DebugBloomConfig() : render::Job::Config(false) {}

    int mode{ MODE_ALL_LEVELS };

signals:
    void dirty();
};

class DebugBloom {
public:
    using Inputs = render::VaryingSet5<gpu::FramebufferPointer, gpu::FramebufferPointer, gpu::FramebufferPointer, gpu::FramebufferPointer, gpu::FramebufferPointer>;
    using Config = DebugBloomConfig;
    using JobModel = render::Job::ModelI<DebugBloom, Inputs, Config>;

    DebugBloom();

    void configure(const Config& config);
    void run(const render::RenderContextPointer& renderContext, const Inputs& inputs);

private:
    gpu::PipelinePointer _pipeline;
    DebugBloomConfig::Mode _mode;
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
