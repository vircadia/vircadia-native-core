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
    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext, const gpu::FramebufferPointer& sourceBuffer);

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

#endif // hifi_AntialiasingEffect_h
