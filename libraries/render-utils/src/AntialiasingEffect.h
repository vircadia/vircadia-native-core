//
//  AntialiasingEffect.h
//  libraries/render-utils/src/
//
//  Created by Raffi Bedikian on 8/30/15
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AntialiasingEffect_h
#define hifi_AntialiasingEffect_h

#include <DependencyManager.h>

#include "render/DrawTask.h"
#include "DeferredFrameTransform.h"
#include "VelocityBufferPass.h"

class AntialiasingConfig : public render::Job::Config {
    Q_OBJECT
    Q_PROPERTY(float debugX MEMBER debugX NOTIFY dirty)
    Q_PROPERTY(float blend MEMBER blend NOTIFY dirty)
    Q_PROPERTY(float velocityScale MEMBER velocityScale NOTIFY dirty)

public:
    AntialiasingConfig() : render::Job::Config(true) {}

    float debugX{ 0.0f };
    float blend{ 0.1f };
    float velocityScale{ 1.0f };

signals:
    void dirty();
};


struct TAAParams {
    float debugX{ 0.0f };
    float blend{ 0.1f };
    float velocityScale{ 1.0f };
    float spareB;

};
using TAAParamsBuffer = gpu::StructBuffer<TAAParams>;

class Antialiasing {
public:
    using Inputs = render::VaryingSet3<DeferredFrameTransformPointer, gpu::FramebufferPointer, VelocityFramebufferPointer>;
    using Config = AntialiasingConfig;
    using JobModel = render::Job::ModelI<Antialiasing, Inputs, Config>;

    Antialiasing();
    ~Antialiasing();
    void configure(const Config& config);
    void run(const render::RenderContextPointer& renderContext, const Inputs& inputs);

    const gpu::PipelinePointer& getAntialiasingPipeline();
    const gpu::PipelinePointer& getBlendPipeline();




private:

    // Uniforms for AA
    gpu::int32 _texcoordOffsetLoc;

    gpu::FramebufferPointer _antialiasingBuffer[2];
    gpu::TexturePointer _antialiasingTexture[2];

    gpu::PipelinePointer _antialiasingPipeline;
    gpu::PipelinePointer _blendPipeline;

    TAAParamsBuffer _params;
    int _currentFrame{ 0 };
};

/*
class AntiAliasingConfig : public render::Job::Config {
    Q_OBJECT
    Q_PROPERTY(bool enabled MEMBER enabled)
public:
    AntiAliasingConfig() : render::Job::Config(true) {}
};

class Antialiasing {
public:
    using Config = AntiAliasingConfig;
    using JobModel = render::Job::ModelI<Antialiasing, gpu::FramebufferPointer, Config>;
    
    Antialiasing();
    ~Antialiasing();
    void configure(const Config& config) {}
    void run(const render::RenderContextPointer& renderContext, const gpu::FramebufferPointer& sourceBuffer);
    
    const gpu::PipelinePointer& getAntialiasingPipeline();
    const gpu::PipelinePointer& getBlendPipeline();
    
private:
    
    // Uniforms for AA
    gpu::int32 _texcoordOffsetLoc;
    
    gpu::FramebufferPointer _antialiasingBuffer;
    
    gpu::TexturePointer _antialiasingTexture;
    
    gpu::PipelinePointer _antialiasingPipeline;
    gpu::PipelinePointer _blendPipeline;
    int _geometryId { 0 };
};
*/
class JitterSampleConfig : public render::Job::Config {
    Q_OBJECT
    Q_PROPERTY(float scale MEMBER scale NOTIFY dirty)
    Q_PROPERTY(bool freeze MEMBER freeze NOTIFY dirty)
public:
    JitterSampleConfig() : render::Job::Config(true) {}
    
    float scale { 0.5f };
    bool freeze{ false };
signals:
    void dirty();
};

class JitterSample {
public:
 
    struct SampleSequence {
    SampleSequence();
        static const int SEQUENCE_LENGTH { 8 };
        int sequenceLength{ SEQUENCE_LENGTH };
        int currentIndex { 0 };
        
        glm::vec2 offsets[SEQUENCE_LENGTH];
    };
    
    using JitterBuffer = gpu::StructBuffer<SampleSequence>;
    
    using Config = JitterSampleConfig;
    using JobModel = render::Job::ModelO<JitterSample, JitterBuffer, Config>;
    
    void configure(const Config& config);
    void run(const render::RenderContextPointer& renderContext, JitterBuffer& jitterBuffer);

    
private:
    
    JitterBuffer _jitterBuffer;
    float _scale { 1.0 };
    bool _freeze { false };
};

#endif // hifi_AntialiasingEffect_h
