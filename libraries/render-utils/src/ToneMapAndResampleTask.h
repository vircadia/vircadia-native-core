//
//  ToneMapAndResample.h
//  libraries/render-utils/src
//
//  Created by Anna Brewer on 7/3/19.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ToneMapAndResample_h
#define hifi_ToneMapAndResample_h

#include <DependencyManager.h>
#include <NumericalConstants.h>

#include <gpu/Resource.h>
#include <gpu/Pipeline.h>
#include <render/Forward.h>
#include <render/DrawTask.h>

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

class ToneMapAndResample {
public:
    ToneMapAndResample();
    virtual ~ToneMapAndResample() {}

    void render(RenderArgs* args, const gpu::TexturePointer& lightingBuffer, gpu::FramebufferPointer& destinationBuffer);

    void setExposure(float exposure);
    float getExposure() const { return _parametersBuffer.get<Parameters>()._exposure; }

    void setToneCurve(ToneCurve curve);
    ToneCurve getToneCurve() const { return (ToneCurve)_parametersBuffer.get<Parameters>()._toneCurve; }

    // Inputs: lightingFramebuffer, destinationFramebuffer
    using Input = render::VaryingSet2<gpu::FramebufferPointer, gpu::FramebufferPointer>;
    using Output = gpu::FramebufferPointer;
    using Config = ToneMappingConfig;
    using JobModel = render::Job::ModelIO<ToneMapAndResample, Input, Output, Config>;

    void configure(const Config& config);
    void run(const render::RenderContextPointer& renderContext, const Input& input, Output& output);

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

    void init();
};

#endif // hifi_ToneMapAndResample_h
