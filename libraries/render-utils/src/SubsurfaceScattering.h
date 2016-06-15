//
//  SubsurfaceScattering.h
//  libraries/render-utils/src/
//
//  Created by Sam Gateau 6/3/2016.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_SubsurfaceScattering_h
#define hifi_SubsurfaceScattering_h

#include <DependencyManager.h>

#include "render/DrawTask.h"
#include "DeferredFrameTransform.h"

class SubsurfaceScatteringConfig : public render::Job::Config {
    Q_OBJECT
    Q_PROPERTY(float bentRed MEMBER bentRed NOTIFY dirty)
    Q_PROPERTY(float bentGreen MEMBER bentGreen NOTIFY dirty)
    Q_PROPERTY(float bentBlue MEMBER bentBlue NOTIFY dirty)
    Q_PROPERTY(float bentScale MEMBER bentScale NOTIFY dirty)

    Q_PROPERTY(float curvatureOffset MEMBER curvatureOffset NOTIFY dirty)
    Q_PROPERTY(float curvatureScale MEMBER curvatureScale NOTIFY dirty)


    Q_PROPERTY(bool showLUT MEMBER showLUT NOTIFY dirty)
public:
    SubsurfaceScatteringConfig() : render::Job::Config(true) {}

    float bentRed{ 1.5f };
    float bentGreen{ 0.8f };
    float bentBlue{ 0.3f };
    float bentScale{ 1.5f };

    float curvatureOffset{ 0.08f };
    float curvatureScale{ 0.8f };

    bool showLUT{ false };

signals:
    void dirty();
};

class SubsurfaceScattering {
public:
    using Inputs = render::VaryingTrio<DeferredFrameTransformPointer, gpu::FramebufferPointer, gpu::FramebufferPointer>;
    using Config = SubsurfaceScatteringConfig;
    using JobModel = render::Job::ModelIO<SubsurfaceScattering, Inputs, gpu::FramebufferPointer, Config>;

    SubsurfaceScattering();

    void configure(const Config& config);
    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext, const Inputs& inputs, gpu::FramebufferPointer& scatteringFramebuffer);

    static gpu::TexturePointer generatePreIntegratedScattering(RenderArgs* args);

private:
    typedef gpu::BufferView UniformBufferView;

    // Class describing the uniform buffer with all the parameters common to the AO shaders
    class Parameters {
    public:
        glm::vec4 normalBentInfo { 0.0f };
        glm::vec4 curvatureInfo{ 0.0f };

        Parameters() {}
    };
    gpu::BufferView _parametersBuffer;


    gpu::TexturePointer _scatteringTable;


    bool updateScatteringFramebuffer(const gpu::FramebufferPointer& sourceFramebuffer, gpu::FramebufferPointer& scatteringFramebuffer);
    gpu::FramebufferPointer _scatteringFramebuffer;


    gpu::PipelinePointer _scatteringPipeline;
    gpu::PipelinePointer getScatteringPipeline();

    gpu::PipelinePointer _showLUTPipeline;
    gpu::PipelinePointer getShowLUTPipeline();
    bool _showLUT{ false };
};

#endif // hifi_SubsurfaceScattering_h
