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
#include "DeferredFramebuffer.h"
#include "SurfaceGeometryPass.h"
#include "LightingModel.h"

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
    void setShowCurvature(bool show);
    bool isShowCurvature() const;
    void setShowDiffusedNormal(bool show);
    bool isShowDiffusedNormal() const;

    UniformBufferView getParametersBuffer() const { return _parametersBuffer; }

    gpu::TexturePointer getScatteringProfile() const { return _scatteringProfile; }
    gpu::TexturePointer getScatteringTable() const { return _scatteringTable; }
    gpu::TexturePointer getScatteringSpecular() const { return _scatteringSpecular; }

    void generateScatteringTable(RenderArgs* args);

    static gpu::TexturePointer generateScatteringProfile(RenderArgs* args);
    static gpu::TexturePointer generatePreIntegratedScattering(const gpu::TexturePointer& profile, RenderArgs* args);
    static gpu::TexturePointer generateScatteringSpecularBeckmann(RenderArgs* args);

protected:


    // Class describing the uniform buffer with the transform info common to the AO shaders
    // It s changing every frame
    class Parameters {
    public:
        glm::vec4 normalBentInfo{ 1.5f, 0.8f, 0.3f, 1.5f };
        glm::vec2 curvatureInfo{ 0.08f, 0.8f };
        float level{ 1.0f };
        float showBRDF{ 0.0f };
        float showCurvature{ 0.0f };
        float showDiffusedNormal{ 0.0f };
        float spare1{ 0.0f };
        float spare2{ 0.0f };


        Parameters() {}
    };
    UniformBufferView _parametersBuffer;



    gpu::TexturePointer _scatteringProfile;
    gpu::TexturePointer _scatteringTable;
    gpu::TexturePointer _scatteringSpecular;
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

    Q_PROPERTY(bool enableScattering MEMBER enableScattering NOTIFY dirty)
    Q_PROPERTY(bool showScatteringBRDF MEMBER showScatteringBRDF NOTIFY dirty)
    Q_PROPERTY(bool showCurvature MEMBER showCurvature NOTIFY dirty)
    Q_PROPERTY(bool showDiffusedNormal MEMBER showDiffusedNormal NOTIFY dirty)

public:
    SubsurfaceScatteringConfig() : render::Job::Config(true) {}

    float bentRed{ 1.5f };
    float bentGreen{ 0.8f };
    float bentBlue{ 0.3f };
    float bentScale{ 1.5f };

    float curvatureOffset{ 0.08f };
    float curvatureScale{ 0.9f };

    bool enableScattering{ true };
    bool showScatteringBRDF{ false };
    bool showCurvature{ false };
    bool showDiffusedNormal{ false };

signals:
    void dirty();
};

class SubsurfaceScattering {
public:
    //using Inputs = render::VaryingSet4<DeferredFrameTransformPointer, gpu::FramebufferPointer, gpu::FramebufferPointer, SubsurfaceScatteringResourcePointer>;
    using Outputs = SubsurfaceScatteringResourcePointer;
    using Config = SubsurfaceScatteringConfig;
    using JobModel = render::Job::ModelO<SubsurfaceScattering, Outputs, Config>;

    SubsurfaceScattering();

    void configure(const Config& config);
    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext, Outputs& outputs);

private:
    SubsurfaceScatteringResourcePointer _scatteringResource;
};



class DebugSubsurfaceScatteringConfig : public render::Job::Config {
    Q_OBJECT

    Q_PROPERTY(bool showProfile MEMBER showProfile NOTIFY dirty)
    Q_PROPERTY(bool showLUT MEMBER showLUT NOTIFY dirty)
    Q_PROPERTY(bool showSpecularTable MEMBER showSpecularTable NOTIFY dirty)
    Q_PROPERTY(bool showCursorPixel MEMBER showCursorPixel NOTIFY dirty)
    Q_PROPERTY(glm::vec2 debugCursorTexcoord MEMBER debugCursorTexcoord NOTIFY dirty)
public:
    DebugSubsurfaceScatteringConfig() : render::Job::Config(true) {}

    bool showProfile{ false };
    bool showLUT{ false };
    bool showSpecularTable{ false };
    bool showCursorPixel{ false };
    glm::vec2 debugCursorTexcoord{ 0.5, 0.5 };

signals:
    void dirty();
};

class DebugSubsurfaceScattering {
public:
    using Inputs = render::VaryingSet6<DeferredFrameTransformPointer, DeferredFramebufferPointer, LightingModelPointer, SurfaceGeometryFramebufferPointer, gpu::FramebufferPointer, SubsurfaceScatteringResourcePointer>;
    using Config = DebugSubsurfaceScatteringConfig;
    using JobModel = render::Job::ModelI<DebugSubsurfaceScattering, Inputs, Config>;

    DebugSubsurfaceScattering();

    void configure(const Config& config);
    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext, const Inputs& inputs);

private:

    gpu::PipelinePointer _scatteringPipeline;
    gpu::PipelinePointer getScatteringPipeline();

    gpu::PipelinePointer _showLUTPipeline;
    gpu::PipelinePointer getShowLUTPipeline();
    bool _showProfile{ false };
    bool _showLUT{ false };
    bool _showSpecularTable{ false };
    bool _showCursorPixel{ false };
    glm::vec2 _debugCursorTexcoord;
};

#endif // hifi_SubsurfaceScattering_h
