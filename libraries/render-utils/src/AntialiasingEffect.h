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

class AntialiasingConfig : public render::Job::Config {
    Q_OBJECT
public:
    AntialiasingConfig() : render::Job::Config(true) {}
};

class Antialiasing {
public:
    using Config = AntialiasingConfig;
    using JobModel = render::Job::ModelI<Antialiasing, gpu::FramebufferPointer, Config>;

    Antialiasing();
    ~Antialiasing();
    void configure(const Config& config) {}
    void run(const render::RenderContextPointer& renderContext, const gpu::FramebufferPointer& sourceBuffer);

    const gpu::PipelinePointer& getAntialiasingPipeline(RenderArgs* args);
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
