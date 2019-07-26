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

#ifndef hifi_render_ToneMapAndResampleTask_h
#define hifi_render_ToneMapAndResampleTask_h

#include "Engine.h"

namespace render {

    enum class ToneCurve {
        // Different tone curve available
            None,
        Gamma22,
        Reinhard,
        Filmic,
    };

    class ToneMappingConfig : public render::Job::Config {
        Q_OBJECT
        Q_PROPERTY(float exposure MEMBER exposure WRITE setExposure);
        Q_PROPERTY(int curve MEMBER curve WRITE setCurve);

    public:
        ToneMappingConfig() : render::Job::Config(true) {}

        void setExposure(float newExposure) { exposure = newExposure; emit dirty(); }
        void setCurve(int newCurve) { curve = std::max((int)ToneCurve::None, std::min((int)ToneCurve::Filmic, newCurve)); emit dirty(); }

        float exposure{ 0.0f };
        int curve{ (int)ToneCurve::Gamma22 };
    signals:
        void dirty();
    };
/*
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
*/
/*
    class ResampleConfig : public render::Job::Config {
        Q_OBJECT
        Q_PROPERTY(float factor MEMBER factor NOTIFY dirty)
    public:

        float factor{ 1.0f };

        signals:
            void dirty();
    };

    class Resample {
    public:
        using Config = ResampleConfig;
        using JobModel = Job::ModelIO<Resample, gpu::FramebufferPointer, gpu::FramebufferPointer, Config>;

        Resample(float factor = 2.0f) : _factor{ factor } {}

        void configure(const Config& config);
        void run(const RenderContextPointer& renderContext, const gpu::FramebufferPointer& sourceFramebuffer, gpu::FramebufferPointer& resampledFrameBuffer);

    protected:

        static gpu::PipelinePointer _pipeline;

        gpu::FramebufferPointer _destinationFrameBuffer;
        float _factor{ 2.0f };

        gpu::FramebufferPointer getResampledFrameBuffer(const gpu::FramebufferPointer& sourceFramebuffer);
    };
    */
    class ToneMapAndResample{
    public:
        ToneMapAndResample();
        virtual ~ToneMapAndResample() {}

        void setExposure(float exposure);
        float getExposure() const { return _parametersBuffer.get<Parameters>()._exposure; }

        void setToneCurve(ToneCurve curve);
        ToneCurve getToneCurve() const { return (ToneCurve)_parametersBuffer.get<Parameters>()._toneCurve; }

        // Inputs: lightingFramebuffer, destinationFramebuffer
        using Input = gpu::FramebufferPointer;
        using Output = gpu::FramebufferPointer;
        using Config = ToneMappingConfig;
        //using JobModel = render::Job::ModelIO<ToneMapAndResample, Input, Output, Config>;
        using JobModel = Job::ModelIO<ToneMapAndResample, Input, gpu::FramebufferPointer, Config>;

        void configure(const Config& config);
        void run(const RenderContextPointer& renderContext, const Input& input, gpu::FramebufferPointer& resampledFrameBuffer);

    protected:

        static gpu::PipelinePointer _pipeline;
        static gpu::PipelinePointer _mirrorPipeline;

        gpu::FramebufferPointer _destinationFrameBuffer;

        float _factor{ 2.0f };

        gpu::FramebufferPointer getResampledFrameBuffer(const gpu::FramebufferPointer& sourceFramebuffer);

    private:

        gpu::PipelinePointer _blitLightBuffer;

        // Class describing the uniform buffer with all the parameters common to the tone mapping shaders
        class Parameters {
        public:
            float _exposure = 0.0f;
            float _twoPowExposure = 1.0f;
            glm::vec2 spareA;
            int _toneCurve = (int)ToneCurve::Gamma22;
            glm::vec3 spareB;

            Parameters() {}
        };
        typedef gpu::BufferView UniformBufferView;
        gpu::BufferView _parametersBuffer;

        void init(RenderArgs* args);
    };
}

#endif // hifi_render_ResampleTask_h