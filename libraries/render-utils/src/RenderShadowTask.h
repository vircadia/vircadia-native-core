//
//  RenderShadowTask.h
//  render-utils/src/
//
//  Created by Zach Pomerantz on 1/7/2016.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RenderShadowTask_h
#define hifi_RenderShadowTask_h

#include <gpu/Framebuffer.h>
#include <gpu/Pipeline.h>

#include <render/CullTask.h>

#include "Shadows_shared.slh"

#include "LightingModel.h"
#include "LightStage.h"

class ViewFrustum;

class RenderShadowMap {
public:
    using Inputs = render::VaryingSet3<render::ShapeBounds, AABox, LightStage::ShadowFramePointer>;
    using JobModel = render::Job::ModelI<RenderShadowMap, Inputs>;

    RenderShadowMap(render::ShapePlumberPointer shapePlumber, unsigned int cascadeIndex) : _shapePlumber{ shapePlumber }, _cascadeIndex{ cascadeIndex } {}
    void run(const render::RenderContextPointer& renderContext, const Inputs& inputs);

protected:
    render::ShapePlumberPointer _shapePlumber;
    unsigned int _cascadeIndex;
};

//class RenderShadowTaskConfig : public render::Task::Config::Persistent {
class RenderShadowTaskConfig : public render::Task::Config {
    Q_OBJECT
public:
   // RenderShadowTaskConfig() : render::Task::Config::Persistent(QStringList() << "Render" << "Engine" << "Shadows", true) {}
    RenderShadowTaskConfig() {}

signals:
    void dirty();
};

class RenderShadowTask {
public:
    // There is one AABox per shadow cascade
    using CascadeBoxes = render::VaryingArray<AABox, SHADOW_CASCADE_MAX_COUNT>;
    using Input = render::VaryingSet2<LightStage::FramePointer, LightingModelPointer>;
    using Output = render::VaryingSet2<CascadeBoxes, LightStage::ShadowFramePointer>;
    using Config = RenderShadowTaskConfig;
    using JobModel = render::Task::ModelIO<RenderShadowTask, Input, Output, Config>;

    RenderShadowTask() {}
    void build(JobModel& task, const render::Varying& inputs, render::Varying& outputs, render::CullFunctor cameraCullFunctor, uint8_t tagBits = 0x00, uint8_t tagMask = 0x00);

    void configure(const Config& configuration);

    struct CullFunctor {
        float _minSquareSize{ 0.0f };

        bool operator()(const RenderArgs* args, const AABox& bounds) const {
            // Cull only objects that are too small relatively to shadow frustum
            const auto boundsSquareRadius = glm::dot(bounds.getDimensions(), bounds.getDimensions());
            return boundsSquareRadius > _minSquareSize;
        }
    };

    CullFunctor _cullFunctor;
};

class RenderShadowSetupConfig : public render::Job::Config {
    Q_OBJECT
    Q_PROPERTY(float constantBias0 MEMBER constantBias0 NOTIFY dirty)
    Q_PROPERTY(float constantBias1 MEMBER constantBias1 NOTIFY dirty)
    Q_PROPERTY(float constantBias2 MEMBER constantBias2 NOTIFY dirty)
    Q_PROPERTY(float constantBias3 MEMBER constantBias3 NOTIFY dirty)
    Q_PROPERTY(float slopeBias0 MEMBER slopeBias0 NOTIFY dirty)
    Q_PROPERTY(float slopeBias1 MEMBER slopeBias1 NOTIFY dirty)
    Q_PROPERTY(float slopeBias2 MEMBER slopeBias2 NOTIFY dirty)
    Q_PROPERTY(float slopeBias3 MEMBER slopeBias3 NOTIFY dirty)
    Q_PROPERTY(float biasInput MEMBER biasInput NOTIFY dirty)
    Q_PROPERTY(float maxDistance MEMBER maxDistance NOTIFY dirty)

public:
    // Set to > 0 to experiment with these values
    float constantBias0 { 0.0f };
    float constantBias1 { 0.0f };
    float constantBias2 { 0.0f };
    float constantBias3 { 0.0f };
    float slopeBias0 { 0.0f };
    float slopeBias1 { 0.0f };
    float slopeBias2 { 0.0f };
    float slopeBias3 { 0.0f };
    float biasInput { 0.0f };
    float maxDistance { 0.0f };

signals:
    void dirty();
};

class RenderShadowSetup {
public:
    using Input = RenderShadowTask::Input;
    using Output = render::VaryingSet5<RenderArgs::RenderMode, glm::ivec2, ViewFrustumPointer, LightStage::ShadowFramePointer, graphics::LightPointer>;
    using Config = RenderShadowSetupConfig;
    using JobModel = render::Job::ModelIO<RenderShadowSetup, Input, Output, Config>;

    RenderShadowSetup();
    void configure(const Config& config);
    void run(const render::RenderContextPointer& renderContext, const Input& input, Output& output);

private:
    ViewFrustumPointer _cameraFrustum;
    ViewFrustumPointer _coarseShadowFrustum;
    struct {
        float _constant;
        float _slope;
    } _bias[SHADOW_CASCADE_MAX_COUNT];

    LightStage::ShadowFrame::Object _globalShadowObject;
    LightStage::ShadowFramePointer _shadowFrameCache;

    // Values from config
    float constantBias0;
    float constantBias1;
    float constantBias2;
    float constantBias3;
    float slopeBias0;
    float slopeBias1;
    float slopeBias2;
    float slopeBias3;
    float biasInput;
    float maxDistance;

    void setConstantBias(int cascadeIndex, float value);
    void setSlopeBias(int cascadeIndex, float value);
    void calculateBiases(float biasInput);
};

class RenderShadowCascadeSetup {
public:
    using Inputs = LightStage::ShadowFramePointer;
    using Outputs = render::VaryingSet3<render::ItemFilter, ViewFrustumPointer, RenderShadowTask::CullFunctor>;
    using JobModel = render::Job::ModelIO<RenderShadowCascadeSetup, Inputs, Outputs>;

    RenderShadowCascadeSetup(unsigned int cascadeIndex, render::ItemFilter filter) : _cascadeIndex(cascadeIndex), _filter(filter) {}

    void run(const render::RenderContextPointer& renderContext, const Inputs& input, Outputs& output);

private:
    unsigned int _cascadeIndex;
    render::ItemFilter _filter;
};

class RenderShadowCascadeTeardown {
public:
    using Input = render::ItemFilter;
    using JobModel = render::Job::ModelI<RenderShadowCascadeTeardown, Input>;
    void run(const render::RenderContextPointer& renderContext, const Input& input);
};

class RenderShadowTeardown {
public:
    using Input = RenderShadowSetup::Output;
    using JobModel = render::Job::ModelI<RenderShadowTeardown, Input>;
    void run(const render::RenderContextPointer& renderContext, const Input& input);
};

class CullShadowBounds {
public:
    using Inputs = render::VaryingSet5<render::ShapeBounds, render::ItemFilter, ViewFrustumPointer, graphics::LightPointer, RenderShadowTask::CullFunctor>;
    using Outputs = render::VaryingSet2<render::ShapeBounds, AABox>;
    using JobModel = render::Job::ModelIO<CullShadowBounds, Inputs, Outputs>;

    void run(const render::RenderContextPointer& renderContext, const Inputs& inputs, Outputs& outputs);
};

#endif // hifi_RenderShadowTask_h
