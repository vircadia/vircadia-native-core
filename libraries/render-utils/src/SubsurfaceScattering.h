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

class SubsurfaceScatteringResource {
public:
    using UniformBufferView = gpu::BufferView;

    SubsurfaceScatteringResource();

    void setBentNormalFactors(const glm::vec4& rgbsBentFactors);
    glm::vec4 getBentNormalFactors() const;

    void setCurvatureFactors(const glm::vec2& sbCurvatureFactors);
    glm::vec2 getCurvatureFactors() const;

    void setLevel(float level);
    float getLevel() const;


    void setShowBRDF(bool show);
    bool isShowBRDF() const;


    UniformBufferView getParametersBuffer() const { return _parametersBuffer; }

    gpu::TexturePointer getScatteringTable() const { return _scatteringTable; }

    void generateScatteringTable(RenderArgs* args);
    static gpu::TexturePointer generatePreIntegratedScattering(RenderArgs* args);

protected:


    // Class describing the uniform buffer with the transform info common to the AO shaders
    // It s changing every frame
    class Parameters {
    public:
        glm::vec4 normalBentInfo{ 1.5f, 0.8f, 0.3f, 1.5f };
        glm::vec2 curvatureInfo{ 0.08f, 0.8f };
        float level{ 1.0f };
        float showBRDF{ 0.0f };

        Parameters() {}
    };
    UniformBufferView _parametersBuffer;



    gpu::TexturePointer _scatteringTable;
};

using SubsurfaceScatteringResourcePointer = std::shared_ptr<SubsurfaceScatteringResource>;



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
    using Inputs = render::VaryingSet3<DeferredFrameTransformPointer, gpu::FramebufferPointer, gpu::FramebufferPointer>;
    using Config = SubsurfaceScatteringConfig;
    using JobModel = render::Job::ModelIO<SubsurfaceScattering, Inputs, gpu::FramebufferPointer, Config>;

    SubsurfaceScattering();

    void configure(const Config& config);
    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext, const Inputs& inputs, gpu::FramebufferPointer& scatteringFramebuffer);

private:
    SubsurfaceScatteringResourcePointer _scatteringResource;

    bool updateScatteringFramebuffer(const gpu::FramebufferPointer& sourceFramebuffer, gpu::FramebufferPointer& scatteringFramebuffer);
    gpu::FramebufferPointer _scatteringFramebuffer;


    gpu::PipelinePointer _scatteringPipeline;
    gpu::PipelinePointer getScatteringPipeline();

    gpu::PipelinePointer _showLUTPipeline;
    gpu::PipelinePointer getShowLUTPipeline();
    bool _showLUT{ false };
};

#endif // hifi_SubsurfaceScattering_h
