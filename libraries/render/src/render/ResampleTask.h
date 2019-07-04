//
//  ResampleTask.h
//  render/src/render
//
//  Various to upsample or downsample textures into framebuffers.
//
//  Created by Olivier Prat on 10/09/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_render_ResampleTask_h
#define hifi_render_ResampleTask_h

#include "Engine.h"

namespace render {

    class HalfDownsample {
    public:
        using Config = JobConfig;
        using JobModel = Job::ModelIO<HalfDownsample, gpu::FramebufferPointer, gpu::FramebufferPointer, Config>;

        HalfDownsample();

        void configure(const Config& config);
        void run(const RenderContextPointer& renderContext, const gpu::FramebufferPointer& sourceFramebuffer, gpu::FramebufferPointer& resampledFrameBuffer);

    protected:

        static gpu::PipelinePointer _pipeline;

        gpu::FramebufferPointer _destinationFrameBuffer;

        gpu::FramebufferPointer getResampledFrameBuffer(const gpu::FramebufferPointer& sourceFramebuffer);
    };

    class UpsampleConfig : public render::Job::Config {
        Q_OBJECT
            Q_PROPERTY(float factor MEMBER factor NOTIFY dirty)
    public:

        float factor{ 1.0f };

    signals:
        void dirty();
    };

    class Upsample {
    public:
        using Config = UpsampleConfig;
        using JobModel = Job::ModelIO<Upsample, gpu::FramebufferPointer, gpu::FramebufferPointer, Config>;

        Upsample(float factor = 2.0f) : _factor{ factor } {}

        void configure(const Config& config);
        void run(const RenderContextPointer& renderContext, const gpu::FramebufferPointer& sourceFramebuffer, gpu::FramebufferPointer& resampledFrameBuffer);

    protected:

        static gpu::PipelinePointer _pipeline;

        gpu::FramebufferPointer _destinationFrameBuffer;
        float _factor{ 2.0f };

        gpu::FramebufferPointer getResampledFrameBuffer(const gpu::FramebufferPointer& sourceFramebuffer);
    };

    class UpsampleToBlitFramebuffer {
    public:
        using Input = gpu::FramebufferPointer;
        using JobModel = Job::ModelIO<UpsampleToBlitFramebuffer, Input, gpu::FramebufferPointer>;

        UpsampleToBlitFramebuffer() {}

        void run(const RenderContextPointer& renderContext, const Input& input, gpu::FramebufferPointer& resampledFrameBuffer);

    protected:

        static gpu::PipelinePointer _pipeline;
        static gpu::PipelinePointer _mirrorPipeline;
    };
}

#endif // hifi_render_ResampleTask_h
