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

#include "graphics/Bloom.h"

#include "DeferredFrameTransform.h"

class BloomConfig : public render::Task::Config {
    Q_OBJECT
};

class BloomThresholdConfig : public render::Job::Config {
    Q_OBJECT
};

class BloomThreshold {
public:
    using Inputs = render::VaryingSet3<DeferredFrameTransformPointer, gpu::FramebufferPointer, graphics::BloomPointer>;
    using Outputs = render::VaryingSet2<gpu::FramebufferPointer, float>;
    using Config = BloomThresholdConfig;
    using JobModel = render::Job::ModelIO<BloomThreshold, Inputs, Outputs, Config>;

    BloomThreshold(unsigned int downsamplingFactor);

    void configure(const Config& config);
    void run(const render::RenderContextPointer& renderContext, const Inputs& inputs, Outputs& outputs);

private:

#include "BloomThreshold.shared.slh"

    gpu::FramebufferPointer _outputBuffer;
    gpu::PipelinePointer _pipeline;
    gpu::StructBuffer<Parameters> _parameters;
};


class BloomApplyConfig : public render::Job::Config {
    Q_OBJECT
};

class BloomApply {
public:
    using Inputs = render::VaryingSet5<gpu::FramebufferPointer, gpu::FramebufferPointer, gpu::FramebufferPointer, gpu::FramebufferPointer, graphics::BloomPointer>;
    using Config = BloomApplyConfig;
    using JobModel = render::Job::ModelI<BloomApply, Inputs, Config>;

    BloomApply();

    void configure(const Config& config);
    void run(const render::RenderContextPointer& renderContext, const Inputs& inputs);

private:

#include "BloomApply.shared.slh"

    gpu::PipelinePointer _pipeline;
    gpu::StructBuffer<Parameters> _parameters;
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
    gpu::BufferPointer _params;
    DebugBloomConfig::Mode _mode;
};

class BloomEffect {
public:
    using Inputs = render::VaryingSet3<DeferredFrameTransformPointer, gpu::FramebufferPointer, graphics::BloomPointer>;
    using Config = BloomConfig;
	using JobModel = render::Task::ModelI<BloomEffect, Inputs, Config>;

	BloomEffect();

	void configure(const Config& config);
	void build(JobModel& task, const render::Varying& inputs, render::Varying& outputs);

};

#endif // hifi_render_utils_BloomEffect_h
